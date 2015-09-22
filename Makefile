KMOD=		acerhdf
KMODDIR=	/boot/modules
SRCS=		acerhdf.c opt_acpi.h device_if.h bus_if.h acpi_if.h

MAN_URL=	https://www.freebsd.org/cgi/man.cgi?query=%N&sektion=%S&apropos=0&manpath=FreeBSD+10.2-RELEASE

README.md:
	mandoc -Thtml -Ofragment -Oman="${MAN_URL}" acerhdf.4 > ${@}

.include <bsd.kmod.mk>
