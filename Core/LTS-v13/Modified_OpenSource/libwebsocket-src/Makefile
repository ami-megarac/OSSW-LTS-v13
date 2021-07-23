#------- Comment/uncomment the following line to enable/disable debugging--------------
DEBUG = n

#---------------------- Change according to your files ----------------------------------
LIBRARY_NAME 	= libwebsocket

SRC     +=      communicate.c
SRC     +=      socket_list.c
SRC     +=      errors.c
SRC     +=      handshake.c

CFLAGS += -Wall -Wextra -Werror -DRUPIFY -std=gnu99
CFLAGS += -I${TARGETDIR}/usr/include
CFLAGS += -I${SPXINC}/websocket
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/encoding

CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/hostname

LIBS   += -L${SPXLIB}/unix -lunix
LIBS   += -L${SPXLIB}/encoding -lencoding

#----------------------------------------------------------------------------------------
include ${TOOLDIR}/rules/Rules.make.libs
