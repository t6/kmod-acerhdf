KMOD=		acerhdf
KMODDIR=	/boot/modules
SRCS=		acerhdf.c opt_acpi.h device_if.h bus_if.h acpi_if.h

.include <bsd.kmod.mk>
