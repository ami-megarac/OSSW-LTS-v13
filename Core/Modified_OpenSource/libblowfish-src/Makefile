#-------------------------------------------------------------------------------------------------------------------------
#                      Makefile to build libblowfish encryption
#                 Copyright (c) 2008 American Megatrends Inc 
#-------------------------------------------------------------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging-------------------
DEBUG = n

#---------------------- Change according to your files ---------------------------------------------------------
LIBRARY_NAME 	= libblowfish

SRC    =  blowfish.c

CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -D_POSIX_SAVED_IDS -DPATH_MAX=4096 -DMAX_INPUT=255 -DUNIXTOOLS
CFLAGS += -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -DHAVE_DEV_URANDOM
CFLAGS += -I${SPXINC}/unix

LIBS += -L${SPXLIB}/unix -lunix

include ${TOOLDIR}/rules/Rules.make.libs
#----------------------------------------------------------------------------------------------------------------------------
