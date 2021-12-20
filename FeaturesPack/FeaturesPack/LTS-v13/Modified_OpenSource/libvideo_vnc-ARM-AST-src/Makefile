DEBUG = n

LIBRARY_NAME = libvideo_vnc

ifeq "$(CONFIG_SPX_FEATURE_VIDEO_INT)" "YES"

SRC = hwcursor.c vnc_ast.c decode.c jtables.c

else ifeq "$(CONFIG_SPX_FEATURE_VIDEO_RVAS)" "YES"

SRC = vnc_rvas.c

endif

CFLAGS += -I${SPXINC}/videocap

LIBS += -ldl

include ${TOOLDIR}/rules/Rules.make.libs
