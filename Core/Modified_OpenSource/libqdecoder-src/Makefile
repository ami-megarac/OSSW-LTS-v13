#------------------------------------------------------------------------------------------------------
#                         Makefile to build libqdecoder
#                 Copyright (c) 2012 American Megatrends Inc 
#------------------------------------------------------------------------------------------------------

#------- Set y/n in the following line to enable/disable debugging------
DEBUG = n

#---------------------- Change according to your files ---------------------------------------
LIBRARY_NAME	= libqdecoder

SRC   = 	internal.c
SRC +=  qcgireq.c
SRC +=  qcgires.c
SRC +=	qcgisess.c
SRC +=	qentry.c

CFLAGS += -DENABLE_FASTCGI

CFLAGS += -I${SPXINC}/fastcgi
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/racsessioninfo
#CFLAGS += -I${SPXINC}/safesystem

LIBS += -L${SPXLIB}/fastcgi -lfcgi
LIBS += -L${SPXLIB}/unix -lunix
LIBS	+=	-L${SPXLIB}/racsessioninfo -lracsessioninfo
#LIBS += -L${SPXLIB}/safesystem -lsafesystem

include ${TOOLDIR}/rules/Rules.make.libs

#------------------------------------------------------------------------------------------------------
