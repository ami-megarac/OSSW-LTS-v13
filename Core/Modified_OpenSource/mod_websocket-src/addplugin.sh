#!/bin/sh

IMAGE_TREE=$1
LIGHTY_CONF="lighttpd.conf"

if [ -f $IMAGE_TREE/conf/$LIGHTY_CONF ] && [ -f $IMAGE_TREE/etc/defconfig/$LIGHTY_CONF ]
then
	grep -rwn "mod_websocket" $IMAGE_TREE/conf/$LIGHTY_CONF >>/dev/null
	
	if [ $? -ne 0 ]
	then 	
		echo "server.modules	+=	( \"mod_websocket\" )" >>$IMAGE_TREE/conf/$LIGHTY_CONF
		echo "server.modules    +=      ( \"mod_websocket\" )" >>$IMAGE_TREE/etc/defconfig/$LIGHTY_CONF
	fi
fi
