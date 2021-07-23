/*
 * Copyright(c) 2010, Norio Kobota, All rights reserved.
 */
#include <string.h>
#include <math.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "dbgout.h"
#include "dlfcn.h"
#include<errno.h>
#include <sys/socket.h>
#include <netinet/in.h>    /* sockaddr_in structure */
#include <netdb.h>	   /* gethostbyname() */
#include    <ncml.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include    <vmedia_cfg.h>
#include "iusb.h"

#include "featuredef.h"
#include <semaphore.h>
#include <pthread.h>
#include "nwcfg.h"
#include "base.h"
#include "connections.h"
#include "fdevent.h"
#include "joblist.h"
#include "log.h"
#include "ncml.h"
#include "mod_websocket.h"
#include "mod_websocket_socket.h"
#include "apphead.h"
#include "sessioncfg.h"
#include "redirimage.h"
#include "token.h"
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/prctl.h>
#include <semaphore.h>
#include <pthread.h>

#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
#include "nbd.h"
#include "libusbgadget.h"
/* instance number allocated by gadget lib */
static int gadget_cd_instance = -1;
#endif

#ifdef	HAVE_PCRE_H
# include <pcre.h>
#endif	/* HAVE_PCRE_H */

#if defined (LIGHTTPD_VERSION_ID) && (LIGHTTPD_VERSION_ID >= (1 << 16 | 4 << 8 | 30))
# define	NETWORK_SSL_BACKEND_WRITE(a,b,c,d) \
    network_ssl_backend_write(a, b, c, d, MAX_WRITE_LIMIT)
# define	NETWORK_BACKEND_WRITE(a,b,c,d) \
    network_backend_write(a, b, c, d, MAX_WRITE_LIMIT)
#else
# define	NETWORK_SSL_BACKEND_WRITE(a,b,c,d) \
    network_ssl_backend_write(a, b, c, d)
# define	NETWORK_BACKEND_WRITE(a,b,c,d) \
    network_backend_write(a, b, c, d)
#endif

#define NCML_LIB "/usr/local/lib/libncml.so"
/* prototypes */
static int connect_backend(handler_ctx *);
static void disconnect_backend(handler_ctx *);
static void prepare_disconnect_client(handler_ctx *);
static handler_t handle_backend(server *, void *, int);
static handler_t mod_websocket_handle_subrequest(server *, connection *, void *);
int get_service_config(buffer *service, SERVICE_CONF_STRUCT *ServiceConfig);
int get_port(SERVICE_CONF_STRUCT ServiceConfig, buffer *port);
int cd_instance = 0; // usb device instance
int media_instance = 0; // thread instance number
#define OPCODE 2
typedef struct {
    int        targetfd;
    int        sockfd;
    SSL_CTX   *ssl_ctx;
    SSL       *ssl;
    int        hixie;
    int        hybi;
    int        media;
    char      *cin_buf;
    char      *cout_buf;
    char      *tin_buf;
    char      *tout_buf;
  int        cli_stopped;
  int        ser_stopped;
#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
  NBD_CTX nbd_ctx;
#endif
} webs_ctx;
webs_ctx *ws_ctx_cd;
/**
 * IVTP packet header
**/
typedef struct
{
	unsigned short		type;				/**< Type of the packet **/
	unsigned long		pkt_size;			/**< Size of the remaining portion of the packet */
	unsigned short		status;				/**< status field. used for sending response	 */

} __attribute__((packed)) ivtp_hdr_t;

#define IVTP_CONNECTION_FAILED				(0x003B)
#define MAX_LEN         11
#define MAX_INSTANCES	8
pthread_t WebSockThread[MAX_INSTANCES];

pthread_t ReadFromCliThread[MAX_INSTANCES];
pthread_t WriteToCliThread[MAX_INSTANCES];

extern CoreFeatures_T g_corefeatures;
webs_ctx WebSockCtx[MAX_INSTANCES];
#define INVALID_SOCKET  -1
#define SOCKET int
#define LOOPBACK "lo"
#define BOTH_INTERFACES "both"

#define BUFSIZE (65536 * 4)//(8000000 * 4)
#define DBUFSIZE (BUFSIZE * 3) / 4 - 20  //  (786432)/4 - 20   // 196608- 20 // 196588
#define SOCKET_ERROR -1
#define MAXLINE 80
#define MAX_WEBSOCK_HEADER_LEN 10
#define BUFSIZEWOHDR (BUFSIZE - MAX_WEBSOCK_HEADER_LEN)  //buffer without header

#define IUSB_HEADER_SIZE 61

#define SELECT_REMOTE_RESPONSE_TIMEOUT_SECS		20

#define ntohl64(p) \
    ((((uint64_t)((p)[7])) <<  0) + (((uint64_t)((p)[6])) <<  8) +\
     (((uint64_t)((p)[5])) << 16) + (((uint64_t)((p)[4])) << 24) +\
     (((uint64_t)((p)[3])) << 32) + (((uint64_t)((p)[2])) << 40) +\
     (((uint64_t)((p)[1])) << 48) + (((uint64_t)((p)[0])) << 56))

#define htonl64(p) \
    (uint64_t)((p & ((uint64_t)0xff <<  56)) >>  56) + (uint64_t)((p & ((uint64_t)0xff << 48)) >>  40) +\
    (uint64_t)((p & ((uint64_t)0xff << 40)) >> 24) + (uint64_t)((p & ((uint64_t)0xff << 32)) >> 8) +\
    (uint64_t)((p & ((uint64_t)0xff << 24)) << 8) + (uint64_t)((p & ((uint64_t)0xff << 16)) << 24) +\
    (uint64_t)((p & ((uint64_t)0xff << 8)) << 40) + (uint64_t)((p & ((uint64_t)0xff << 0)) << 56)

/*
  Find free slot for incoming connection
*/
int WSFindFreeSlot()
 {
	int Instances = -1;

	for (Instances = 0; Instances < MAX_INSTANCES; Instances++) {
		if (WebSockCtx[Instances].cin_buf == NULL)
			return Instances;
	}
	return -1;

}

/*
  get_portnum : Fetches port number for the given service
  return : returns port number on success, -1 on failure
*/
int get_portnum(SERVICE_CONF_STRUCT ServiceConfig) {
	int conf_port_num = 0;
	void *dl_ncmlhandle = NULL;
	int (*dl_getNotEditableData)(unsigned char *, int, unsigned char *);

	dl_ncmlhandle = dlopen(NCML_LIB, RTLD_LAZY);
	if (dl_ncmlhandle == NULL) {
		TCRIT("Problem in loading library:%s\n", dlerror());
		return -1;
	}

	dl_getNotEditableData = dlsym(dl_ncmlhandle, "getNotEditableData");
	if (dl_getNotEditableData == NULL) {
		TCRIT("No Symbol Found:%s\n", dlerror());
		dlclose(dl_ncmlhandle);
		return -1;
	}
	(*dl_getNotEditableData)(
			(unsigned char *) &(ServiceConfig.NonSecureAccessPort),
			sizeof(ServiceConfig.NonSecureAccessPort),
			(unsigned char *) &(conf_port_num));
	dlclose(dl_ncmlhandle);
	return conf_port_num;
}

/*
  HTTPS Session: Connect with server using ipv4 socket
*/
SOCKET 
WSInitializeClientv4(u_short PortNo, char *InterfaceName)
 {
	int error;
	struct addrinfo hints, *res, *r;
	char serv[6];
	int is_connected = 0;
	SOCKET sock = -1; /* Server Socket */
	int reuseaddr = 1;

	/* We need to find addresses to listen on. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	snprintf(serv, sizeof(serv), "%d", PortNo);

	if ((error = getaddrinfo(NULL, serv, &hints, &res))) {
		TCRIT("Couldn't get bind addresses: %s\n", gai_strerror(error));
		return -1;
	}

	if (res == NULL) {
		TCRIT("No addresses to bind to.\n");
		return -1;
	}

	/* Now bind sockets to an address */
	for (r = res; r != NULL; r = r->ai_next) {
		/*
		 * Check the Network Family before creating socket. we have to allow only IPv6
		 * family becuase it is possible to create the IPv4 socket instead of IPv6.
		 * So add proper validation here.
		 */
		if (r->ai_family != AF_INET)
			continue;

		if ((sock = socket(r->ai_family, r->ai_socktype, IPPROTO_TCP)) < 0) {
			TCRIT("Couldn't make socket");
			continue;
		}

		if (strcasecmp(InterfaceName, BOTH_INTERFACES) != 0) {
			if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, InterfaceName,
					strlen(InterfaceName)) == -1)
				TWARN("Setsockopt(SO_BINDTODEVICE) Failed for server socket\n");
		}

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))
				== -1)
			TWARN("Setsockopt(SO_REUSEADDR) Failed for server socket\n");

		/* Establish socket connection with (remote) server. */
		if (connect(sock, r->ai_addr, r->ai_addrlen) < 0) {
			TCRIT("Server connect failure %d\n", errno);
			close(sock);
			continue;
			//return INVALID_SOCKET;
		} else {
			is_connected = 1;
			break;
		}
	}
	freeaddrinfo(res);
	return (is_connected ? sock : INVALID_SOCKET);
}

/*
  HTTPS Session: Connect with server using ipv6socket
*/
int WSInitializeClientv6(u_short PortNo, char *InterfaceName) {
	int error;
	struct addrinfo hints, *res, *r;
	char serv[6];
	int is_connected = 0;
	int sock = -1; /* Server Socket */
	int reuseaddr = 1;

	/* We need to find addresses to listen on. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	snprintf(serv, sizeof(serv), "%d", PortNo);

	if ((error = getaddrinfo(NULL, serv, &hints, &res))) {
		TCRIT("\n Couldn't get bind addresses: %s\n", gai_strerror(error));
		return -1;
	}

	if (res == NULL) {
		TCRIT("\n No addresses to bind to.\n");
		return -1;
	}

	/* Now bind sockets to an address */
	for (r = res; r != NULL; r = r->ai_next) {
		/*
		 * Check the Network Family before creating socket. we have to allow only IPv6
		 * family becuase it is possible to create the IPv4 socket instead of IPv6.
		 * So add proper validation here.
		 */
		if (r->ai_family != AF_INET6)
			continue;
		if ((sock = socket(r->ai_family, r->ai_socktype, IPPROTO_TCP)) < 0) {
			TCRIT("\n Couldn't make socket");
			continue;
		}
		if (strcasecmp(InterfaceName, BOTH_INTERFACES) != 0) {
			if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, InterfaceName,
					strlen(InterfaceName)) == -1)
				TCRIT("\n Setsockopt(SO_BINDTODEVICE) Failed for server socket\n");
		}

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))
				== -1)
			TCRIT("\n Setsockopt(SO_REUSEADDR) Failed for server socket\n");

		/* Establish socket connection with (remote) server. */
		if (connect(sock, r->ai_addr, r->ai_addrlen) < 0) {
			TCRIT("\n Server connect failure %d\n", errno);
			close(sock);
			continue;
			//return INVALID_SOCKET;
		} else {
			is_connected = 1;
			break;
		}
	}
	freeaddrinfo(res);
	return (is_connected ? sock : INVALID_SOCKET);
}


/*
 HTTPS Session: Connect with server
 */
SOCKET WSInitializeClient(u_short PortNo, char *InterfaceName) {
	SOCKET sock; /* Server Socket */

	/* Create a socket to listening */
	if ((g_corefeatures.global_ipv6 == ENABLED) && (IsBMCNFSMode() != 1)) {
		sock = WSInitializeClientv6(PortNo, InterfaceName);
	} else {
		sock = WSInitializeClientv4(PortNo, InterfaceName);
	}
	return sock;
}

ssize_t ws_recv(webs_ctx *ctx, void *buf, size_t len) {
	if (ctx->ssl != NULL) {
		return SSL_read(ctx->ssl, buf, len);
	} else {
		return recv(ctx->sockfd, buf, len, 0);
	}
}

ssize_t ws_send(webs_ctx *ctx, const void *buf, size_t len) {
	if (ctx->ssl != NULL) {
		return SSL_write(ctx->ssl, buf, len);
	} else {
		return send(ctx->sockfd, buf, len, 0);
	}
}

