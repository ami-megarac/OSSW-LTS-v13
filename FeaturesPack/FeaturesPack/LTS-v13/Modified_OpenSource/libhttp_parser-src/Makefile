#------------------------------------------------------------------------------------------------------
#                         Makefile to build http parser
#                 Copyright (c) 2015 American Megatrends Inc 
#------------------------------------------------------------------------------------------------------

#------- Set y/n in the following line to enable/disable debugging------
DEBUG = n

#---------------------- Change according to your files ---------------------------------------
LIBRARY_NAME	= libhttp_parser

SRC  = 	http_parser.c

CFLAGS += -I./
#CFLAGS += -I${TARGETDIR}/usr/include
#CFLAGS += -I${TARGETDIR}/usr/include/${MULTI_ARCH_LIB}

#LIBS += -L${TARGETDIR}/usr/lib
#LIBS += -L${TARGETDIR}/usr/lib/${MULTI_ARCH_LIB}
#LIBS += -L${TARGETDIR}/lib
#LIBS += -L${TARGETDIR}/lib/${MULTI_ARCH_LIB}

include ${TOOLDIR}/rules/Rules.make.libs

#------------------------------------------------------------------------------------------------------
