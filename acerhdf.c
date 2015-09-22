/*
 * acerhdf - A driver which monitors the temperature
 *           of the aspire one netbook, turns on/off the fan
 *           as soon as the upper/lower threshold is reached.
 *
 * (C) 2015 - Tobias Kortkamp   t@tobik.me
 *
 * Based on the Linux kernel module "acerhdf":
 *
 * (C) 2009 - Peter Feuerer     peter (a) piie.net
 *                              http://piie.net
 *     2009 Borislav Petkov     bp (a) alien8.de
 *
 * Inspired by and many thanks to:
 *  o acerfand   - Rachel Greenham
 *  o acer_ec.pl - Michael Kurz     michi.kurz (at) googlemail.com
 *               - Petr Tomasek     tomasek (#) etf,cuni,cz
 *               - Carlos Corbacho  cathectic (at) gmail.com
 *  o lkml       - Matthew Garrett
 *               - Borislav Petkov
 *               - Andreas Mohr
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <contrib/dev/acpica/include/acpi.h>
#include <sys/bus.h>
#include <dev/acpica/acpivar.h>

#if __FreeBSD__ < 11
// kern_getenv is getenv in FreeBSD < 11
#define kern_getenv getenv
#endif

/*
 * According to the Atom N270 datasheet,
 * (http://download.intel.com/design/processor/datashts/320032.pdf) the
 * CPU's optimal operating limits denoted in junction temperature as
 * measured by the on-die thermal monitor are within 0 <= Tj <= 90. So,
 * assume 89°C is critical temperature.
 */
#define ACERHDF_TEMP_CRIT 89

/*
 * No matter what value the user puts into the fanon variable, turn on the fan
 * at 80 degree Celsius to prevent hardware damage
 */
#define ACERHDF_MAX_FANON 80
#define ACERHDF_MIN_FANON 50

#define ACERHDF_MAX_FANOFF 80
#define ACERHDF_MIN_FANOFF 50

/*
 * Maximum interval between two temperature checks is 15 seconds, as the die
 * can get hot really fast under heavy load (plus we shouldn't forget about
 * possible impact of _external_ aggressive sources such as heaters, sun etc.)
 */
#define ACERHDF_MAX_INTERVAL 15
#define ACERHDF_MIN_INTERVAL 1

/*
 * cmd_off:  to switch the fan completely off and check if the fan is off
 * cmd_auto: to set the BIOS in control of the fan. The BIOS then
 *           regulates the fan speed depending on the temperature
 */
struct fancmd {
    UINT8 cmd_off;
    UINT8 cmd_auto;
};

struct manualcmd {
    UINT8 mreg;
    UINT8 moff;
};

/* default register and command to disable fan in manual mode */
static const struct manualcmd mcmd = {
    .mreg = 0x94,
    .moff = 0xff,
};

/* BIOS settings */
struct bios_settings {
    const char *vendor;
    const char *product;
    const char *version;
    UINT8 fanreg;
    UINT8 tempreg;
    struct fancmd cmd;
    int mcmd_enable;
};