int form_encode_websocket_hybi_hdr(uint64_t srclength, char *dest,
		size_t destsize, int *payload_offset) {
	unsigned long long srclen_b64,  len = 0;
	char opcode  = (char) ((OPCODE & 0x0F) | 0x80);

	if ((int) srclength <= 0) {
		return 0;
	}

	srclen_b64 = srclength;	 

	if (srclen_b64 <= 125) {
		dest[8] = opcode;
		dest[9] = (char) srclen_b64;
		*payload_offset = 2;
	} else if ((srclen_b64 > 125) && (srclen_b64 < 65536)) {
		dest[6] = opcode;
		dest[7] = (char) 126;
		*(u_short*) &(dest[8]) = htons(srclen_b64);
		*payload_offset = 4;
	} else {
		dest[0] = opcode;
		dest[1] = (char) 127;
		dest[2] = (char) (((srclen_b64 & ((uint64_t) 0xff << 56)) >> 56) & 0xFF);
		dest[3] = (char) (((srclen_b64 & ((uint64_t) 0xff << 48)) >> 48) & 0xFF);
		dest[4] = (char) (((srclen_b64 & ((uint64_t) 0xff << 40)) >> 40) & 0xFF);
		dest[5] = (char) (((srclen_b64 & ((uint64_t) 0xff << 32)) >> 32) & 0xFF);
		dest[6] = (char) (((srclen_b64 & ((uint64_t) 0xff << 24)) >> 24) & 0xFF);
		dest[7] = (char) (((srclen_b64 & ((uint64_t) 0xff << 16)) >> 16) & 0xFF);
		dest[8] = (char) (((srclen_b64 & ((uint64_t) 0xff << 8)) >> 8) & 0xFF);
		dest[9] = (char) (((srclen_b64 & ((uint64_t) 0xff << 0)) >> 0) & 0xFF);
		*payload_offset = 10;
	}

	if (srclength > (destsize - *payload_offset)) {
		TCRIT("\npacket is beyond targsize\n");
		return -1;
	}

	len = srclength;

	if ((int) len < 0) {
		return len;
	}

	return (len + MAX_WEBSOCK_HEADER_LEN);
}

int encode_websocket_hybi(u_char const *src, uint64_t srclength, char *dest,
		size_t destsize) {
	unsigned long long srclen_b64, payload_offset = 2, len = 0;

	if ((int) srclength <= 0) {
		return 0;
	}

	srclen_b64 = srclength;

	dest[0] = (char) ((OPCODE & 0x0F) | 0x80);

	if (srclen_b64 <= 125) {
		dest[1] = (char) srclen_b64;
		payload_offset = 2;
	} else if ((srclen_b64 > 125) && (srclen_b64 < 65536)) {
		dest[1] = (char) 126;
		*(u_short*) &(dest[2]) = htons(srclen_b64);
		payload_offset = 4;
	} else {
		dest[1] = (char) 127;
		dest[2] = (char) (((srclen_b64 & ((uint64_t) 0xff << 56)) >> 56) & 0xFF);
		dest[3] = (char) (((srclen_b64 & ((uint64_t) 0xff << 48)) >> 48) & 0xFF);
		dest[4] = (char) (((srclen_b64 & ((uint64_t) 0xff << 40)) >> 40) & 0xFF);
		dest[5] = (char) (((srclen_b64 & ((uint64_t) 0xff << 32)) >> 32) & 0xFF);
		dest[6] = (char) (((srclen_b64 & ((uint64_t) 0xff << 24)) >> 24) & 0xFF);
		dest[7] = (char) (((srclen_b64 & ((uint64_t) 0xff << 16)) >> 16) & 0xFF);
		dest[8] = (char) (((srclen_b64 & ((uint64_t) 0xff << 8)) >> 8) & 0xFF);
		dest[9] = (char) (((srclen_b64 & ((uint64_t) 0xff << 0)) >> 0) & 0xFF);
		payload_offset = 10;
	}

	if (srclength > (destsize - payload_offset)) {
		TCRIT("\npacket is beyond targsize\n");
		return -1;
	}

	memcpy((char *) dest + payload_offset, (char *) src, srclength);
	len = srclength;

	if ((int) len < 0) {
		return len;
	}

	return len + payload_offset;
}

int decode_websocket_hybi(unsigned char *src, uint64_t srclength,
                u_char *target, size_t targsize,
                unsigned int *opcode, unsigned int *left, int *current_fin)
 {
	unsigned char *full_frame = NULL, *frame_mask = NULL, *payload = NULL, cntstr[MAX_LEN] = {0};
	int masked = 0;
	int len, framecount = 0;
	unsigned long long remaining = 0;
	unsigned int dest_offset = 0, hdr_length = 0;
	unsigned long long i = 0, payload_length = 0;
	int retval = 0;

	*left = srclength;
	full_frame = src;
	if (0) {
		targsize = targsize;
	}

	while (1) {
		// Need at least two bytes of the header
		// Find beginning of next frame. First time hdr_length, masked and
		// payload_length are zero
		full_frame += hdr_length + 4 * masked + payload_length;

		if (full_frame > src + srclength) {
			break;
		}
		remaining = (src + srclength) - full_frame;
		if (remaining < 2) {
			break;
		}
		framecount++;

		*opcode = full_frame[0] & 0x0f;
		masked = (full_frame[1] & 0x80) >> 7;

		if (*opcode == 0x8) {
			break;
		}

		payload_length = full_frame[1] & 0x7f;
		if (payload_length < 126) {
			hdr_length = 2;
		} else if (payload_length == 126) {
			payload_length = (full_frame[2] << 8) + full_frame[3];
			hdr_length = 4;
		} else if (payload_length == 127) {
			payload_length = ntohl64(&full_frame[2]);
			hdr_length = 10;
		}
		if ((hdr_length + 4 * masked + payload_length) > remaining) {
			continue;
		}
		payload = full_frame + hdr_length + 4 * masked;

		// keep track of fin bit of current frame, so that we can know whether
		// the current frame is the last fragment or there will be continuation frames
		*current_fin = (full_frame[0]>>7);

		// ignore websocket opcodes other than 0(continuation frames),1(text),2(binary) and 0xa(pong)
		if ((*opcode > 2)) {
			if (*opcode != 0xa) {
				TCRIT("Ignoring non-data frame, opcode 0x%x\n", *opcode);
			}
			continue;
		}

		if (payload_length == 0) {
			continue;
		}

		if ((payload_length > 0) && (!masked)) {
			return -1;
		}

		// unmask the data
		frame_mask = payload - 4;
		for (i = 0; i < payload_length; i++) {
			payload[i] ^= frame_mask[i % 4];
			target[dest_offset + i] = payload[i];
		}
		len = payload_length;

		if (len < 0) {
			return len;
		}
		dest_offset += len;
	}

	if (framecount > 1) {
		retval = snprintf((char *) cntstr, sizeof(cntstr), "%d", framecount);
		if(retval < 0 || retval >= (signed)sizeof(cntstr))
		{
			TCRIT("Buffer Overflow\n");
			return -1;
		}
		TCRIT("cntstr :%s\n", (char *) cntstr);
	}

	*left = remaining;
	return dest_offset;
}

int cleanup_instance( int instance, int clear_ssl )
 {
	int ret = -1;

	if (instance < 0 || instance > MAX_INSTANCES)
		return ret;

	if (WebSockCtx[(int) instance].cin_buf != NULL) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		free(WebSockCtx[(int) instance].cin_buf);
		WebSockCtx[(int) instance].cin_buf = NULL; /* Fortify [Buffer Overflow]:: False Positive */
	}
	if (WebSockCtx[(int) instance].cout_buf != NULL) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		free(WebSockCtx[(int) instance].cout_buf);
		WebSockCtx[(int) instance].cout_buf = NULL; /* Fortify [Buffer Overflow]:: False Positive */
	}
	if (WebSockCtx[(int) instance].tin_buf != NULL) {
		free(WebSockCtx[(int) instance].tin_buf);
		WebSockCtx[(int) instance].tin_buf = NULL; /* Fortify [Buffer Overflow]:: False Positive */
	}
	if (WebSockCtx[(int) instance].tout_buf != NULL) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		free(WebSockCtx[(int) instance].tout_buf);
		WebSockCtx[(int) instance].tout_buf = NULL; /* Fortify [Buffer Overflow]:: False Positive */
	}

	if (WebSockCtx[(int) instance].targetfd > 0) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		shutdown(WebSockCtx[(int) instance].targetfd, SHUT_RDWR);
		close(WebSockCtx[(int) instance].targetfd);
		WebSockCtx[(int) instance].targetfd = -1;
	}
	if (WebSockCtx[(int) instance].sockfd > 0) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		shutdown(WebSockCtx[(int) instance].sockfd, SHUT_RDWR);
		close(WebSockCtx[(int) instance].sockfd);
		if(WebSockCtx[(int) instance].ssl_ctx != NULL)
			WebSockCtx[(int) instance].ssl_ctx = NULL;/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		if((clear_ssl == 1) && (WebSockCtx[(int) instance].ssl != NULL)) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
			SSL_free(WebSockCtx[(int) instance].ssl);
			WebSockCtx[(int) instance].ssl = NULL;
		}
		WebSockCtx[(int) instance].sockfd = -1;
	}
	WebSockCtx[(int) instance].media = 0; /* Fortify [Buffer Overflow]:: False Positive */
	/* To ensure proper cleanup, memset the websocket instance once the
	** cleanup is done. */
	memset(&WebSockCtx[instance], 0, sizeof(webs_ctx)); /*Fortify[Buffer Overflow] :: false positive*/
	return 0;
}

void NanoSleep(unsigned long nsec) {
    struct timespec delay = { nsec / 1000000000, nsec % 1000000000 };
    pselect(0, NULL, NULL, NULL, &delay, NULL);
}

/*
 * sendDataToServer: sends the given data to server socket
 * server: server socket
 * tout_buf: data to be sent
 * len: length of data.
 * 
 */
void sendDataToServer(int server, char *tout_buf, int len)
{
	int bytes = 0;
	unsigned int sout_start, sout_end;

	sout_start = 0;
	sout_end = len;
	while(1) {
		bytes = send(server, tout_buf, len, 0);
		if (bytes < 0) {
		    TCRIT("\nunable to send to cdserver: %d %s\n", errno, strerror(errno));
		    break;
		}
		sout_start += bytes;
		if (sout_start >= sout_end) {
			sout_start = sout_end = 0;
			break;
		}
	}
}

/*
 * sendDataToH5Viewer: sends the given data to h5viewer client
 * cin_buf: data to be send
 * bytes: data length
 * hybi: is websocket version hybi
 */
int sendDataToH5Viewer(char *cin_buf, int bytes, int hybi)
{
	int len = 0, cout_start = 0, cout_end = 0, start_point = 0;
	int ssl_err = 0;
	cout_start = 0;
	if (hybi) {
		cout_end = form_encode_websocket_hybi_hdr(bytes, cin_buf, BUFSIZE, &start_point);
	}
	if (cout_end < 0) {
		TCRIT("encoding error\n");
		fflush(stdout);
		return -1;
	}
	cout_start = MAX_WEBSOCK_HEADER_LEN - start_point;
	//Send data to client which is received from server
	{
		write_again: 
		len = cout_end - cout_start;
		if(len > 0)
		{
			bytes = ws_send(ws_ctx_cd, cin_buf + cout_start, len);
			if (bytes < 0) {
				if (ws_ctx_cd->ssl != NULL) {
					ssl_err = SSL_get_error(ws_ctx_cd->ssl, bytes);
					if ((errno == EAGAIN) && ((ssl_err == SSL_ERROR_WANT_WRITE) || (ssl_err == SSL_ERROR_SYSCALL)))
						goto write_again;
				}
				else {
					if (errno == EAGAIN) {
						goto write_again;
					}
					TCRIT("\n Non SSL send failed: errno == %d \n", errno);
				}
				return -1;
			}
			if (len < 3) {
				TCRIT("len: %d, bytes: %d: %d\n", (int) len, (int) bytes, (int) *(ws_ctx_cd->cout_buf + cout_start));
			}
			cout_start += bytes;
		}
		// if the whole data is not sent to H5Viewer, call write again
		if ((cout_end - cout_start)> 0)
		{
			goto write_again;
		}
		else
		{
			len = cout_start = cout_end = 0;
		}
	}
	return 0;
}

