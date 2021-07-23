#-----------------------------------------------------------------------------
DEBUG = n
#-----------------------------------------------------------------------------

LIBRARY_NAME=mod_websocket
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------

SRC	+=	mod_websocket_base64.c
SRC     +=      mod_websocket.c
SRC     +=      mod_websocket_frame.c
SRC     +=      mod_websocket_handshake.c
SRC     +=      mod_websocket_sha1.c
SRC     +=      mod_websocket_socket.c
SRC     += 	cdservice.c

CFLAGS	+=	-DHAVE_CONFIG_H
CFLAGS	+=	-D_REENTRANT -D__EXTENSIONS__ -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGE_FILES -D_LARGEFILE64_SOURCE
CFLAGS	+=	-fPIC -DPIC


CFLAGS 	+= 	-I.
CFLAGS  +=	-I${SPXINC}/modhapi
CFLAGS 	+= 	-I${SPXINC}/session
CFLAGS 	+= 	-I${SPXINC}/ipmi
CFLAGS 	+= 	-I${SPXINC}/global
CFLAGS 	+= 	-I${SPXINC}/dbgout
CFLAGS  += 	-I${SPXINC}/token
CFLAGS 	+= 	-I${SPXINC}/libipmi
CFLAGS 	+= 	-I${SPXINC}/userm
CFLAGS 	+= 	-I${SPXINC}/webifc
CFLAGS	+=	-I${SPXINC}/kvm_vm_session
CFLAGS	+=	-I${SPXINC}/adviser
CFLAGS	+=	-I${SPXINC}/activedircfg
CFLAGS 	+=	-I${SPXINC}/unix
CFLAGS 	+=	-I${SPXINC}/advisercfg
CFLAGS	+=	-I${SPXINC}/vmedia
CFLAGS 	+=	-I${TARGETDIR}/usr/include
CFLAGS 	+=	-I${SPXINC}/network
CFLAGS 	+=	-I${SPXINC}/ncml
CFLAGS +=   -I${SPXINC}/hostname
CFLAGS +=   -I${SPXINC}/userm
CFLAGS +=   -I${SPXINC}/userprivilegepdk
CFLAGS +=   -I${SPXINC}/userprivilege
CFLAGS  +=  -I${SPXINC}/featuredefine
CFLAGS	+= 	-I${SPXINC}/Extendedpriv
CFLAGS += -I${SPXINC}/racsessioninfo

# make use of USB Gadget + NBD API for Media
ifeq "$(CONFIG_SPX_FEATURE_LIB_USB_GADGET)" "YES"
# $(info USB Gadget support is avilable in websocket)
CFLAGS += -I${SPXINC}/nbd
CFLAGS += -I${SPXINC}/usbgadget

LIBS   += -L${SPXLIB}/nbd -lnbd
LIBS   += -L${SPXLIB}/usbgadget -lusbgadget
LIBS   += -L${SPXLIB}/safesystem -lsafesystem
endif

#CFLAGS += 	DGNU_SOURCE
CFLAGS	+= 	-DHAVE_SOCKLEN_T

LIBS	+=	-L${SPXLIB}/token -ltoken
LIBS	+=	-L${SPXLIB}/vmedia -lvmedia
LIBS	+=	-L${SPXLIB}/network -lnetwork
LIBS	+=	-L${SPXLIB}/userm -luserm
LIBS	+=	-L${SPXLIB}/ipmi -lipmi
LIBS	+=	-L${SPXLIB}/ipmiparams -lipmiparams
LIBS	+=	-L${SPXLIB}/unix -lunix
LIBS	+=	-L${SPXLIB}/dbgout -ldbgout
LIBS	+=	-L${SPXLIB}/ncml -lncml
LIBS	+=	-L$(SPXLIB)/user_auth -luserauth
LIBS    +=  -L${SPXLIB}/featuredefine -lfeaturedefine
#LIBS    +=  -L${SPXLIB}/session -lsession

LIBS	+=	-lpthread
LIBS	+=	-ldl 
LIBS	+=	-rdynamic

include ${TOOLDIR}/rules/Rules.make.libs

#------------------------------------------------------------------------------