/* Register addresses and values for different BIOS versions */
static const struct bios_settings bios_tbl[] = {
        /* AOA110 */
        {"Acer", "AOA110", "v0.3109", 0x55, 0x58, {0x1f, 0x00}, 0},
        {"Acer", "AOA110", "v0.3114", 0x55, 0x58, {0x1f, 0x00}, 0},
        {"Acer", "AOA110", "v0.3301", 0x55, 0x58, {0xaf, 0x00}, 0},
        {"Acer", "AOA110", "v0.3304", 0x55, 0x58, {0xaf, 0x00}, 0},
        {"Acer", "AOA110", "v0.3305", 0x55, 0x58, {0xaf, 0x00}, 0},
        {"Acer", "AOA110", "v0.3307", 0x55, 0x58, {0xaf, 0x00}, 0},
        {"Acer", "AOA110", "v0.3308", 0x55, 0x58, {0x21, 0x00}, 0},
        {"Acer", "AOA110", "v0.3309", 0x55, 0x58, {0x21, 0x00}, 0},
        {"Acer", "AOA110", "v0.3310", 0x55, 0x58, {0x21, 0x00}, 0},
        /* AOA150 */
        {"Acer", "AOA150", "v0.3114", 0x55, 0x58, {0x1f, 0x00}, 0},
        {"Acer", "AOA150", "v0.3301", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3304", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3305", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3307", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3308", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3309", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AOA150", "v0.3310", 0x55, 0x58, {0x20, 0x00}, 0},
        /* LT1005u */
        {"Acer", "LT-10Q", "v0.3310", 0x55, 0x58, {0x20, 0x00}, 0},
        /* Acer 1410 */
        {"Acer", "Aspire 1410", "v0.3108", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v0.3113", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v0.3115", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v0.3117", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v0.3119", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v0.3120", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v1.3204", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v1.3303", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v1.3308", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v1.3310", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1410", "v1.3314", 0x55, 0x58, {0x9e, 0x00}, 0},
        /* Acer 1810xx */
        {"Acer", "Aspire 1810TZ", "v0.3108", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3108", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v0.3113", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3113", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v0.3115", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3115", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v0.3117", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3117", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v0.3119", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3119", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v0.3120", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v0.3120", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v1.3204", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v1.3204", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v1.3303", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v1.3303", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v1.3308", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v1.3308", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v1.3310", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v1.3310", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810TZ", "v1.3314", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1810T",  "v1.3314", 0x55, 0x58, {0x9e, 0x00}, 0},
        /* Acer 5755G */
        {"Acer", "Aspire 5755G",  "V1.20",   0xab, 0xb4, {0x00, 0x08}, 0},
        {"Acer", "Aspire 5755G",  "V1.21",   0xab, 0xb3, {0x00, 0x08}, 0},
        /* Acer 521 */
        {"Acer", "AO521", "V1.11", 0x55, 0x58, {0x1f, 0x00}, 0},
        /* Acer 531 */
        {"Acer", "AO531h", "v0.3104", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AO531h", "v0.3201", 0x55, 0x58, {0x20, 0x00}, 0},
        {"Acer", "AO531h", "v0.3304", 0x55, 0x58, {0x20, 0x00}, 0},
        /* Acer 751 */
        {"Acer", "AO751h", "V0.3206", 0x55, 0x58, {0x21, 0x00}, 0},
        {"Acer", "AO751h", "V0.3212", 0x55, 0x58, {0x21, 0x00}, 0},
        /* Acer 753 */
        {"Acer", "Aspire One 753", "V1.24", 0x93, 0xac, {0x14, 0x04}, 1},
        /* Acer 1825 */
        {"Acer", "Aspire 1825PTZ", "V1.3118", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Acer", "Aspire 1825PTZ", "V1.3127", 0x55, 0x58, {0x9e, 0x00}, 0},
        /* Acer Extensa 5420 */
        {"Acer", "Extensa 5420", "V1.17", 0x93, 0xac, {0x14, 0x04}, 1},
        /* Acer Aspire 5315 */
        {"Acer", "Aspire 5315", "V1.19", 0x93, 0xac, {0x14, 0x04}, 1},
        /* Acer Aspire 5739 */
        {"Acer", "Aspire 5739G", "V1.3311", 0x55, 0x58, {0x20, 0x00}, 0},
        /* Acer TravelMate 7730 */
        {"Acer", "TravelMate 7730G", "v0.3509", 0x55, 0x58, {0xaf, 0x00}, 0},
        /* Acer TravelMate TM8573T */
        {"Acer", "TM8573T", "V1.13", 0x93, 0xa8, {0x14, 0x04}, 1},
        /* Gateway */
        {"Gateway", "AOA110", "v0.3103",  0x55, 0x58, {0x21, 0x00}, 0},
        {"Gateway", "AOA150", "v0.3103",  0x55, 0x58, {0x20, 0x00}, 0},
        {"Gateway", "LT31",   "v1.3103",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Gateway", "LT31",   "v1.3201",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Gateway", "LT31",   "v1.3302",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Gateway", "LT31",   "v1.3303t", 0x55, 0x58, {0x9e, 0x00}, 0},
        /* Packard Bell */
        {"Packard Bell", "DOA150",  "v0.3104",  0x55, 0x58, {0x21, 0x00}, 0},
        {"Packard Bell", "DOA150",  "v0.3105",  0x55, 0x58, {0x20, 0x00}, 0},
        {"Packard Bell", "AOA110",  "v0.3105",  0x55, 0x58, {0x21, 0x00}, 0},
        {"Packard Bell", "AOA150",  "v0.3105",  0x55, 0x58, {0x20, 0x00}, 0},
        {"Packard Bell", "ENBFT",   "V1.3118",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "ENBFT",   "V1.3127",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v1.3303",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3120",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3108",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3113",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3115",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3117",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v0.3119",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMU",   "v1.3204",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMA",   "v1.3201",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMA",   "v1.3302",  0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTMA",   "v1.3303t", 0x55, 0x58, {0x9e, 0x00}, 0},
        {"Packard Bell", "DOTVR46", "v1.3308",  0x55, 0x58, {0x9e, 0x00}, 0},
        /* pewpew-terminator */
        {"", "", "", 0, 0, {0, 0}, 0}
};

typedef enum {
    ACERHDF_FAN_OFF,
    ACERHDF_FAN_AUTO
} acerhdf_fanstate;

struct acerhdf_softc {
    device_t dev;
    device_t ec_dev;

    struct callout tick_handle;

    int interval;
    UINT8 fanon;
    UINT8 fanoff;
    int enabled;

    struct sysctl_ctx_list *sysctl_ctx;
    struct sysctl_oid *sysctl_tree;
};

static devclass_t acerhdf_devclass;

ACPI_SERIAL_DECL(acerhdf, "Acer Aspire One fan control");

static const struct bios_settings *bios_cfg = NULL;

static ACPI_STATUS acerhdf_set_fanstate(struct acerhdf_softc *sc,
                                        acerhdf_fanstate state);
static ACPI_STATUS acerhdf_get_fanstate(struct acerhdf_softc *sc,
                                        acerhdf_fanstate *state);
static ACPI_STATUS acerhdf_get_temperature(struct acerhdf_softc *sc, int *t);
static int acerhdf_probe(device_t dev);
static int acerhdf_attach(device_t dev);
static int acerhdf_detach(device_t dev);

static ACPI_STATUS
acerhdf_set_fanstate(struct acerhdf_softc *sc, acerhdf_fanstate state) {
    UINT64 cmd;

    cmd = state == ACERHDF_FAN_OFF ?
        bios_cfg->cmd.cmd_off : bios_cfg->cmd.cmd_auto;

    ACPI_STATUS retval = ACPI_EC_WRITE(sc->ec_dev, bios_cfg->fanreg, cmd, 1);
    if (ACPI_FAILURE(retval)) {
        return retval;
    }

    if (bios_cfg->mcmd_enable && state == ACERHDF_FAN_OFF) {
        retval = ACPI_EC_WRITE(sc->ec_dev, mcmd.mreg, mcmd.moff, 1);
        if (ACPI_FAILURE(retval)) {
            return retval;
        }
    }

    if (bootverbose) {
        device_printf(sc->dev, "fan state changed to '%s'\n",
                      state == ACERHDF_FAN_OFF ? "off" : "auto");
    }

    return retval;
}

static ACPI_STATUS
acerhdf_get_fanstate(struct acerhdf_softc *sc, acerhdf_fanstate *state)
{
    UINT64 fan;

    ACPI_STATUS retval = ACPI_EC_READ(sc->ec_dev,
                                      bios_cfg->fanreg,
                                      &fan,
                                      0);
    if (ACPI_SUCCESS(retval)) {
        if (fan == bios_cfg->cmd.cmd_off) {
            *state = ACERHDF_FAN_OFF;
        } else {
            *state = ACERHDF_FAN_AUTO;
        }
    }

    return retval;
}

static ACPI_STATUS
acerhdf_get_temperature(struct acerhdf_softc *sc, int *t)
{
    UINT64 read_temp = 0;

    ACPI_STATUS retval = ACPI_EC_READ(sc->ec_dev,
                                      bios_cfg->tempreg,
                                      &read_temp,
                                      1);
    if (ACPI_SUCCESS(retval)) {
        *t = read_temp;
    }

    return retval;
}

static int
acerhdf_sysctl_fanon(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int error = 0;
    int temp = sc->fanon;

    error = sysctl_handle_int(oidp, &temp, 0, req);
    if (error || !req->newptr) {
        return error;
    }

    if (temp > ACERHDF_MAX_FANON || temp < ACERHDF_MIN_FANON) {
        return EINVAL;
    }

    sc->fanon = temp;

    return 0;
}

static int
acerhdf_sysctl_fanoff(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int error = 0;
    int temp = sc->fanoff;

    error = sysctl_handle_int(oidp, &temp, 0, req);
    if (error || !req->newptr) {
        return error;
    }

    if (temp < ACERHDF_MIN_FANOFF || temp > ACERHDF_MAX_FANOFF) {
        return EINVAL;
    }

    sc->fanoff = temp;

    return 0;
}

static int
acerhdf_sysctl_temperature(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int temp;
    int error;

    ACPI_SERIAL_BEGIN(acerhdf);
    error = acerhdf_get_temperature(sc, &temp);
    ACPI_SERIAL_END(acerhdf);
    if (error) {
        return EINVAL;
    }

    return sysctl_handle_int(oidp, &temp, 1, req);
}

static int
acerhdf_sysctl_interval(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int error = 0;
    int t = sc->interval;

    error = sysctl_handle_int(oidp, &t, 0, req);
    if (error || !req->newptr) {
        return error;
    }

    if (t > ACERHDF_MAX_INTERVAL || t < ACERHDF_MIN_INTERVAL) {
        return EINVAL;
    }

    sc->interval = t;

    return 0;
}

static int
acerhdf_sysctl_enabled(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int error = 0;
    int val = sc->enabled;

    error = sysctl_handle_int(oidp, &val, 0, req);
    if (error || !req->newptr) {
        return error;
    }

    if (val != 0 && val != 1) {
        error = EINVAL;
    } else {
        sc->enabled = val;
    }

    if (!sc->enabled) {
        // Make sure the fan is on when we are not in control of it!
        ACPI_SERIAL_BEGIN(acerhdf);
        acerhdf_set_fanstate(sc, ACERHDF_FAN_AUTO);
        ACPI_SERIAL_END(acerhdf);
    }

    return error;
}

static int
acerhdf_sysctl_fanstate(SYSCTL_HANDLER_ARGS)
{
    struct acerhdf_softc *sc = (struct acerhdf_softc *)oidp->oid_arg1;
    int error = 0;

    acerhdf_fanstate state;
    ACPI_SERIAL_BEGIN(acerhdf);
    error = acerhdf_get_fanstate(sc, &state);
    ACPI_SERIAL_END(acerhdf);
    if (error) {
        return error;
    }

    char *description;
    if (state == ACERHDF_FAN_AUTO) {
        description = "auto";
    } else if (state == ACERHDF_FAN_OFF) {
        description = "off";
    } else {
        return EINVAL;
    }

    error = sysctl_handle_string(oidp,
                                 description,
                                 strlen(description),
                                 req);

    if (error || !req->newptr) {
        return error;
    }

    return 0;
}

static void
acerhdf_tick(void *data) {
    struct acerhdf_softc *sc = data;
    int error;

    ACPI_SERIAL_BEGIN(acerhdf);

    if (!sc->enabled) {
        goto reset;
    }

    int temperature;
    error = acerhdf_get_temperature(sc, &temperature);
    if (ACPI_FAILURE(error)) {
        goto reset;
    }

    if (temperature >= ACERHDF_TEMP_CRIT) {
        panic("Critical system temperature %i\n", temperature);
    }

    acerhdf_fanstate fanstate;
    error = acerhdf_get_fanstate(sc, &fanstate);
    if (ACPI_FAILURE(error)) {
        goto reset;
    }

    if (temperature >= sc->fanon && fanstate == ACERHDF_FAN_OFF) {
        acerhdf_set_fanstate(sc, ACERHDF_FAN_AUTO);
    } else if (temperature <= sc->fanoff && fanstate == ACERHDF_FAN_AUTO) {
        acerhdf_set_fanstate(sc, ACERHDF_FAN_OFF);
    }

 reset:
    ACPI_SERIAL_END(acerhdf);
    callout_reset(&sc->tick_handle, sc->interval * hz, acerhdf_tick, sc);
}

/* checks if str begins with start */
static int
str_starts_with(const char *str, const char *start)
{
    unsigned long str_len = 0, start_len = 0;

    str_len = strlen(str);
    start_len = strlen(start);

    if (str_len >= start_len && !strncmp(str, start, start_len)) {
        return 1;
    }

    return 0;
}

static int
acerhdf_probe(device_t dev)
{
    if (acpi_disabled("acerhdf") ||
        device_get_unit(dev) != 0) {
        return ENXIO;
    }

    int error = 0;
    char *vendor, *version, *product;
    const struct bios_settings *bt = NULL;

    vendor = kern_getenv("smbios.bios.vendor");
    version = kern_getenv("smbios.bios.version");
    product = kern_getenv("smbios.system.product");

    if (!vendor || !version || !product) {
        if (bootverbose)
            device_printf(dev, "error getting hardware information\n");
        error = ENXIO;
        goto out;
    }

    /* search BIOS version and vendor in BIOS settings table */
    for (bt = bios_tbl; bt->vendor[0]; bt++) {
        /*
         * check if actual hardware BIOS vendor, product and version
         * IDs start with the strings of BIOS table entry
         */
        if (str_starts_with(vendor, bt->vendor) &&
            str_starts_with(product, bt->product) &&
            str_starts_with(version, bt->version)) {
            bios_cfg = bt;
            break;
        }
    }

    if (!bios_cfg) {
        if (bootverbose)
            device_printf(dev, "unsupported BIOS version!\n");
        error = ENXIO;
        goto out;
    }

    if (bootverbose) {
        device_printf(dev,
                      "Settings: %s/%s/%s 0x%x/0x%x/0x%x/0x%x\n",
                      bios_cfg->product,
                      bios_cfg->vendor,
                      bios_cfg->version,
                      bios_cfg->fanreg,
                      bios_cfg->tempreg,
                      bios_cfg->cmd.cmd_off,
                      bios_cfg->cmd.cmd_auto);
        device_printf(dev,
                      "Fan control disabled by default, "
                      "enable with sysctl dev.acerhdf.0.enabled=1\n");
    }

    device_set_desc(dev, "Acer Aspire One fan control");

 out:
    freeenv(vendor);
    freeenv(version);
    freeenv(product);

    return error;
}

static int
acerhdf_attach(device_t dev)
{
    struct acerhdf_softc *sc;
    devclass_t ec_devclass;

    sc = device_get_softc(dev);
    sc->dev = dev;

    /* Look for the first embedded controller */
    if (!(ec_devclass = devclass_find("acpi_ec"))) {
        if (bootverbose)
            device_printf(dev, "Couldn't find acpi_ec devclass\n");
        return (EINVAL);
    }
    if (!(sc->ec_dev = devclass_get_device(ec_devclass, 0))) {
        if (bootverbose)
            device_printf(dev, "Couldn't find acpi_ec device\n");
        return (EINVAL);
    }

    callout_init(&sc->tick_handle, CALLOUT_MPSAFE);
    callout_reset(&sc->tick_handle, sc->interval * hz, acerhdf_tick, sc);

    /* Get the sysctl tree */
    sc->sysctl_ctx = device_get_sysctl_ctx(dev);
    sc->sysctl_tree = device_get_sysctl_tree(dev);

    // Default settings
    sc->enabled = 0;
    sc->interval = 5; // seconds
    sc->fanoff = 53; // degree celsius
    sc->fanon = 60; // degree celsius

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "enabled",
                    CTLTYPE_INT | CTLFLAG_RW,
                    sc,
                    0,
                    acerhdf_sysctl_enabled,
                    "I",
                    "Kernel mode fan control: 1 = enabled, 0 = disabled");

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "temperature",
                    CTLTYPE_INT | CTLFLAG_RD,
                    sc,
                    0,
                    acerhdf_sysctl_temperature,
                    "I",
                    "Current temperature");

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "interval",
                    CTLTYPE_INT | CTLFLAG_RW,
                    sc,
                    0,
                    acerhdf_sysctl_interval,
                    "I",
                    "Temperature check interval in s");

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "fanstate",
                    CTLTYPE_STRING | CTLFLAG_RD,
                    sc,
                    0,
                    acerhdf_sysctl_fanstate,
                    "A",
                    "Fan state");

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "fanon",
                    CTLTYPE_INT | CTLFLAG_RW,
                    sc,
                    0,
                    acerhdf_sysctl_fanon,
                    "I",
                    "The temperature at which the fan should be turned on");

    SYSCTL_ADD_PROC(sc->sysctl_ctx,
                    SYSCTL_CHILDREN(sc->sysctl_tree),
                    OID_AUTO,
                    "fanoff",
                    CTLTYPE_INT | CTLFLAG_RW,
                    sc,
                    0,
                    acerhdf_sysctl_fanoff,
                    "I",
                    "The temperature at which the fan should be turned off again");

    return 0;
}

static int
acerhdf_detach(device_t dev)
{
    struct acerhdf_softc *sc = device_get_softc(dev);

    callout_stop(&sc->tick_handle);
    callout_drain(&sc->tick_handle);

    ACPI_SERIAL_BEGIN(acerhdf);
    acerhdf_set_fanstate(sc, ACERHDF_FAN_AUTO);
    ACPI_SERIAL_END(acerhdf);

    return (0);
}

static device_method_t acerhdf_methods[] = {
    /* Device interface */
    DEVMETHOD(device_probe, acerhdf_probe),
    DEVMETHOD(device_attach, acerhdf_attach),
    DEVMETHOD(device_detach, acerhdf_detach),

    DEVMETHOD_END
};

static driver_t acerhdf_driver = {
    "acerhdf",
    acerhdf_methods,
    sizeof(struct acerhdf_softc),
};

DRIVER_MODULE(acerhdf, acpi, acerhdf_driver, acerhdf_devclass, 0, 0);
MODULE_DEPEND(acerhdf, acpi, 1, 1, 1);