#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
/**
 * @brief Sends iUSB SCSI command to H5Viewer
 * 
 * @param req iUSB SCSI request packet
 * @return int 0 on success, -1 on failure
 */
static int sendSCSICmd(IUSB_SCSI_PACKET *req)
{
	int sendLen = -1;
	IUSB_SCSI_PACKET AckCommand;
	memset(&AckCommand, 0, sizeof(IUSB_SCSI_PACKET));
	// Note: IUSB_SIG used for signature purpose so it is not null terminated.
	// Its value is "IUSB" followed by 4 blank spaces. So using sizeof(IUSB_SIG)-1 for string copy.
	memcpy((char *)(AckCommand.Header.Signature), IUSB_SIG, sizeof(AckCommand.Header.Signature));
	AckCommand.CommandPkt.OpCode = req->CommandPkt.OpCode;

	if (req->CommandPkt.OpCode == 0xA8) // SCSI READ(12)
	{
		AckCommand.CommandPkt.Lba = req->CommandPkt.Lba;
		AckCommand.CommandPkt.CmdLen.Cmd12.Length32 = req->CommandPkt.CmdLen.Cmd12.Length32;
	}
	
	AckCommand.Header.DataPktLen = mac2long(sizeof(IUSB_SCSI_PACKET) - sizeof(IUSB_HEADER));

	sendLen = encode_websocket_hybi((const u_char *)&AckCommand, sizeof(IUSB_SCSI_PACKET), ws_ctx_cd->cout_buf, BUFSIZE);
	if (0 >= sendLen)
	{
		TCRIT("ERROR: Unable to encode AckCommand packet");
		return -1;
	}
	else
	{
		if (0 > ws_send(ws_ctx_cd, ws_ctx_cd->cout_buf, sendLen))
		{
			TCRIT("ERROR: Unable to send AckCommand packet to H5Viewer client");
			return -1;
		}
	}

	return 0;
}
#endif

/*
 * WebSCSIHndlr: SCSI handler for the cd_instance
 * reads data from cd usb instance and sends the data to H5viewer client
 * 
 */
void *WebSCSIHndlr() {
	pthread_t self;
#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
	IUSB_SCSI_PACKET req;
	NBD_CTX *ctx = &ws_ctx_cd->nbd_ctx;

	if (-1 != (gadget_cd_instance = cdrom_start(nbd_dev_list[ctx->instance])))
	{
		while (1)
		{
			if (ctx->state == NBD_WAIT_REQ)
			{
				if (nbd_read_req(ctx) != -1)
				{
					memset(&req, 0, sizeof(req));
					req.CommandPkt.OpCode = 0xA8;

					/* convert the nbd request into SCSI sectors request */
					req.CommandPkt.Lba = htobe32(ctx->offset < ctx->blocksize ? 0 :(uint32_t )( ( ( uint64_t )ctx->offset)  / ( (uint64_t)ctx->blocksize ) ) );
					req.CommandPkt.CmdLen.Cmd12.Length32 = (ctx->length < ctx->blocksize ? 1 :(uint32_t) ((uint32_t)ctx->length / (uint64_t)ctx->blocksize));

					// printf("\n[%s:%d] sec no:%u numsectors:%u\n", __FUNCTION__, __LINE__, (req.CommandPkt.Lba), (req.CommandPkt.CmdLen.Cmd12.Length32));fflush(stdout);

					sendSCSICmd(&req);

					ctx->state = NBD_WAIT_RES;
				}
				else
				{
					if (gadget_cd_instance != -1)
					{
						cdrom_stop(gadget_cd_instance);
						gadget_cd_instance = -1;
					}
					nbd_disconnect(ctx);
					break;
				}
			}
			NanoSleep(1);
		}
	}
	else
	{
		nbd_disconnect(ctx);
	}
#else
	IUSB_SCSI_PACKET *tmp_ReqPkt = NULL;
	u8* ioctl_data;
	int Retval = 0, cmd_len;
	int ret;
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
	/* Create communication Pipe between parent and thread */
	if(cd_instance != -1) {	
		Retval = requestUsbDev(cd_instance);
		if(Retval == 1)
		{
			TCRIT("WebSCSIHndlr, request usb dev failed\n");
			return NULL;
		}
		processActivate (cd_instance);
	}		
	while (cd_instance != -1) {

		ioctl_data = (u8*) addAuthInfo (USB_CDROM_REQ,(u8*)ws_ctx_cd->cout_buf + MAX_WEBSOCK_HEADER_LEN, cd_instance);

		Retval = ioctl(mod_usbfd,USB_CDROM_REQ,ioctl_data);
		if (Retval < 0) {
			break;
		}

		tmp_ReqPkt =(IUSB_SCSI_PACKET *) ( ws_ctx_cd->cout_buf + MAX_WEBSOCK_HEADER_LEN);
		cmd_len = mac2long(tmp_ReqPkt->Header.DataPktLen) + sizeof(IUSB_HEADER);			

		ret = sendDataToH5Viewer(ws_ctx_cd->cout_buf, cmd_len, ws_ctx_cd->hybi);
		if(ret!=0){
			break;
		}
	}
	//disconnect the thread
	tmp_ReqPkt = NULL;
	releaseUsbDev(cd_instance);
#endif
	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);
}

void *WebSocketProxy() {
	fd_set rdlist, exlist;
	struct timeval tv;
	int ret;
	int maxfd = 0;
	unsigned int opcode = 0, left = 0;
	unsigned int sout_end = 0;
	uint64_t sin_start = 0, sin_end = 0;
	int len = 0, bytes;
	int current_fin = -1;
	pthread_t self;
	int ssl_err = 0;
	IUSB_SCSI_PACKET *TempResPkt = NULL;
	DEV_REDIR_ACK *AckCommand = NULL;
	pthread_t mod_ChildId = 0;
	prctl(PR_SET_NAME, __FUNCTION__, 0, 0, 0);

	time_t h5PktRecvdTime = { 0 };
	struct sysinfo sys_info;
	double timedout = 0;
	maxfd = ws_ctx_cd->sockfd > ws_ctx_cd->targetfd ? ws_ctx_cd->sockfd + 1 : ws_ctx_cd->targetfd + 1;
#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
	int send_scsi_read_capacity = 0;
#endif
	while (1) {

		FD_ZERO(&rdlist);
		FD_ZERO(&exlist);
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_SET(ws_ctx_cd->targetfd, &exlist);
		FD_SET(ws_ctx_cd->targetfd, &rdlist);

		FD_SET(ws_ctx_cd->sockfd, &rdlist);
		FD_SET(ws_ctx_cd->sockfd, &exlist);

		ret = select(maxfd, &rdlist, NULL, &exlist, &tv);

		if (ret == -1) {
			TCRIT("select(): %s\n", strerror(errno));
			break;
		} else if (ret == 0) {
			continue;
		}

		// read fd is set for client, so read data from client socket
		if (ws_ctx_cd->sockfd != -1) {
			if (FD_ISSET(ws_ctx_cd->sockfd, &rdlist)) {
				read_again:
				//when full websocket frame is not received in single read
				//should read the socket directly without going for select()
				//removing "goto" jump will break huge data transfer

				bytes = (uint64_t) ws_recv(ws_ctx_cd, ws_ctx_cd->tin_buf + sin_end, BUFSIZE - 1);
				TempResPkt = (IUSB_SCSI_PACKET *) ws_ctx_cd->tout_buf;
				if (bytes <= 0) {

					if (ws_ctx_cd->ssl != NULL) {
						ssl_err = SSL_get_error(ws_ctx_cd->ssl, bytes);
						if ((ssl_err == SSL_ERROR_WANT_READ) || (ssl_err == SSL_ERROR_SYSCALL)) {
							goto read_again;
						}
						TCRIT("SSL client closed connection  sslerr:%d \n", SSL_get_error(ws_ctx_cd->ssl, bytes));
					} else {
						if (errno == EAGAIN) {
							goto read_again;
						}
						TCRIT("Non SSL client closed connection  err:%d \n", errno);
					}
					cleanup_instance(media_instance, 1);
					break;
				}

				sin_end += bytes;

				if (ws_ctx_cd->hybi) {
					len = decode_websocket_hybi((unsigned char *) ws_ctx_cd->tin_buf + sin_start, (uint64_t) sin_end - sin_start, (u_char *) ws_ctx_cd->tout_buf + sout_end, BUFSIZE - 1, &opcode, &left, &current_fin);
				}

				if (opcode == 8) {
					TCRIT("Client sent orderly close frame\n");
					ws_ctx_cd->sockfd = -1;
					cleanup_instance(media_instance, 1);
					break;
				}

				if (len < 0) {
					TCRIT("Decoding error\n");
					fflush(stdout);
				}

				if (left) {
					sin_start = sin_end - left;
				} else {
					sin_start = 0;
					sin_end = 0;
				}
				sout_end += len;

				//Need to read more data in order to complete the frame
				// again read the data from socket.
				if (left > 0) {
					goto read_again;
				}
				left = 0;
				TempResPkt = (IUSB_SCSI_PACKET *) ws_ctx_cd->tout_buf;
				if (TempResPkt->DataLen + IUSB_HEADER_SIZE != sout_end) {
					// If the fin is 0 then it means it is not the end frame. And more frames will be received as continuation frames.
					// If the fin value is 1 then it means it is the end frame.
					if (current_fin == 0) {
						// Since there are multiple packets sent simultaneously in case of continuation frames
						// so we can directly go for socket read without checking for socket event.
						// Incase of continuation frames, the data should be read from the socket
						// till we receive the end of the frames denoting the completion of continuation frames
						// so that we don't miss out any packets in the socket
						goto read_again;
					}
				}

				//Send data to server or USB driver which is received from client
				{
					// based on the opcode received from the H5Viewer client, send AMI USB SCSI opcode packets to CD server and
					// common USB SCSI opcode packets to USB driver directly.
					if (TempResPkt->CommandPkt.OpCode != MEDIA_SESSION_DISCONNECT && TempResPkt->CommandPkt.OpCode != AUTH_CMD && TempResPkt->CommandPkt.OpCode != DEVICE_INFO && TempResPkt->CommandPkt.OpCode != CDROM_KEEPALIVE_SCSI_CMD) {
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
						sendDataToDriver(cd_instance, ws_ctx_cd->tout_buf + 0);
#else
						if (TempResPkt->CommandPkt.OpCode == 0x25) // SCSI Read Capacity
						{
							READ_CAPACITY *r = (READ_CAPACITY *)&TempResPkt->Data;
							r->BlockSize = ntohl(r->BlockSize);
							r->TotalBlocks = ntohl(r->TotalBlocks);

							// printf("\n[%s:%d] b:%u tb:%u\n", __FUNCTION__, __LINE__, r->BlockSize, r->TotalBlocks);fflush(stdout);

							ws_ctx_cd->nbd_ctx.blocksize = r->BlockSize;
							/* Adding +1 to include last sector. Otherwise checksum mismatch will happen */
							ws_ctx_cd->nbd_ctx.totalblocks = r->TotalBlocks + 1;
							ws_ctx_cd->nbd_ctx.flags = NBD_FLAG_RO; // CDROM always readonly

							if(nbd_init(&ws_ctx_cd->nbd_ctx) != -1)
							{
								ws_ctx_cd->nbd_ctx.state = NBD_WAIT_REQ;

								if (0 != pthread_create(&mod_ChildId, NULL, WebSCSIHndlr, (void *)NULL))
								{
									nbd_disconnect(&ws_ctx_cd->nbd_ctx);
									sendCmdToCdserver(ws_ctx_cd->targetfd, MEDIA_SESSION_DISCONNECT, cd_instance);
									break;
								}
							}
							else
							{
								break;
							}
						}
						else if (TempResPkt->CommandPkt.OpCode == 0xA8) //SCSI Read(12)
						{
							ws_ctx_cd->nbd_ctx.data = (void *)&TempResPkt->Data;

							if (nbd_write_res(&ws_ctx_cd->nbd_ctx) != -1)
							{
								ws_ctx_cd->nbd_ctx.state = NBD_WAIT_REQ;
							}
							else
							{
								if (gadget_cd_instance != -1)
								{
									cdrom_stop(gadget_cd_instance);
									gadget_cd_instance = -1;
								}
								nbd_disconnect(&ws_ctx_cd->nbd_ctx);
								sendCmdToCdserver(ws_ctx_cd->targetfd, MEDIA_SESSION_DISCONNECT, cd_instance);
							}
						}
#endif
						// update h5viewer packet received time
						if (!sysinfo(&sys_info)) {
							h5PktRecvdTime = sys_info.uptime;
						}

					} else {
						sendDataToServer(ws_ctx_cd->targetfd, ws_ctx_cd->tout_buf, len);

						if (TempResPkt->CommandPkt.OpCode == MEDIA_SESSION_DISCONNECT) {
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
							processDisconnect(cd_instance);
#endif
							pthread_cancel(mod_ChildId);
							pthread_join(mod_ChildId, NULL);
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
							releaseUsbDev(cd_instance);
#else
							if (gadget_cd_instance != -1)
							{
								cdrom_stop(gadget_cd_instance);
								gadget_cd_instance = -1;
							}
							nbd_disconnect(&ws_ctx_cd->nbd_ctx);
#endif
							cleanup_instance(media_instance, 1);
							break;
						}
					}
					sout_end = 0;
					sin_start = 0;
					sin_end = 0;
				}
			}
		}
		// read fd is set for server, so read data from server socket
		if (FD_ISSET(ws_ctx_cd->targetfd, &rdlist)) {
			bytes = recv(ws_ctx_cd->targetfd, ws_ctx_cd->cin_buf + MAX_WEBSOCK_HEADER_LEN, BUFSIZEWOHDR, 0);
			if (bytes <= 0) {
				TCRIT("\ntarget closed connection  errno:%d\n", errno);
				ws_ctx_cd->targetfd = -1;
				break;
			}
			AckCommand = (DEV_REDIR_ACK *) (ws_ctx_cd->cin_buf + MAX_WEBSOCK_HEADER_LEN);
			if (AckCommand->iUsbScsiPkt.CommandPkt.OpCode == DEVICE_REDIRECTION_ACK) {
				if( (AckCommand->ConnectionStatus == CONNECTION_ACCEPTED) ||
					(AckCommand->ConnectionStatus == CONNECTION_ACCEPTED_WITH_MEDIA_BOOST ) ||
					(AckCommand->ConnectionStatus == CONNECTION_ACCEPTED_WITH_OUT_MEDIA_BOOST ) )
				{
					cd_instance = AckCommand->iUsbScsiPkt.Header.Instance;

#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
					if (0 != pthread_create(&mod_ChildId, NULL, WebSCSIHndlr, (void *) NULL)) {
						break;
					}
#else
					send_scsi_read_capacity = 1;
#endif

				}
				else
				{
					sendDataToH5Viewer(ws_ctx_cd->cin_buf, bytes, ws_ctx_cd->hybi);
					break; //cdserver rejects incoming connection !!
				}
			}
			//Based on lastpacket received time out send Keepalive pkt to cdserver/remote client
			//KEEP ALIVE MECHANISM:
			// When cdserver sends keep alive, check if there is data transfer between h5viewer and usb driver. (i.e based on h5PktRecvdTime)
			// If there is data transfer between h5viewer and usb driver, no need to send and wait for response from h5viewer.
			// so mod_websocket will send the keepalive response to the cdserver by itself.
			// If there is no data transfer between h5viewer and usb driver, then mod_websocket will send the keepalive
			// received from cdserver to h5viewer. and h5viewer will reply to the keep alive.
			if (AckCommand->iUsbScsiPkt.CommandPkt.OpCode == CDROM_KEEPALIVE_SCSI_CMD) {
				if (!sysinfo(&sys_info)) {
					timedout = difftime(sys_info.uptime, h5PktRecvdTime);
					if (timedout > (SELECT_REMOTE_RESPONSE_TIMEOUT_SECS)) {
						sendDataToH5Viewer(ws_ctx_cd->cin_buf, bytes, ws_ctx_cd->hybi);
						h5PktRecvdTime = 0;
					} else {
						sendCmdToCdserver(ws_ctx_cd->targetfd, CDROM_KEEPALIVE_SCSI_CMD, cd_instance);
						h5PktRecvdTime = 0;
					}

				}
			} else {
				sendDataToH5Viewer(ws_ctx_cd->cin_buf, bytes, ws_ctx_cd->hybi);
#ifdef CONFIG_SPX_FEATURE_LIB_USB_GADGET
				if (send_scsi_read_capacity == 1)
				{
					IUSB_SCSI_PACKET req;
					memset(&req, 0, sizeof(req));
					req.CommandPkt.OpCode = 0x25;
					sendSCSICmd(&req);

					send_scsi_read_capacity = 0;
				}
#endif
			}
		}
		//while Reconnecting network back sending disconnect cmd based on opcode  value 
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
		if (opcode == 8) {
#else
		if ((opcode == 8) || ((gadget_cd_instance != -1) && (cdrom_eject_status(gadget_cd_instance) == 1))) {

			if ((gadget_cd_instance != -1) && (cdrom_eject_status(gadget_cd_instance) == 1))
			{
				IUSB_SCSI_PACKET req;
				memset(&req, 0, sizeof(req));
				req.CommandPkt.OpCode = MEDIA_SESSION_DISCONNECT;
				sendSCSICmd(&req);
			}
#endif
			sendCmdToCdserver(ws_ctx_cd->targetfd, MEDIA_SESSION_DISCONNECT, cd_instance);
			pthread_cancel(mod_ChildId);
			pthread_join(mod_ChildId, NULL);
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
			releaseUsbDev(cd_instance);
#else
			if (gadget_cd_instance != -1)
			{
				cdrom_stop(gadget_cd_instance);
				gadget_cd_instance = -1;
			}
			nbd_disconnect(&ws_ctx_cd->nbd_ctx);
#endif
			cleanup_instance(media_instance, 1);
		}
		// Add sleep to make CPU happy.
		// Without this sleep, the thread might shoot up the CPU usage if the server runs faster than the kvm client
		if ((FD_ISSET(ws_ctx_cd->sockfd, &rdlist)) || (FD_ISSET(ws_ctx_cd->targetfd, &rdlist))) {
			NanoSleep(1);
		}
	}
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
	// disconnect the thread
	if (mod_usbfd >= 0) {
		close(mod_usbfd);
		mod_usbfd = -1;
	}
#endif
	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);

	// exit
	return NULL;

}

