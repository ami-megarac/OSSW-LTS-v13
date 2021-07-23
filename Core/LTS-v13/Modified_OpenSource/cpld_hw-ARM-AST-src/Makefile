DEBUG := n
TARGET := cpld_hw

OBJS := cpld.o
OBJS += ast_jtag.o
OBJS += jbicomp.o
OBJS += jbijtag.o
OBJS += jbimain.o
OBJS += jbistub.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/helper
EXTRA_CFLAGS += -I${SPXINC}/cpld

#KBUILD_EXTRA_SYMBOLS += ${SPXINC}/jtag/Module.symvers
#KBUILD_EXTRA_SYMBOLS += ${SPXINC}/jtag_hw/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers
