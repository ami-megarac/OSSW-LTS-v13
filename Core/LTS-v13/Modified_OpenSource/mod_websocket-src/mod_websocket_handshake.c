/*
 * Copyright(c) 2010, Norio Kobota, All rights reserved.
 */

#include "mod_websocket.h"
#include "mod_websocket_socket.h"

# include <string.h>

#ifdef HAVE_PCRE_H
# include <pcre.h>
#endif /* HAVE_PCRE_H */

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
# include <ctype.h>
# include <poll.h>
# include <unistd.h>
# include "md5.h"

# define	SEC_WEBSOCKET_KEY3_STRLEN	(8)
# define	MD5_STRLEN			(16)

# if defined (LIGHTTPD_VERSION_ID) && (LIGHTTPD_VERSION_ID >= (1 << 16 | 4 << 8 | 29))
typedef li_MD5_CTX MD5_CTX;
#  define	MD5_Init	li_MD5_Init
#  define	MD5_Update	li_MD5_Update
#  define	MD5_Final	li_MD5_Final
# endif	/* (LIGHTTPD_VERSION_ID) && (LIGHTTPD_VERSION_ID >= (1 << 16 | 4 << 8 | 29)) */

#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
# include <limits.h>

# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif

# ifdef USE_OPENSSL
#  include <openssl/sha.h>
typedef unsigned char sha1_byte;
# else
#  include "mod_websocket_sha1.h"
#  define	SHA_DIGEST_LENGTH	SHA1_DIGEST_LENGTH
# endif

# include "mod_websocket_base64.h"
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
static int get_key3(handler_ctx *hctx) {
    int ret, timeout = 1000; /* XXX: poll timeout = 1000ms */
    char key3[SEC_WEBSOCKET_KEY3_STRLEN];
    chunkqueue *q;
    const char *body = NULL;
    struct pollfd pfd;
    ssize_t siz;

    q = hctx->con->read_queue;
    if (chunkqueue_is_empty(q)) {
        pfd.fd = hctx->con->fd;
        pfd.events = POLLIN;
        ret = poll(&pfd, 1, timeout);
        if (ret > 0 && pfd.revents & POLLIN) {
            siz = read(hctx->con->fd, key3, SEC_WEBSOCKET_KEY3_STRLEN);
            if (siz != SEC_WEBSOCKET_KEY3_STRLEN) {
                return -1;
            }
            buffer_copy_string_len(hctx->handshake.key3, key3, SEC_WEBSOCKET_KEY3_STRLEN);
        } else {
            return -1;
        }
    } else {
        body = &q->first->mem->ptr[q->first->offset];
        buffer_copy_string_len(hctx->handshake.key3, body, SEC_WEBSOCKET_KEY3_STRLEN);
    }
    return 0;
}
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

static mod_websocket_bool_t is_allowed_origin(handler_ctx *hctx) {
    size_t i;
    data_unset *du = NULL;
    array *allowed_origins = NULL;
    data_string *allowed_origin = NULL;

#ifdef	HAVE_PCRE_H
    pcre *re = NULL;
    int rc = 0;
    const char *err_str;
    int err_off;
# define	N	(10)
    int ovec[N * 3];
#endif	/* HAVE_PCRE_H */

    du = array_get_element(hctx->ext->value, "origins");
    if (!du || du->type != TYPE_ARRAY) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "allowed origins are not specified");
        return MOD_WEBSOCKET_TRUE;
    }
    allowed_origins = ((data_array *)du)->value;
    if (!allowed_origins || allowed_origins->used == 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "s", "allowed origins are not specified");
        return MOD_WEBSOCKET_TRUE;
    }
    for (i = allowed_origins->used; i > 0; i--) {
        du = allowed_origins->data[i - 1];
        if (du->type != TYPE_STRING) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "s", "allowed origin configuration is invalid");
            continue;
        }
        allowed_origin = (data_string *)du;
        if (buffer_is_empty(allowed_origin->value)) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "s", "allowed origin value is empty");
            continue;
        }

#ifdef	HAVE_PCRE_H
        re = pcre_compile(allowed_origin->value->ptr, 0, &err_str, &err_off, NULL);
        if (!re) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_WARN, "ss", allowed_origin->value->ptr, "is invalid RegExp");
            continue;
        }
        rc = pcre_exec(re, NULL, hctx->handshake.origin->ptr, hctx->handshake.origin->used - 1,
                       0, PCRE_ANCHORED, ovec, N);
        free(re);
        if (rc > 0) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sss",
                      hctx->handshake.origin->ptr, "is match allowed origin:", allowed_origin->value->ptr);
            return MOD_WEBSOCKET_TRUE;
        }
# undef	N
#else
        if (buffer_is_equal(allowed_origin->value, hctx->handshake.origin)) {
            DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "sss",
                      hctx->handshake.origin->ptr, "is match allowed origin:", allowed_origin->value->ptr);
            return MOD_WEBSOCKET_TRUE;
        }
#endif	/* HAVE_PCRE_H */

    }
    DEBUG_LOG(MOD_WEBSOCKET_LOG_INFO, "ss",
              hctx->handshake.origin->ptr, "is not match any allowed origins");
    return MOD_WEBSOCKET_FALSE;
}

mod_websocket_errno_t mod_websocket_handshake_check_request(handler_ctx *hctx) {
    size_t i;
    array *hdrs;
    data_string *hdr = NULL;
    mod_websocket_handshake_t *handshake;
    buffer *version_hdr_value = NULL;

    if (hctx == NULL || hctx->con == NULL) {
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
    hdrs = hctx->con->request.headers;
    handshake = &hctx->handshake;

    /* store necessary headers */
    for (i = hdrs->used; i > 0; i--) {
        hdr = (data_string *)hdrs->data[i - 1];
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Host"))) {
            handshake->host = hdr->value;
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Sec-WebSocket-Version"))) {
            version_hdr_value = hdr->value;
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Origin")) ||
            buffer_is_equal_string(hdr->key, CONST_STR_LEN("Sec-WebSocket-Origin"))) {
            handshake->origin = hdr->value;
        }

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Sec-WebSocket-Key1"))) {
            handshake->key1 = hdr->value;
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Sec-WebSocket-Key2"))) {
            handshake->key2 = hdr->value;
        }
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("Sec-WebSocket-Key"))) {
            handshake->key = hdr->value;
        }
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

    }
    if (buffer_is_empty(handshake->host)) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Host header does not exist");
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
    if (buffer_is_empty(handshake->origin)) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Origin header does not exist");
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
    if (is_allowed_origin(hctx) != MOD_WEBSOCKET_TRUE) {
        return MOD_WEBSOCKET_FORBIDDEN;
    }
    if (buffer_is_empty(version_hdr_value)) {
        handshake->version = 0;
    } else {
        handshake->version = (int)(strtol(version_hdr_value->ptr, NULL, 10) & INT_MAX);
    }

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
    if (handshake->version == 0 && get_key3(hctx) < 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Sec-WebSocket-Key is invalid");
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

    return MOD_WEBSOCKET_OK;
}

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
static uint32_t count_spc(buffer *b) {
    size_t i;
    uint32_t c = 0;

    for (i = b->used; i > 0; i--) {
        c += (b->ptr[i - 1] == ' ');
    }
    return c;
}

static int get_key_number(uint32_t *ret, buffer *b) {
#define	UINT32_MAX_STRLEN	(10)
    char tmp[UINT32_MAX_STRLEN + 1];
    size_t i, j = 0;
    unsigned long n;
    uint32_t s;

    memset(tmp, 0, sizeof(tmp));
    for (i = 0; i < b->used; i++) {
        if (isdigit((int)b->ptr[i])) {
            tmp[j] = b->ptr[i];
            j++;
        }
        if (UINT32_MAX_STRLEN < j) {
            return -1;
        }
    }
    n = strtoul(tmp, NULL, 10);
    if (UINT32_MAX < n) {
        return -1;
    }
    s = count_spc(b);
    if (s == 0) {
        return -1;
    }
    *ret = (uint32_t)(n / s);
    return 0;
#undef	UINT32_MAX_STRLEN
}

static int create_MD5_sum(unsigned char *md5sum, handler_ctx *hctx) {
    unsigned char buf[MD5_STRLEN];
    uint32_t k1 = 0, k2 = 0;
    MD5_CTX ctx;

    if (buffer_is_empty(hctx->handshake.key1) ||
        buffer_is_empty(hctx->handshake.key2) ||
        buffer_is_empty(hctx->handshake.key3)) {
        return -1;
    }
    if (get_key_number(&k1, hctx->handshake.key1) < 0 ||
        get_key_number(&k2, hctx->handshake.key2) < 0 ||
        hctx->handshake.key3->used < SEC_WEBSOCKET_KEY3_STRLEN) {
        return -1;
    }
    buf[0] = (k1 >> 24) & 0x0ff;
    buf[1] = (k1 >> 16) & 0x0ff;
    buf[2] = (k1 >>  8) & 0x0ff;
    buf[3] =  k1        & 0x0ff;
    buf[4] = (k2 >> 24) & 0x0ff;
    buf[5] = (k2 >> 16) & 0x0ff;
    buf[6] = (k2 >>  8) & 0x0ff;
    buf[7] =  k2        & 0x0ff;
    memcpy(&buf[8], hctx->handshake.key3->ptr, SEC_WEBSOCKET_KEY3_STRLEN);
    MD5_Init(&ctx);
    MD5_Update(&ctx, buf, sizeof(buf));
    MD5_Final(md5sum, &ctx);
    return 0;
}

static mod_websocket_errno_t create_response_ietf_00(handler_ctx *hctx) {
    const char *const_hdr = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
                            "Upgrade: WebSocket\r\n"
                            "Connection: Upgrade\r\n";
    buffer *resp = NULL;
    unsigned char md5sum[MD5_STRLEN];

    /* calc MD5 sum from keys */
    if (create_MD5_sum(md5sum, hctx) < 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Sec-WebSocket-Key is invalid");
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    resp = chunkqueue_get_append_buffer(hctx->tocli);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    resp = buffer_init();
#endif 		
    if (resp == NULL) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "no memory");
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    buffer_append_string(resp, const_hdr);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    buffer_copy_string(resp, const_hdr);
#endif 		
    /* Sec-WebSocket-Origin header */
    if (!buffer_is_empty(hctx->handshake.origin)) {
        buffer_append_string(resp, "Sec-WebSocket-Origin: ");
        buffer_append_string_buffer(resp, hctx->handshake.origin);
        buffer_append_string(resp, "\r\n");
    } else {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Origin header is invalid");
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45  	
		buffer_free(resp);
#endif 		
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
    /* Sec-WebSocket-Location header */
    buffer_append_string(resp, "Sec-WebSocket-Location: ");
    if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {
        buffer_append_string(resp, "wss://");
    } else {
        buffer_append_string(resp, "ws://");
    }
    if (!buffer_is_empty(hctx->handshake.host)) {
        buffer_append_string_buffer(resp, hctx->handshake.host);
    } else {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Host header does not exist");
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45  	
        buffer_free(resp);       
#endif 		
        return MOD_WEBSOCKET_BAD_REQUEST;
    }
    buffer_append_string_buffer(resp, hctx->con->uri.path);
    buffer_append_string(resp, "\r\n\r\n");
    buffer_append_string_len(resp, (char *)md5sum, MD5_STRLEN);
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45  	
    chunkqueue_append_buffer(hctx->tocli, resp);
    buffer_free(resp);
#endif 		
    return MOD_WEBSOCKET_OK;
}
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
static mod_websocket_errno_t create_response_rfc_6455(handler_ctx *hctx) {
    const char *const_hdr = "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n";
    buffer *key;
    buffer *resp = NULL;
    SHA_CTX sha;
    unsigned char sha_digest[SHA_DIGEST_LENGTH];
    unsigned char *accept_body;
    size_t accept_body_siz;
    data_unset *du;

    if (buffer_is_empty(hctx->handshake.key)) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "Sec-WebSocket-Key is invalid");
        return MOD_WEBSOCKET_BAD_REQUEST;
    }

    key = buffer_init_buffer(hctx->handshake.key);
/* refer: RFC-6455 Sec.1.3 Opening Handshake */
#define	GUID	"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    buffer_append_string(key, GUID);
#undef GUID

    /* get SHA1 hash of key */
    SHA1_Init(&sha);
    SHA1_Update(&sha, (sha1_byte *)key->ptr, key->used - 1);
    SHA1_Final(sha_digest, &sha);
    buffer_free(key);

    /* get base64 encoded SHA1 hash */
    if (mod_websocket_base64_encode(&accept_body, &accept_body_siz, sha_digest, SHA_DIGEST_LENGTH) < 0) {
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "no memory");
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    resp = chunkqueue_get_append_buffer(hctx->tocli);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    resp = buffer_init(); 
#endif 		
    if (resp == NULL) {
        free(accept_body);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "no memory");
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
    buffer_append_string(resp, const_hdr);
    buffer_append_string(resp, "Sec-WebSocket-Accept: ");
    buffer_append_string_len(resp, (char *)accept_body, accept_body_siz);
    free(accept_body);
    buffer_append_string(resp, "\r\n");

    du = array_get_element(hctx->ext->value, "subproto");
    if (du && du->type == TYPE_STRING) {
        buffer_append_string(resp, "Sec-WebSocket-Protocol: ");
        buffer_append_string_buffer(resp, ((data_string *)du)->value);
        buffer_append_string(resp, "\r\n");
    }
    buffer_append_string(resp, "\r\n");
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45  	
    chunkqueue_append_buffer(hctx->tocli, resp);
    buffer_free(resp);
#endif 		
    return MOD_WEBSOCKET_OK;
}
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

mod_websocket_errno_t mod_websocket_handshake_create_response(handler_ctx *hctx) {
    if (!hctx) {
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
    DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "s", "send handshake response");

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
    if (hctx->handshake.version == 0) {
        return create_response_ietf_00(hctx);
    }
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

#ifdef	_MOD_WEBSOCKET_SPEC_RFC_6455_
    if (hctx->handshake.version >= 8) {
        return create_response_rfc_6455(hctx);
    }
#endif	/* _MOD_WEBSOCKET_SPEC_RFC_6455_ */

    DEBUG_LOG(MOD_WEBSOCKET_LOG_ERR, "s", "not supported WebSocket Version");
    return MOD_WEBSOCKET_SERVICE_UNAVAILABLE;
}

/*
 * append X-Forwarded-{For, Proto, Port}
 */
static void append_x_forwarded_headers(handler_ctx *hctx) {
    size_t i;
    mod_websocket_sockinfo_t info;
    array *hdrs;
    data_string *hdr = NULL;
    data_string *x_forwarded_proto = NULL;
    data_string *x_forwarded_for = NULL;
    data_string *x_forwarded_port = NULL;
    buffer *addr = NULL;
    buffer *port = NULL;

    if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {

#ifdef USE_OPENSSL
        if (mod_websocket_getsockinfo(SSL_get_fd(hctx->con->ssl), &info) < 0) {
            addr = buffer_init_string("unknown");
            port = buffer_init_string("unknown");
        } else {
            addr = buffer_init_string(info.peer.addr);
            port = buffer_init();
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
            buffer_append_long(port, info.peer.port);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
            buffer_append_int(port, info.peer.port);
#endif 		
        }
#else
        addr = buffer_init_string("unknown");
        port = buffer_init_string("unknown");
#endif

    } else {
        if (mod_websocket_getsockinfo(hctx->con->fd, &info) < 0) {
            addr = buffer_init_string("unknown");
            port = buffer_init_string("unknown");
        } else {
            addr = buffer_init_string(info.peer.addr);
            port = buffer_init();
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
            buffer_append_long(port, info.peer.port);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
            buffer_append_int(port, info.peer.port);
#endif 		
        }
    }
    hdrs = hctx->con->request.headers;
    for (i = hdrs->used; i > 0; i--) {
        hdr = (data_string *)hdrs->data[i - 1];
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("X-Forwarded-Proto"))) {
            x_forwarded_proto = hdr;
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("X-Forwarded-For"))) {
            x_forwarded_for = hdr;
        }
        if (buffer_is_equal_string(hdr->key, CONST_STR_LEN("X-Forwarded-Port"))) {
            x_forwarded_port = hdr;
        }
    }
    if (x_forwarded_proto == NULL) {
        x_forwarded_proto = data_string_init();
        buffer_append_string(x_forwarded_proto->key, "X-Forwarded-Proto");
        if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {
            buffer_append_string(x_forwarded_proto->value, "https");
        } else {
            buffer_append_string(x_forwarded_proto->value, "http");
        }
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "append X-Forwarded-Proto:", x_forwarded_proto->value->ptr);
        array_insert_unique(hdrs, (data_unset *)x_forwarded_proto);
    } else {
        if (((server_socket *)(hctx->con->srv_socket))->is_ssl) {
            buffer_append_string(x_forwarded_proto->value, ", https");
        } else {
            buffer_append_string(x_forwarded_proto->value, ", http");
        }
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "X-Forwarded-Proto:", x_forwarded_proto->value->ptr);
    }
    if (x_forwarded_for == NULL) {
        x_forwarded_for = data_string_init();
        buffer_append_string(x_forwarded_for->key, "X-Forwarded-For");
        buffer_append_string_buffer(x_forwarded_for->value, addr);
        buffer_append_string(x_forwarded_for->value, ":");
        buffer_append_string_buffer(x_forwarded_for->value, port);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "append X-Forwarded-For:", x_forwarded_for->value->ptr);
        array_insert_unique(hdrs, (data_unset *)x_forwarded_for);
    } else {
        buffer_append_string(x_forwarded_for->value, ", ");
        buffer_append_string_buffer(x_forwarded_for->value, addr);
        buffer_append_string(x_forwarded_for->value, ":");
        buffer_append_string_buffer(x_forwarded_for->value, port);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "X-Forwarded-For:", x_forwarded_for->value->ptr);
    }
    if (x_forwarded_port == NULL) {
        x_forwarded_port = data_string_init();
        buffer_append_string(x_forwarded_port->key, "X-Forwarded-Port");
        buffer_append_string_buffer(x_forwarded_port->value, port);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "append X-Forwarded-Port:", x_forwarded_port->value->ptr);
        array_insert_unique(hdrs, (data_unset *)x_forwarded_port);
    } else {
        buffer_append_string(x_forwarded_port->value, ", ");
        buffer_append_string_buffer(x_forwarded_port->value, port);
        DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "ss", "X-Forwarded-Port:", x_forwarded_port->value->ptr);
    }
    buffer_free(addr);
    buffer_free(port);
    return;
}

mod_websocket_errno_t mod_websocket_handshake_forward_request(handler_ctx *hctx) {
    size_t i;
    array *hdrs;
    data_string *hdr = NULL;
    buffer *new_request;

    if (!hctx || !hctx->con || !hctx->tosrv) {
        return MOD_WEBSOCKET_INTERNAL_SERVER_ERROR;
    }
    append_x_forwarded_headers(hctx);
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_35  	
    new_request = chunkqueue_get_append_buffer(hctx->tosrv);
#elif defined(CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45)
    new_request = buffer_init();    
#endif 		
    buffer_append_string_buffer(new_request, hctx->con->request.request_line);
    buffer_append_string(new_request, "\r\n");
    hdrs = hctx->con->request.headers;
    for (i = 0; i < hdrs->used; i++) {
        hdr = (data_string *)hdrs->data[i];
        buffer_append_string_buffer(new_request, hdr->key);
        buffer_append_string(new_request, ": ");
        buffer_append_string_buffer(new_request, hdr->value);
        buffer_append_string(new_request, "\r\n");
    }
    buffer_append_string(new_request, "\r\n");

#ifdef	_MOD_WEBSOCKET_SPEC_IETF_00_
	buffer_string_prepare_append(hctx->handshake.key3, SEC_WEBSOCKET_KEY3_STRLEN);
    buffer_append_string_len(new_request, hctx->handshake.key3->ptr, SEC_WEBSOCKET_KEY3_STRLEN);
#endif	/* _MOD_WEBSOCKET_SPEC_IETF_00_ */

    DEBUG_LOG(MOD_WEBSOCKET_LOG_DEBUG, "s", "forward handshake request");
#ifdef CONFIG_SPX_FEATURE_LIGHTTPD_1_4_45  	
    chunkqueue_append_buffer(hctx->tosrv, new_request);
    buffer_free(new_request);
#endif 		
    return MOD_WEBSOCKET_OK;
}