void *ReadFromWebClient(void *instance) {
	fd_set rdlist,  exlist;
	struct timeval tv;
	webs_ctx *ws_ctx = &(WebSockCtx[(int) instance]);
	int ret, client = ws_ctx->sockfd;
	int server = 0;
	unsigned int opcode, left;
	unsigned int sout_start, sout_end;
	uint64_t sin_start, sin_end;
	int len, bytes, cout_start, cout_end, maxfd;
	int current_fin = -1;
	pthread_t self;

	len = sout_start = sout_end = cout_start = cout_end = 0 ;
	sin_start = sin_end = 0;

	server = ws_ctx->targetfd;
	ws_ctx->hybi = 1;
	maxfd = client + 1 ;

	while (1) {
	  if(ws_ctx->ser_stopped == 1)
	    break;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO(&rdlist);
		FD_ZERO(&exlist);

		FD_SET(client, &exlist);

		if (sout_end == sout_start) {
			// nothing is left for server , so read from client
			FD_SET(client, &rdlist);
		}


		ret = select(maxfd, &rdlist, NULL, &exlist, &tv);
		if (ret <= 0) {
			if (errno != 0) {
				if(errno != EAGAIN) {
					TCRIT("\nWebSocketProxy: select failed ret:%d err:%d :%s\n", ret, errno, strerror(errno));
					break;
				}
			}
		}

		if (FD_ISSET(client, &exlist)) {
			TCRIT("\nclient exception occurred\n");
			break;
		}

		if (ret == -1) {//select error
			TCRIT("select(): %s\n", strerror(errno));
			break;
		} else if (ret == 0) {//select timedout
				continue;
		}
		// read fd is set for client, so read data from client socket
		if (FD_ISSET(client, &rdlist)) {
			read_again:
			//when full websocket frame is not received in single read
			//should read the socket directly without going for select()
			//removing "goto" jump will break huge data transfer

			bytes = (uint64_t) ws_recv(ws_ctx, ws_ctx->tin_buf + sin_end, BUFSIZE - 1);
			if (bytes <= 0) {

				if (ws_ctx->ssl != NULL) {
					if ((SSL_get_error(ws_ctx->ssl, bytes) == SSL_ERROR_WANT_READ) || 
							(SSL_get_error(ws_ctx->ssl, bytes) == SSL_ERROR_SYSCALL)) {
						goto read_again;
					}
					TCRIT("\n SSL client closed connection  sslerr:%d \n", SSL_get_error(ws_ctx->ssl, bytes));
				}
				else{
					if (errno == EAGAIN) {
						goto read_again;
					}
					TCRIT("\n 222 Non SSL client closed connection  err:%d \n", errno);
				}
				break;
			}

			sin_end += bytes;

			if (ws_ctx->hybi) {
				len = decode_websocket_hybi((unsigned char *) ws_ctx->tin_buf + sin_start, (uint64_t) sin_end - sin_start, (u_char *) ws_ctx->tout_buf, BUFSIZE - 1, &opcode, &left, &current_fin);
			}

			if (opcode == 8) {
				TCRIT("\nclient sent orderly close frame\n");
				break;
			}

			if (len < 0) {
				TCRIT("\ndecoding error\n");
				fflush(stdout);
			}

			if (left) {
				sin_start = sin_end - left;
			} else {
				sin_start = 0;
				sin_end = 0;
			}

			sout_start = 0;
			sout_end = len;

			//Need to read more data inorder to complete the frame
			// again read the data from socket.
			if (left > 0) {
				goto read_again;
			}
			//Send data to server which is received from client
			{
				len = sout_end - sout_start;
				bytes = send(server, ws_ctx->tout_buf + sout_start, len, 0);
				if (bytes < 0) {
					TCRIT("\ntarget connection error: %s\n", strerror(errno));
					break;
				}
				sout_start += bytes;
				if (sout_start >= sout_end) {
					sout_start = sout_end = 0;
				}
			}
			// If the fin is 0 then it means it is not the end frame. And more frames will be received as continuation frames.
			// If the fin value is 1 then it means it is the end frame.
			if (current_fin == 0) {
				// Since there are multiple packets sent simultaneously in case of continuation frames
				// so we can directly go for socket read without checking for socket event.
				// Incase of continuation frames, the data should be read from the socket
				// till we receive the end of the frames denoting the completion of continuation frames
				// so that we don't miss out any packets in the socket
				goto read_again;
			}
		}

		// NOTE : Sleep is not needed here since the data sent from kvm client to video server will be less when compared to other way.
		// If high cpu usage issue occurs, this needs to change.
		// read fd is set for server, so read data from server socket
		// Add sleep to make CPU happy.
		// Without this sleep, the thread might shoot up the CPU usage if the server runs faster than the kvm client
		
		//		if(ismedia != 1){
		//		NanoSleep(1);
		//	}
	}

	if(ws_ctx->ser_stopped == 1)
	{
	  // cleanup resources
	    cleanup_instance((int) instance, 1);
	}
	else
	{
	  ws_ctx->cli_stopped = 1;
	  if(ws_ctx->sockfd != -1)
	    shutdown(ws_ctx->sockfd, SHUT_RDWR);
	}
	// disconnect the thread
	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);

	// exit
	return NULL;
}



void *WritetoWebClient(void *instance) {
	fd_set rdlist,  exlist;
	struct timeval tv;
	webs_ctx *ws_ctx = &(WebSockCtx[(int) instance]);
	int ret;//, client = ws_ctx->sockfd;
	int server = 0;
	unsigned int sout_start, sout_end;
	int len, bytes, cout_start, cout_end, maxfd,start_point;
	pthread_t self;
	int ssl_err = 0;

	len = sout_start = sout_end = cout_start = cout_end = 0 ,start_point=0;


	server = ws_ctx->targetfd;
	ws_ctx->hybi = 1;
	maxfd = server + 1;

	while (1) {
	  if(ws_ctx->cli_stopped == 1)
	    break;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO(&rdlist);
		FD_ZERO(&exlist);

		FD_SET(server, &exlist);

		if (cout_end == cout_start) {
			// nothing is left for client, so read from server
			FD_SET(server, &rdlist);
		}

		ret = select(maxfd, &rdlist, NULL, &exlist, &tv);
		if (ret <= 0) {
			if (errno != 0) {
				if(errno != EAGAIN) {
					TCRIT("\nWebSocketProxy: select failed ret:%d err:%d :%s\n", ret, errno, strerror(errno));
					break;
				}
			}
		}

		if (FD_ISSET(server, &exlist)) {
			TCRIT("\ntarget exception occurred\n");
			break;
		}

		if (ret == -1) {
			TCRIT("select(): %s\n", strerror(errno));
			break;
		} else if (ret == 0) {
			//TCRIT("\nselect timedout\n");
			continue;
		}

		// read fd is set for server, so read data from server socket
		if (FD_ISSET(server, &rdlist)) {
			bytes = recv(server, ws_ctx->cin_buf + MAX_WEBSOCK_HEADER_LEN, BUFSIZEWOHDR, 0);
			if (bytes <= 0) {
				TCRIT("\ntarget closed connection  errno:%d\n", errno);
				break;
			}

			cout_start = 0;
			if (ws_ctx->hybi) {
				cout_end = form_encode_websocket_hybi_hdr(bytes, ws_ctx->cin_buf, BUFSIZE, &start_point);
			}
			if (cout_end < 0) {
				TCRIT("encoding error\n");
				fflush(stdout);
				break;
			}
			cout_start = MAX_WEBSOCK_HEADER_LEN - start_point;
			//Send data to client which is received from server
			{
				WebClient_write_again: 
				len = cout_end - cout_start;
				if(len > 0)
				{
					bytes = ws_send(ws_ctx, ws_ctx->cin_buf + cout_start, len);
					if (bytes < 0) {
						if (ws_ctx->ssl != NULL) {
							ssl_err = SSL_get_error(ws_ctx->ssl, bytes);
							if ((errno == EAGAIN) && ((ssl_err == SSL_ERROR_WANT_WRITE) || (ssl_err == SSL_ERROR_SYSCALL))){
								goto WebClient_write_again;
							}
							TCRIT("\n WritetoWebClient read failed: errno == %d\t %d\n", errno,	SSL_get_error(ws_ctx->ssl, bytes));
						}
						else{
							if (errno == EAGAIN) {
								goto WebClient_write_again;
							}
							TCRIT("\n Non SSL read failed: errno == %d \n", errno);
						}
						break;
					}
					if (len < 3) {
						TCRIT("len: %d, bytes: %d: %d\n", (int) len, (int) bytes, (int) *(ws_ctx->cout_buf + cout_start));
					}
					cout_start += bytes;
				}
				// if the whole data is not sent to H5Viewer, call write again
				if ((cout_end - cout_start)> 0)
				{
					goto WebClient_write_again;
				}
				else
				{
					len = cout_start = cout_end = 0;
				}
			}
		}

		// NOTE : Sleep is not needed here since the data sent from video server to video client is restricted based on the number of frames
		// the kvm client can process in given time.
		// If high cpu usage issue occurs, this needs to change.
		// Add sleep to make CPU happy.
		// Without this sleep, the thread might shoot up the CPU usage if the server runs faster than the kvm client
		//		if(ismedia != 1){
			//NanoSleep(1);
		//}
	}

	
	if(ws_ctx->cli_stopped == 1)
	{
	  // cleanup resources
	    cleanup_instance((int) instance, 1);
	}
	else
	{
	  ws_ctx->ser_stopped = 1;
	  if(ws_ctx->targetfd != -1)
	    shutdown(ws_ctx->targetfd, SHUT_RDWR);
	}

	// disconnect the thread
	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);

	// exit
	return NULL;
}



/*
  Creating threads for HTTPS session connection
*/
int ProcessWebThreads(int thread_no) 
{
	// dual thread for video and single thread for media.
	// video transfer is made dual thread to solve the window drag issue
	// and to give equal priority to packets to and fro.
	// upon running video in the host, the mouse packets from the client will be sent with huge delay
	// dual thread fixes this issue.
	if(WebSockCtx[thread_no].media == 0)
	{
		if (0 != pthread_create(&ReadFromCliThread[thread_no], NULL, ReadFromWebClient,
				(void *) thread_no)) {
			TCRIT("\nUnable to Create write_thread  no:%d \n", thread_no);
			return -1;
		}

		if (0 != pthread_create(&WriteToCliThread[thread_no], NULL, WritetoWebClient,
				(void *) thread_no)) {
			TCRIT("\nUnable to Create write_thread  no:%d \n", thread_no);
			return -1;
		}
		
	}
	else {
		ws_ctx_cd = &WebSockCtx[thread_no];
		media_instance = thread_no;
#ifndef CONFIG_SPX_FEATURE_LIB_USB_GADGET
		if (mod_usbfd <= 0) {
			/* Open the USB Device */
			mod_usbfd = open(USB_DEVICE, O_RDWR);
			if (mod_usbfd < 0) {
				TCRIT("Error in opening USB Device\n");
				return 1;
			}

		}
#endif
		if (0 != pthread_create(&WebSockThread[thread_no], NULL, WebSocketProxy, NULL)) {
			TCRIT("\nUnable to Create WebSockThread  no:%d \n", thread_no);
			return -1;
		}
	}
	return 0;
}

int ServiceWebSockCmd(int Instance, buffer* uri)
 {

	SERVICE_CONF_STRUCT ServiceConfig;
	int port = 0, ret = -1;

	if (Instance < 0 || Instance > MAX_INSTANCES) {
		TCRIT("ServiceWebSockCmd: Invalid instance number = %d", Instance);
		return ret;
	}

	if (uri == NULL) {
		TCRIT("ServiceWebSockCmd: uri is NULL");
		return ret;
	}

	ret = get_service_config(uri, &ServiceConfig);

	if ((ret != ERR_LOAD_DEFCONF) && (ret != ERR_GET_DEFCONF) && (ret
			!= ERR_GET_CONF)) //ERR_SET_CONF doesn't affect service running.
	{
		port = get_portnum(ServiceConfig);
		if (port < 0) {
			TCRIT("ServiceWebSockCmd: Invalid port number = %d", port);
			return ret;
		}

		WebSockCtx[Instance].targetfd = WSInitializeClient(port, LOOPBACK); /* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		/* If media / video server port not listening */
		if (WebSockCtx[Instance].targetfd <= 0) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
			TCRIT("ServiceWebSockCmd: Invalid targetfd");

			/* In case of H5Viewer KVM redir, we will always assume abrupt socket close as reconnect case.
			** So in this (target server not running) case, for proper KVM redir status update in H5Viewer
			** we need to sent IVTP_CONNECTION_FAILED packet (from webserver to H5Viewer client), denoting
			** the port not listening status.
			**/ 
			if (0 == strncmp(ServiceConfig.ServiceName, KVM_SERVICE_NAME, strlen(KVM_SERVICE_NAME)))
			{
				int sendLen = -1;
				ivtp_hdr_t failure;
				memset(&failure, 0, sizeof(ivtp_hdr_t));
				failure.type =IVTP_CONNECTION_FAILED;
				failure.pkt_size = 0;
				failure.status 	= 0;
				sendLen = encode_websocket_hybi((const u_char *)&failure, sizeof(ivtp_hdr_t), WebSockCtx[Instance].cout_buf, BUFSIZE);/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
				if(0 >= sendLen){
					TCRIT("ERROR: Unable to encode IVTP_CONNECTION_FAILED packet");
				}
				else
				{
					if(0 > ws_send(&WebSockCtx[Instance], WebSockCtx[Instance].cout_buf, sendLen)){/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
						TCRIT("ERROR: Unable to send IVTP_CONNECTION_FAILED packet to H5Viewer client");
					}
				}
			}

			/* In case of H5Viewer media redir, we will always assume abrupt socket close as reconnect case.
			** So in this (target server not running) case, for proper media redir status update in H5Viewer
			** we need to sent a iUSB packet (from webserver to H5Viewer client), denoting the port not listening status. */
			if (0 == strncmp(ServiceConfig.ServiceName, CDMEDIA_SERVICE_NAME, strlen(CDMEDIA_SERVICE_NAME)))
			{
				int sendLen = -1;
				DEV_REDIR_ACK AckCommand;
				memset(&AckCommand, 0, sizeof(DEV_REDIR_ACK));
				// Note: IUSB_SIG used for signature purpose so it is not null terminated.
				// Its value is "IUSB" followed by 4 blank spaces. So using sizeof(IUSB_SIG)-1 for string copy.
				memcpy((char *)(AckCommand.iUsbScsiPkt.Header.Signature), IUSB_SIG, sizeof(AckCommand.iUsbScsiPkt.Header.Signature));
				AckCommand.iUsbScsiPkt.CommandPkt.OpCode = DEVICE_REDIRECTION_ACK;
				AckCommand.iUsbScsiPkt.Header.DataPktLen = mac2long(sizeof(DEV_REDIR_ACK) - sizeof(IUSB_HEADER));
				AckCommand.ConnectionStatus = UNABLE_TO_CONNECT;
				sendLen = encode_websocket_hybi((const u_char *)&AckCommand, sizeof(DEV_REDIR_ACK), WebSockCtx[Instance].cout_buf, BUFSIZE);/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
				if(0 >= sendLen){
					TCRIT("ERROR: Unable to encode AckCommand packet");
				}
				else
				{
					if(0 > ws_send(&WebSockCtx[Instance], WebSockCtx[Instance].cout_buf, sendLen)){/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
						TCRIT("ERROR: Unable to send AckCommand packet to H5Viewer client");
					}
				}
			}
			cleanup_instance(Instance, 0);
			return INVALID_SOCKET; // Return INVALID_SOCKET here so that socket will be closed by webserver for the incoming connection.
		}
	}

	WebSockCtx[Instance].media = (0 == strncmp(ServiceConfig.ServiceName, CDMEDIA_SERVICE_NAME, strlen(CDMEDIA_SERVICE_NAME))) ? 1 :0; /* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/

	if (ProcessWebThreads(Instance) != 0) {/* SCA Fix [Memory - illegal accesses]:: False Positive *//* Reason for False Positive - Instance variable contains less than WebSockCtx arry index MAX_INSTANCES(8)*/
		TCRIT("ServiceWebSockCmd: Processwebthreads threads creation failed");
		return ret;
	}

	return 0;
}

/* OK */
static handler_ctx *handler_ctx_init(void) {
    handler_ctx *hctx = calloc(1, sizeof(handler_ctx));

    if (!hctx) {
        return NULL;
    }
    hctx->state = MOD_WEBSOCKET_STATE_INIT;
    hctx->mode = MOD_WEBSOCKET_TCP_PROXY;

    hctx->handshake.host = NULL;
    hctx->handshake.origin = NULL;
    hctx->handshake.version = -1;

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
    hctx->handshake.key1 = NULL;
    hctx->handshake.key2 = NULL;
    hctx->handshake.key3 = buffer_init();
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

#ifdef	_MOD_WEBSOCKET_RFC_6455_
    hctx->handshake.key = NULL;
#endif	/* _MOD_WEBSOCKET_RFC_6455_ */

    hctx->frame.state = MOD_WEBSOCKET_FRAME_STATE_INIT;
    hctx->frame.ctl.siz = 0;
    hctx->frame.type = MOD_WEBSOCKET_FRAME_TYPE_CLOSE;
    hctx->frame.type_before = MOD_WEBSOCKET_FRAME_TYPE_CLOSE;
    hctx->frame.type_backend = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
    hctx->frame.payload = buffer_init();
    hctx->tosrv = chunkqueue_init();

    hctx->fd = -1;
    hctx->fd_idx = -1;

    hctx->srv = NULL;
    hctx->con = NULL;
    hctx->ext = NULL;
    hctx->pd = NULL;
    hctx->fromcli = NULL;
    hctx->tocli = NULL;

    return hctx;
}

/* OK */
static void handler_ctx_free(handler_ctx *hctx) {
    if (!hctx) {
        return;
    }

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
    buffer_free(hctx->handshake.key3);
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

    buffer_free(hctx->frame.payload);
    chunkqueue_free(hctx->tosrv);
    free(hctx);
    return;
}

/*
 * @brief Gets the ServiceConfig structure for the specified service.
 * @parameter service - The buffer which holds the requesting service name.
 * @parameter ServiceConfig - The Service Configuration structure to which the 
 * 								Service Configuration data should be retrieved.
 * 
 * @return returns 0 - if port number is copied properly.
 * 				   error code(< 0) - Otherwise. 
 */
int get_service_config(buffer *service, SERVICE_CONF_STRUCT *ServiceConfig){
    int ret = 0;
    void *dl_servicehandle = NULL;
    int (*dl_get_service_conf)(char *,SERVICE_CONF_STRUCT *);

    dl_servicehandle = dlopen(NCML_LIB,RTLD_LAZY);
    if(dl_servicehandle == NULL)
    {
        TCRIT("Problem in loading library:%s\n",dlerror());
        return ERR_GET_CONF;
    }

    dl_get_service_conf = dlsym(dl_servicehandle,"get_service_configurations");
    if(dl_get_service_conf == NULL)
    {
        TCRIT("No Symbol Found:%s\n",dlerror());
        dlclose(dl_servicehandle);
        return ERR_GET_CONF;
    }

    //KVM
    if(buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_KVM)) || buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_KVM_URI))){
        ret = (*dl_get_service_conf)(KVM_SERVICE_NAME, ServiceConfig);
    }
    //CD
    else if(buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_CDSERVER)) || buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_CDSERVER_URI))){
        ret = (*dl_get_service_conf)(CDMEDIA_SERVICE_NAME, ServiceConfig);
    }
    //HDD
    else if(buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_HDDSERVER)) || buffer_is_equal_string(service, CONST_STR_LEN(MOD_WEBSOCKET_HDDSERVER_URI))){
        ret = (*dl_get_service_conf)(HDMEDIA_SERVICE_NAME, ServiceConfig);
    }
    else{
        ret = ERR_GET_CONF;
    }
    dlclose(dl_servicehandle);
    return ret;
}
/*
 * @brief Gets the Non-Secure port number from the ServiceConfig structure data.
 * @parameter ServiceConfig - The Service Configuration structure from which the 
 * 								Non-Secure port number should be retrieved.
 * @parameter port - The buffer into which the Non-Secure port number should be copied.
 * 				
 * @return returns 0 - if port number is copied properly.
 * 				   -1 - Otherwise. 
 */
int get_port(SERVICE_CONF_STRUCT ServiceConfig, buffer *port){
    int port_num = 0;
    int conf_port_num = 0;
    void *dl_ncmlhandle = NULL;
    int (*dl_getNotEditableData)(unsigned char *, int, unsigned char *);
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35
    int ret = -1;
#endif
    if(NULL != port->ptr)
        port_num = atoi(port->ptr);
    dl_ncmlhandle = dlopen(NCML_LIB,RTLD_LAZY);
    if(dl_ncmlhandle == NULL)
    {
        TCRIT("Problem in loading library:%s\n",dlerror());
        return -1;
    }
if(0)port_num=port_num;
    dl_getNotEditableData = dlsym(dl_ncmlhandle, "getNotEditableData");
    if(dl_getNotEditableData == NULL)
    {
        TCRIT("No Symbol Found:%s\n",dlerror());
        dlclose(dl_ncmlhandle);
        return -1;
    }
    (*dl_getNotEditableData)((unsigned char *)&(ServiceConfig.NonSecureAccessPort), 
            sizeof(ServiceConfig.NonSecureAccessPort), 
            (unsigned char *)&(conf_port_num));
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    ret = buffer_copy_long(port, conf_port_num);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    buffer_copy_int(port, conf_port_num);
#endif	
    dlclose(dl_ncmlhandle);
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    return ret;
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    return 0;
#endif	
}

/* OK */
int connect_backend(handler_ctx *hctx) {
    data_unset *du;
    buffer *host = NULL;
    buffer *port = NULL;
    SERVICE_CONF_STRUCT ServiceConfig;
    int ret = 0;

    du = array_get_element(hctx->ext->value, "host");
    if (!du || du->type != TYPE_STRING) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "host section is invalid");
        hctx->con->http_status = 500;
        hctx->con->mode = DIRECT;
        return -1;
    }
    host = ((data_string *)du)->value;
    if (buffer_is_empty(host)) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "host section is invalid");
        hctx->con->http_status = 500;
        hctx->con->mode = DIRECT;
        return -1;
    }
    du = array_get_element(hctx->ext->value, "port");
    if (!du) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "port section is invalid");
        hctx->con->http_status = 500;
        hctx->con->mode = DIRECT;
        return -1;
    }
    port = buffer_init();
    //get service config for connecting service.
    ret = get_service_config(hctx->ext->key, &ServiceConfig);
    if( (ret != ERR_LOAD_DEFCONF) && (ret != ERR_GET_DEFCONF) &&
            (ret != ERR_GET_CONF) ) //ERR_SET_CONF doesn't affect service running.
    {
        if(0 > (ret = get_port(ServiceConfig, port))){//get port number from ServiceConfig
            buffer_reset(port);//reset the port buffer in case of failure.
        }
    }
    if(ret < 0)
    {//get port from mod_websocket.conf file (old implementation)
        if (du->type == TYPE_INTEGER) {
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
            if (buffer_copy_long(port, ((data_integer *)du)->value) != 0) {
                buffer_free(port);
                DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "no memory");
                hctx->con->http_status = 500;
                hctx->con->mode = DIRECT;
                return -1;
            }
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
           buffer_copy_int(port, ((data_integer *)du)->value)  ;
#endif
        } else if (du->type == TYPE_STRING && !buffer_is_empty(((data_string *)du)->value)) {
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
            if (buffer_copy_string_buffer(port, ((data_string *)du)->value) != 0) {
                buffer_free(port);
                DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "no memory");
                hctx->con->http_status = 500;
                hctx->con->mode = DIRECT;
                return -1;
            }
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
            buffer_copy_buffer(port, ((data_string *)du)->value);
#endif
        } else {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "port section is invalid");
            buffer_free(port);
            hctx->con->http_status = 500;
            hctx->con->mode = DIRECT;
            return -1;
        }
    }
    DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ssss",
            "try to connect backend ->", host->ptr, ":", port->ptr);
    hctx->fd = mod_websocket_connect(host->ptr, port->ptr);
    if (hctx->fd < 0) {
        buffer_free(port);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "s", "fail to connect");
        hctx->con->http_status = 503;
        hctx->con->mode = DIRECT;
        return -1;
    }
    int flag = 1;
    if (setsockopt(hctx->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "s", "fail to set TCP_NODELAY for backend");
    }
    if (setsockopt(hctx->con->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "s", "fail to set TCP_NODELAY for client");
    }
    hctx->fd_idx = -1;
    hctx->srv->cur_fds++;
    fdevent_register(hctx->srv->ev, hctx->fd, handle_backend, hctx);
    fdevent_event_set(hctx->srv->ev, &(hctx->fd_idx), hctx->fd, FDEVENT_IN);

    // for logging remote ipaddr and port
    if (hctx->pd->conf.debug >= MOD_WEBSOCKET_LOG_INFO) {
        mod_websocket_sockinfo_t info;
        if (mod_websocket_getsockinfo(hctx->con->fd, &info) == 0) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sssdsdsssssds",
                    "connected",
                    info.peer.addr, ":", info.peer.port, "( fd =", hctx->con->fd, ") ->",
                    host->ptr, ":", port->ptr, "( fd =", hctx->fd, ")");
        } else {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "connected");
        }
    }
    buffer_free(port);
    return 0;

}

/* OK */
void disconnect_backend(handler_ctx *hctx) {
    if (hctx && hctx->srv && hctx->fd > 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds", "disconnect backend ( fd =", hctx->fd, ")");
        fdevent_event_del(hctx->srv->ev, &(hctx->fd_idx), hctx->fd);
        fdevent_unregister(hctx->srv->ev, hctx->fd);
        mod_websocket_disconnect(hctx->fd);
        hctx->srv->cur_fds--;
        hctx->fd = -1;
    }
}

void prepare_disconnect_client(handler_ctx *hctx) {
    if (hctx && hctx->con && hctx->con->fd > 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds", "disconnect client ( fd =", hctx->con->fd, ")");
    }
    if (hctx && hctx->srv && hctx->fd > 0) {
        fdevent_event_del(hctx->srv->ev, &(hctx->fd_idx), hctx->fd);
        fdevent_unregister(hctx->srv->ev, hctx->fd);
        mod_websocket_disconnect(hctx->fd);
        hctx->srv->cur_fds--;
        hctx->fd = -1;
    }
}

/* OK */
handler_t handle_backend(server *srv, void *ctx, int revents) {
    handler_ctx *hctx = (handler_ctx *)ctx;
    char readbuf[UINT16_MAX];
    ssize_t siz;

    if (revents & FDEVENT_NVAL || revents & FDEVENT_HUP || revents & FDEVENT_ERR) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds",
                  "disconnected from backend ( fd =", hctx->fd, ")");
        prepare_disconnect_client(hctx);
    } else if (revents & FDEVENT_IN) {
        errno = 0;
        memset(readbuf, 0, sizeof(readbuf));
        siz = read(hctx->fd, readbuf, sizeof(readbuf));
        if (siz == 0) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds",
                      "disconnected from backend ( fd =", hctx->fd, ")");
            prepare_disconnect_client(hctx);
        } else if (siz > 0) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                      "recv data from backend ( fd =", hctx->fd, "), size =", siz);
            if (mod_websocket_frame_send(hctx, hctx->frame.type_backend, readbuf, (size_t)siz) < 0) {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "fail to send data to client");
                chunkqueue_reset(hctx->tocli);
            }
        } else if (errno != EAGAIN && errno != EINTR) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "sdss",
                      "recv error from backend ( fd =", hctx->fd, "),", strerror(errno));
            prepare_disconnect_client(hctx);
        }
    }
    return mod_websocket_handle_subrequest(srv, hctx->con, hctx->pd);
}

/* OK */
static int mod_websocket_patch_connection(server *srv, connection *con, plugin_data *p) {
    size_t i, j;
    plugin_config *s;

    if (!p) {
        return -1;
    }
    s = p->config_storage[0];

#define PATCH(x) do { p->conf.x = s->x; } while (0)

    PATCH(exts);
    PATCH(debug);
    PATCH(timeout);

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
    PATCH(ping_interval);
#endif	/* _MOD_WEBSOCKET_RFC_6455_ */

    /* skip the first, the global context */
    for (i = 1; i < srv->config_context->used; i++) {
        data_config *dc = (data_config *)srv->config_context->data[i];
        s = p->config_storage[i];

        /* condition didn't match */
        if (!config_check_cond(srv, con, dc)) {
            continue;
        }
        /* merge config */
        for (j = 0; j < dc->value->used; j++) {
            data_unset *du = dc->value->data[j];

            if (buffer_is_equal_string(du->key, CONST_STR_LEN(MOD_WEBSOCKET_CONFIG_SERVER))) {
                PATCH(exts);
            } else if (buffer_is_equal_string(du->key, CONST_STR_LEN(MOD_WEBSOCKET_CONFIG_DEBUG))) {
                PATCH(debug);
            }
        }
    }
#undef PATCH
    if (!p->conf.exts) {
        return -1;
    }
    return 0;
}

/* OK */
static handler_t mod_websocket_check_extension(server *srv, connection *con, void *p_d) {
    plugin_data *p = p_d;
    size_t i;
    data_array *ext = NULL;
    handler_ctx *hctx = NULL;

#ifdef	HAVE_PCRE_H
    pcre *re = NULL;
    int rc;
    const char* err_str;
    int err_off;
# define	N	(10)
    int ovec[N * 3];
#endif	/* HAVE_PCRE_H */

    if (con->request.http_method != HTTP_METHOD_GET) {
        return HANDLER_GO_ON;
    }
    if (mod_websocket_patch_connection(srv, con, p) < 0) {
        return HANDLER_GO_ON;
    }
    for (i = p->conf.exts->used; i > 0; i--) {
        ext = (data_array *)p->conf.exts->data[i - 1];

#ifdef	HAVE_PCRE_H
        re = pcre_compile(ext->key->ptr, 0, &err_str, &err_off, NULL);
        rc = pcre_exec(re, NULL, con->uri.path->ptr, con->uri.path->used - 1, 0, PCRE_ANCHORED, ovec, N);
        free(re);
        if (rc > 0) {
            break;
        }
# undef	N
#else
        if (buffer_is_equal(con->uri.path, ext->key)) {
            break;
        }
#endif	/* HAVE_PCRE_H */

        ext = NULL;
    }
    if (!ext) {
        return HANDLER_GO_ON;
    }
    if (p->conf.debug >= MOD_WEBSOCKET_LOG_INFO) {
        log_error_write(srv, __FILE__, __LINE__, "sss",
                        con->uri.path->ptr, "is match WebSocket extension:", ext->key->ptr);
    }

    array *hdrs = NULL;
    data_string *hdr = NULL;
    buffer *connection_hdr_value = NULL;
    buffer *upgrade_hdr_value = NULL;

    hdrs = con->request.headers;
    for (i = hdrs->used; i > 0; i--) {
        hdr = (data_string *)hdrs->data[i - 1];
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Connection"))) {
            connection_hdr_value = buffer_init_buffer(hdr->value);
            buffer_to_lower(connection_hdr_value);
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Upgrade"))) {
            upgrade_hdr_value = buffer_init_buffer(hdr->value);
            buffer_to_lower(upgrade_hdr_value);
        }
    }
    /*
     * Connection: upgrade, keep-alive, ...
     * Upgrade: WebSocket, ...
     */
    if (buffer_is_empty(connection_hdr_value) ||
        buffer_is_empty(upgrade_hdr_value) ||
        strstr(connection_hdr_value->ptr, "upgrade") == NULL ||
        strstr(upgrade_hdr_value->ptr, "websocket") == NULL) {
        if (p->conf.debug >= MOD_WEBSOCKET_LOG_INFO) {
            log_error_write(srv, __FILE__, __LINE__, "ss",
                            con->uri.path->ptr, "is not WebSocket Request");
        }
        buffer_free(connection_hdr_value);
        buffer_free(upgrade_hdr_value);
        return HANDLER_GO_ON;
    }

	if(connection_hdr_value != NULL)
	{
		buffer_free(connection_hdr_value);
		connection_hdr_value =NULL;
	}
	if(upgrade_hdr_value != NULL)
	{
		buffer_free(upgrade_hdr_value);
		upgrade_hdr_value = NULL;
	}
    /* init handler-context */
    hctx = handler_ctx_init();
    if (!hctx) {
        if (p->conf.debug) {
            log_error_write(srv, __FILE__, __LINE__, "s", "no memory.");
        }
        return HANDLER_ERROR;
    }
    con->plugin_ctx[p->id] = hctx;
    con->mode = p->id;
    hctx->srv = srv;
    hctx->con = con;
    hctx->ext = ext;
    hctx->pd = p;
    hctx->fromcli = con->read_queue;
    hctx->tocli = con->write_queue;
    return HANDLER_GO_ON;
}

/* OK */
INIT_FUNC(mod_websocket_init) {
    plugin_data *p;
    p = calloc(1, sizeof(*p));
    return p;
}

/* OK */
FREE_FUNC(mod_websocket_free) {
    size_t i;
    plugin_data *p = p_d;
    plugin_config *s = NULL;

    if (p->config_storage) {
        for (i = 0; i < srv->config_context->used; i++) {
            s = p->config_storage[i];
            if (s) {
                array_free(s->exts);
                free(s);
            }
        }
        free(p->config_storage);
    }
    free(p);
    return HANDLER_GO_ON;
}

/* OK */
SETDEFAULTS_FUNC(mod_websocket_set_defaults) {
    size_t i, j;
    plugin_data *p = p_d;
    config_values_t cv[] = {
        { MOD_WEBSOCKET_CONFIG_SERVER,        NULL, T_CONFIG_LOCAL, T_CONFIG_SCOPE_CONNECTION },
        { MOD_WEBSOCKET_CONFIG_TIMEOUT,       NULL, T_CONFIG_SHORT, T_CONFIG_SCOPE_CONNECTION },
        { MOD_WEBSOCKET_CONFIG_DEBUG,         NULL, T_CONFIG_SHORT, T_CONFIG_SCOPE_CONNECTION },

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
        { MOD_WEBSOCKET_CONFIG_PING_INTERVAL, NULL, T_CONFIG_SHORT, T_CONFIG_SCOPE_CONNECTION },
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

        { NULL,                      NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
    };

    if (!p) {
        return HANDLER_ERROR;
    }
    p->config_storage = calloc(1, srv->config_context->used * sizeof(specific_config *));
    if (!p->config_storage) {
        log_error_write(srv, __FILE__, __LINE__, "s", "no memory.");
        return HANDLER_ERROR;
    }
    for (i = 0; i < srv->config_context->used; i++) {
        plugin_config *s = NULL;
        array *ca = NULL;
        data_unset *du = NULL;
        data_array *da = NULL;
        data_array *ext = NULL;

        s = calloc(1, sizeof(plugin_config));
        if (!s) { /* p->config_storage is freed at FREE_FUNC */
            log_error_write(srv, __FILE__, __LINE__, "s", "no memory");
            return HANDLER_ERROR;
        }
        s->exts = array_init();
        cv[0].destination = s->exts;
        s->timeout = 30; // default timeout == 30 sec
        cv[1].destination = &(s->timeout);
        s->debug = MOD_WEBSOCKET_LOG_NONE;
        cv[2].destination = &(s->debug);

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
        s->ping_interval = 0; // send ping
        cv[3].destination = &(s->ping_interval);
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

        p->config_storage[i] = s;

        ca = ((data_config *)(srv->config_context->data[i]))->value;
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
        if (config_insert_values_global(srv, ca, cv) != 0) {
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
        if (config_insert_values_global(srv, ca, cv, i == 0 ? T_CONFIG_SCOPE_SERVER : T_CONFIG_SCOPE_CONNECTION) != 0) {
#endif	
            log_error_write(srv, __FILE__, __LINE__, "s", "no memory.");
            return HANDLER_ERROR;
        }
        if ((du = array_get_element(ca, MOD_WEBSOCKET_CONFIG_SERVER)) == NULL) {
            continue;
        }
        if (du->type != TYPE_ARRAY) {
            log_error_write(srv, __FILE__, __LINE__, "s", "invalid configuration");
            return HANDLER_ERROR;
        }
        da = (data_array *)du;
        for (j = 0; j < da->value->used; j++) {
            if (da->value->data[j]->type != TYPE_ARRAY) {
                log_error_write(srv, __FILE__, __LINE__, "s", "invalid configuration");
                return HANDLER_ERROR;
            }
            ext = data_array_init();
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
            buffer_copy_string_buffer(ext->key, ((data_array *)(da->value->data[j]))->key);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
            buffer_copy_buffer(ext->key, ((data_array *)(da->value->data[j]))->key);
#endif		
            ext->value = array_init_array(((data_array *)(da->value->data[j]))->value);
            ext->is_index_key = ((data_array *)(da->value->data[j]))->is_index_key;
            array_insert_unique(s->exts, (data_unset *)ext);
        }
    }
    return HANDLER_GO_ON;
}

int alloc_ws_ctx(int Instance) {

	memset(&(WebSockCtx[Instance]), 0, sizeof(webs_ctx));

	if (!(WebSockCtx[Instance].cin_buf = malloc(BUFSIZE))) {
		TCRIT("malloc of cin_buf\n");
		return -1;
	}
	if (!(WebSockCtx[Instance].cout_buf = malloc(BUFSIZE))) {
		TCRIT("malloc of cout_buf\n");
		return -1;
	}
	if (!(WebSockCtx[Instance].tin_buf = malloc(BUFSIZE))) {
		TCRIT("malloc of tin_buf\n");
		return -1;
	}
	if (!(WebSockCtx[Instance].tout_buf = malloc(BUFSIZE))) {
		TCRIT("malloc of tout_buf\n");
		return -1;
	}

	WebSockCtx[Instance].ssl = NULL;
	WebSockCtx[Instance].ssl_ctx = NULL;
	return 0;
}


SUBREQUEST_FUNC(mod_websocket_handle_subrequest) {
    plugin_data *p = p_d;
    handler_ctx *hctx = con->plugin_ctx[p->id];
    int ret;
    int Instance = -1;
    mod_websocket_errno_t err;
    data_unset *du;
    data_string *proto = NULL;

    /* not my job */
    if (!hctx || con->mode != p->id) {
        return HANDLER_GO_ON;
    }
    switch (hctx->state) {
    case MOD_WEBSOCKET_STATE_INIT:
        /* check request */
        err = mod_websocket_handshake_check_request(hctx);
        /* not my job */
        if (err != MOD_WEBSOCKET_OK) {
            hctx->con->http_status = err;
            hctx->con->mode = DIRECT;
            return HANDLER_FINISHED;
        }
        /* select mode */
        du = array_get_element(hctx->ext->value, "proto");
        if (du != NULL && du->type == TYPE_STRING) {
            proto = (data_string *)du;
            if (!buffer_is_empty(proto->value) && strcasecmp(proto->value->ptr, "websocket") == 0) {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "works as WebSocket Proxy");
                hctx->mode = MOD_WEBSOCKET_WEBSOCKET_PROXY;
            } else {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "works as WebSocket-TCP Proxy");
                hctx->mode = MOD_WEBSOCKET_TCP_PROXY;
            }
        } else {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "works as WebSocket-TCP Proxy");
            hctx->mode = MOD_WEBSOCKET_TCP_PROXY;
        }
        if (hctx->mode == MOD_WEBSOCKET_TCP_PROXY) {
            if (hctx->handshake.version == 0) {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "WebSocket Version = hybi-00");
            } else {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sd", "WebSocket Version =", hctx->handshake.version);
            }
        }

        if (hctx->mode == MOD_WEBSOCKET_TCP_PROXY) {
            du = array_get_element(hctx->ext->value, "type");
            if (du == NULL) {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "will recv text data from backend");
                hctx->frame.type = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                hctx->frame.type_before = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                hctx->frame.type_backend = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
            } else {
                if (du->type == TYPE_STRING) {
                    data_string *type = (data_string *)du;
                    if (!buffer_is_empty(type->value) &&
                        (strncasecmp(type->value->ptr, "binary", strlen("binary")) == 0)) {
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "will recv binary data from backend");
                        hctx->frame.type = MOD_WEBSOCKET_FRAME_TYPE_BIN;
                        hctx->frame.type_before = MOD_WEBSOCKET_FRAME_TYPE_BIN;
                        hctx->frame.type_backend = MOD_WEBSOCKET_FRAME_TYPE_BIN;
                    } else {
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "will recv text data from backend");
                        hctx->frame.type = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                        hctx->frame.type_before = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                        hctx->frame.type_backend = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                    }
                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "will recv text data from backend");
                    hctx->frame.type = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                    hctx->frame.type_before = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                    hctx->frame.type_backend = MOD_WEBSOCKET_FRAME_TYPE_TEXT;
                }
            }
        }

	/* if the uri is not kvm/cd then connect to backend server */
	/* kvm/cd instance will be handled by do_webs_proxy thread */
	if((strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_KVM_URI, strlen(MOD_WEBSOCKET_KVM_URI)) != 0) && (strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_CDSERVER_URI, strlen(MOD_WEBSOCKET_CDSERVER_URI)) != 0)){
	    if (connect_backend(hctx) < 0) {
		return HANDLER_FINISHED;
	    }
	}

        if (hctx->mode == MOD_WEBSOCKET_WEBSOCKET_PROXY) {
            err = mod_websocket_handshake_forward_request(hctx);
            if (err != MOD_WEBSOCKET_OK) {
                disconnect_backend(hctx);
                hctx->con->http_status = err;
                hctx->con->mode = DIRECT;
                return HANDLER_FINISHED;
            }
            do {
                ret = srv->NETWORK_BACKEND_WRITE(srv, con, hctx->fd, hctx->tosrv);
                if (0 <= ret) {
                    chunkqueue_remove_finished_chunks(hctx->tosrv);
                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "ss",
                              "forward handshake error,", strerror(errno));
                    hctx->con->http_status = MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
                    hctx->con->mode = DIRECT;
                    return HANDLER_FINISHED;
                }
            } while (!chunkqueue_is_empty(hctx->tosrv));
        } else if (hctx->mode == MOD_WEBSOCKET_TCP_PROXY) {
            /* send handshake response */
            err = mod_websocket_handshake_create_response(hctx);
            if (err != MOD_WEBSOCKET_OK) {
                disconnect_backend(hctx);
                hctx->con->http_status = err;
                hctx->con->mode = DIRECT;
                return HANDLER_FINISHED;
            }
            do {
                if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                    ret = srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#else	/* SSL is not available */
                    ret = -1;
#endif	/* USE_OPENSSL */

                } else {
                    ret = srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
                }
                if (ret >= 0) {
                    chunkqueue_remove_finished_chunks(hctx->tocli);
                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "ss",
                              "send handshake response error,", strerror(errno));
                    hctx->con->http_status = MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
                    hctx->con->mode = DIRECT;
                    return HANDLER_FINISHED;
                }
            } while (!chunkqueue_is_empty(hctx->tocli));
        }
        hctx->state = MOD_WEBSOCKET_STATE_CONNECTED;
        hctx->timeout_cnt = 0;

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
        hctx->ping_ts = srv->cur_ts;
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

	/* kvm/cd instance will be handled by do_webs_proxy thread that'll do the encode/decode functions */
		if ((strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_KVM_URI, strlen(MOD_WEBSOCKET_KVM_URI)) == 0) || (strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_CDSERVER_URI, strlen(MOD_WEBSOCKET_CDSERVER_URI)) == 0)) 
		{
			Instance = WSFindFreeSlot();

			if (Instance < 0 || Instance > MAX_INSTANCES) {
				TCRIT("\nInvalid instance %d\n", Instance);
				break;
			}

			if (alloc_ws_ctx(Instance) != 0) {
				hctx->con->http_status = MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
				hctx->con->mode = DIRECT;
				return HANDLER_FINISHED;
			}

			hctx->con->dupped = 1;
			WebSockCtx[Instance].ssl = hctx->con->ssl;
			// Incase of http, dup the lighttpd socket, the lighttpd socket will be closed in lighttpd, dupped socket will be handled here
			// Incase of https, don't dup the lighttpd socket, the lighttpd socket will not be closed by lighttpd and it will be handled here.
			// This is similar to what we do with sockets in singleport
			WebSockCtx[Instance].sockfd = ((WebSockCtx[Instance].ssl != NULL) ? (hctx->con->fd):(dup(hctx->con->fd)));
			WebSockCtx[Instance].hybi = 1;

			if (ServiceWebSockCmd(Instance, hctx->con->request.uri) != 0) {
			    	hctx->con->dupped = 0;
				hctx->con->ssl = NULL;
				hctx->con->http_status = MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
				hctx->con->mode = DIRECT;
				break;
			}
		}

	fdevent_event_del(srv->ev, &(con->fde_ndx), con->fd);
	fdevent_event_set(srv->ev, &(con->fde_ndx), con->fd, FDEVENT_IN);
	//Dont set the connection state here
	//connection_set_state(srv, hctx->con, CON_STATE_READ_CONTINUOUS_WEB_APP_PROXY);
	if ((strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_SOL_URI, strlen(MOD_WEBSOCKET_SOL_URI)) == 0)) {
		chunkqueue_reset(hctx->fromcli);
		fdevent_event_del(srv->ev, &(hctx->fd_idx), hctx->fd);
		fdevent_event_set(srv->ev, &(hctx->fd_idx), hctx->fd, FDEVENT_IN);
		connection_set_state(srv, hctx->con, CON_STATE_READ_CONTINUOUS_WEB_APP_PROXY);
	}
        return HANDLER_WAIT_FOR_EVENT;

    case MOD_WEBSOCKET_STATE_CONNECTED:
    	/* incase of kvm/cd connections, return handler_wait_for_event
    	 * breaking out of the switch case might lead to lighttpd crash
    	 * since h5viewer kvm/cd connection doesn't use hctx->fromcli,
    	 * hctx->tocli, hctx->fd, hctx->tosrv etc
    	 */
    	if ((strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_KVM_URI, strlen(MOD_WEBSOCKET_KVM_URI)) == 0) || (strncmp(hctx->con->request.uri->ptr, MOD_WEBSOCKET_CDSERVER_URI, strlen(MOD_WEBSOCKET_CDSERVER_URI)) == 0)) {
    		return HANDLER_WAIT_FOR_EVENT;
    	}
        if (hctx->con->fd < 0) {
            break;
        } else {
            if (!chunkqueue_is_empty(hctx->fromcli)) {
                hctx->timeout_cnt = 0;
                if (mod_websocket_frame_recv(hctx) < 0) {
                    if (hctx->mode == MOD_WEBSOCKET_WEBSOCKET_PROXY) {
                        break;
                    }
                    if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds",
                                  "disconnected from client ( fd =", hctx->con->ssl, ")");
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sds",
                                  "send close response to client ( fd =", hctx->con->ssl, ")");
                        mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_CLOSE,
                                                 (char *)"1000", strlen("1000"));
                        srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#endif	/* USE_OPENSSL */

                    } else {
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds",
                                  "disconnected from client ( fd =", hctx->con->fd, ")");
                        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sds",
                                  "send close response to client ( fd =", hctx->con->fd, ")");
                        mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_CLOSE,
                                                 (char *)"1000", strlen("1000"));
                        srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
                    }
                    break;
                }
            }
            if (!chunkqueue_is_empty(hctx->tosrv)) {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                          "send data to backend ( fd =", hctx->fd,
                          "), size =", chunkqueue_length(hctx->tosrv));
                ret = srv->NETWORK_BACKEND_WRITE(srv, con, hctx->fd, hctx->tosrv);
                if (0 <= ret) {
                    chunkqueue_remove_finished_chunks(hctx->tosrv);
                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "ss",
                              "fail to send data to backend:", strerror(errno));
                    chunkqueue_reset(hctx->tosrv);
                }
            }
        }
        if (hctx->fd < 0) {
            chunkqueue_reset(hctx->tocli);
            if (hctx->mode == MOD_WEBSOCKET_WEBSOCKET_PROXY) {
                break;
            }
            if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sds",
                          "send close request to client ( fd =", hctx->con->ssl, ")");
                mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_CLOSE, (char *)"1001", strlen("1001"));
                srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#endif	/* USE_OPENSSL */

            } else {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sds",
                          "send close request to client ( fd =", hctx->con->fd, ")");
                mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_CLOSE, (char *)"1001", strlen("1001"));
                srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
            }
            break;
        } else {
            if (!chunkqueue_is_empty(hctx->tocli)) {
                if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                              "send data to client ( fd =", hctx->con->ssl,
                              "), size =", chunkqueue_length(hctx->tocli));
                    ret = srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#else	/* SSL is not available */
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "SSL is not available");
                    ret = -1;
#endif	/* USE_OPENSSL */

                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                              "send data to client ( fd =", hctx->con->fd,
                              "), size =", chunkqueue_length(hctx->tocli));
                    ret = srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
                }
                if (ret >= 0) {
                    chunkqueue_remove_finished_chunks(hctx->tocli);
                } else {
                    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "ss",
                              "fail to send data to client:", strerror(errno));
                    chunkqueue_reset(hctx->tocli);
                }
            }
        }
        fdevent_event_del(srv->ev, &(hctx->fd_idx), hctx->fd);
        fdevent_event_del(srv->ev, &(con->fde_ndx), con->fd);
        if (!chunkqueue_is_empty(hctx->tosrv)) {
            fdevent_event_set(srv->ev, &(hctx->fd_idx), hctx->fd, FDEVENT_OUT);
        } else {
            fdevent_event_set(srv->ev, &(hctx->fd_idx), hctx->fd, FDEVENT_IN);
        }
        if (!chunkqueue_is_empty(hctx->tocli)) {
            fdevent_event_set(srv->ev, &(con->fde_ndx), con->fd, FDEVENT_OUT);
        } else {
            fdevent_event_set(srv->ev, &(con->fde_ndx), con->fd, FDEVENT_IN);
        }
        connection_set_state(srv, hctx->con, CON_STATE_READ_CONTINUOUS_WEB_APP_PROXY);
        return HANDLER_WAIT_FOR_EVENT;
    default:
	break;
    }
    disconnect_backend(hctx);
    handler_ctx_free(hctx);
    connection_set_state(srv, con, CON_STATE_CLOSE);
    con->plugin_ctx[p->id] = NULL;
    return HANDLER_FINISHED;
}

TRIGGER_FUNC(mod_websocket_handle_trigger) {
    connection *con;
    handler_ctx *hctx;
    plugin_data *p = p_d;
    size_t i;

    for (i = 0; i < srv->conns->used; i++) {
        con = srv->conns->ptr[i];
        if (con->mode != p->id) {
            continue;
        }
        hctx = con->plugin_ctx[p->id];
        if (!hctx || hctx->mode != MOD_WEBSOCKET_TCP_PROXY) {
            continue;
        }

        if (hctx->handshake.version != 0 &&
            p->conf.ping_interval > 0 &&
            p->conf.ping_interval < (unsigned int)difftime(srv->cur_ts, hctx->ping_ts)) {
            mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_PING, (char *)"ping", strlen("ping"));
            if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                          "send data to client ( fd =", hctx->con->ssl,
                          "), size =", chunkqueue_length(hctx->tocli));
                srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#else	/* SSL is not available */
                DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "SSL is not available");
                chunkqueue_reset(hctx->tocli);
#endif	/* USE_OPENSSL */

            } else {
                DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "sdsx",
                          "send data to client ( fd =", hctx->con->fd,
                          "), size =", chunkqueue_length(hctx->tocli));
                srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
            }
            hctx->ping_ts = srv->cur_ts;
            chunkqueue_remove_finished_chunks(hctx->tocli);
        }
        hctx->timeout_cnt += 1;
        if (p->conf.timeout > 0 && hctx->timeout_cnt >= p->conf.timeout) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds", "timeout client ( fd =", con->fd, ")");
            mod_websocket_frame_send(hctx, MOD_WEBSOCKET_FRAME_TYPE_CLOSE, NULL, 0);
            if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
                srv->NETWORK_SSL_BACKEND_WRITE(srv, con, hctx->con->ssl, hctx->tocli);
#else	/* SSL is not available */
                DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "SSL is not available");
                chunkqueue_reset(hctx->tocli);
#endif	/* USE_OPENSSL */

            } else {
                srv->NETWORK_BACKEND_WRITE(srv, con, hctx->con->fd, hctx->tocli);
            }
            disconnect_backend(hctx);
            handler_ctx_free(hctx);
            connection_set_state(srv, con, CON_STATE_CLOSE);
            con->plugin_ctx[p->id] = NULL;
        }
    }
    return HANDLER_GO_ON;
}

/* OK */
static handler_t mod_websocket_disconnected_from_client(server *srv, connection *con, void *pd) {
    plugin_data *p = (plugin_data *)pd;
    handler_ctx *hctx = con->plugin_ctx[p->id];

    UNUSED(srv);

    if (con->plugin_ctx[p->id]) {
        if ((server_socket *)(con->srv_socket) && ((server_socket *)(con->srv_socket))->is_ssl) {

#ifdef	USE_OPENSSL
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds", "disconnected from client ( fd =", con->ssl, ")");
#else	/* SSL is not available */
            DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "SSL is not available");
#endif	/* USE_OPENSSL */

        } else if (hctx) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sds", "disconnected from client ( fd =", con->fd, ")");
        }
        disconnect_backend(hctx);
        handler_ctx_free(hctx);
        con->plugin_ctx[p->id] = NULL;
    }
    return HANDLER_GO_ON;
}

int mod_websocket_plugin_init(plugin *p) {
    p->version = LIGHTTPD_VERSION_ID;
    p->name = buffer_init_string("mod_websocket");
    p->init = mod_websocket_init;
    p->cleanup = mod_websocket_free;
    p->set_defaults = mod_websocket_set_defaults;
    p->connection_reset = mod_websocket_disconnected_from_client;
    p->handle_connection_close = mod_websocket_disconnected_from_client;
    p->handle_uri_clean = mod_websocket_check_extension;
    p->read_continuous = mod_websocket_handle_subrequest;
    p->handle_subrequest = mod_websocket_handle_subrequest;
    p->handle_trigger = mod_websocket_handle_trigger;
    p->data = NULL;
    return 0;
}

/* EOF */
