/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "handshake.h"
#include "errors.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <dlfcn.h>
#include "Encode.h"

#define QDECODER_LIB "/usr/local/lib/libqdecoder.so"
#define MAX_LINE_BUF 1024
#define QSESSION_KEY "QSESSIONID"
#define CSRFTOKEN_KEY "garc"

#define AUTH_SUCCESS 0
#define ERROR_BUFFER_OVERFLOW -1
#define ERROR_INVALID_AUTH -2
#define ERROR_INTERNAL_ERROR -3

static int GetAuthTokenInCookie(char *token, char *key, char *value) {
    char temp[MAX_LINE_BUF];
    int tempret = snprintf(temp, MAX_LINE_BUF, "%s", token);
    if (tempret < 0 || tempret >= MAX_LINE_BUF) {
        return ERROR_BUFFER_OVERFLOW;
    }

    char *start = strstr(temp, key);
    if (start == NULL) {
        return ERROR_INVALID_AUTH;
    } else {
        char *query = start, *p;
        char **tokens = &query;

        // Assume value contains no as same special character as below delimiter
        p = strsep(tokens, ";");
        char *val = p, *var;
        if ((var = strsep(&val, "="))) {
            if (strncasecmp(key, var, strnlen(key, MAX_LINE_BUF)) == 0) {
                tempret = snprintf(value, MAX_LINE_BUF, "%s", val);
                if (tempret < 0 || tempret >= MAX_LINE_BUF) {
                    return ERROR_BUFFER_OVERFLOW;
                }
            } else {
                return ERROR_INTERNAL_ERROR;
            }
        } else {
            return ERROR_INVALID_AUTH;
        }
    }
    return AUTH_SUCCESS;
}


/**
 * Returns whether x is a integral multiple of y.
 */
uint32_t isIntergralMultiple(long x, int y) {
	if (y == 1) {
		return (uint32_t) x;
	} else {
		return (uint32_t) x/y;
	}
}

uint32_t generateKey(char *key, int length) {	
	char result[length];
	char chr[2];
	int i, spaces = 0;

	memset(result, '\0', length);

	for(i = 0; i < length; i++) {
		if ( isdigit((unsigned char) key[i]) != 0 ) {	
			memset(chr, '\0', 2);
			sprintf(chr, "%c", key[i]);
			strcat(result, chr);
		} else if (isblank((unsigned char) key[i]) != 0 && key[i] != '\t') {
			spaces++;	
		}
	}

	if (spaces < 1) {
		return 0;
	}

	return isIntergralMultiple(strtol(result, (char **) NULL, 10), spaces);
}

void concate(uint32_t key1, uint32_t key2, char *key3, char *result){
	memset(result, '\0', KEYSIZE);
	memcpy(result, (unsigned char *) &key1, sizeof(uint32_t));
	memcpy(result+sizeof(uint32_t), (unsigned char *) &key2, sizeof(uint32_t));
	memcpy(result+(2*sizeof(uint32_t)), key3, strlen(key3));
}

char *getMemory(char *token, int length) {
	char *temp = (char *) malloc(length);

	if (temp == NULL) {	
		return NULL;
	}

	memset(temp, '\0', length);	
	memcpy(temp, token, length);
	return temp;
}

int get_FileSize (const char *file_name) {
	struct stat sb;
	if ( stat(file_name, &sb) != 0 ) {
		return -1;
	}
	return sb.st_size;
}

char *read_file (const char *file_name) {
	FILE *f;
	char *contents;
	int s, bytes_read;
    //int bytes_write;
	int status;

	f = fopen(file_name, "a+");
	if (f == NULL) {
		return NULL;
	}

	s = get_FileSize(file_name);
	
	if (s <= 0) {
		fclose(f);
		return NULL;
	}

	contents = malloc(s + 1);
	if (contents == NULL) {
		fclose(f);
		return NULL;
	}
	
	bytes_read = fread(contents, sizeof(char), s, f);
	if (bytes_read != s) {
		free(contents);
		fclose(f);
		return NULL;
	}

#if 0
	if (bytes_read == 0)
	{
		bytes_write = fwrite("0\r\n", sizeof(char), 3, f);
		if (bytes_write != 3) {
			free(contents);
			fclose(f);
			return NULL;
		}
	}
#endif

	status = fclose(f);
	if (status != 0) {
		free(contents);
		return NULL;
	}

	return contents;
}

int isNeedleInHaystack(char *needle, char *file_name, int port) {
    int i = 0, amount = 0, ok = 0;
    char *err = NULL, *file = NULL, *tok = NULL;

    file = read_file(file_name);

    if (file != NULL) {
        tok = strtok(file, "\r\n");
        if (tok == NULL) {
            return ok;
        } else {
            amount = strtol(tok, &err, 10);

            if (err[0] == '\0' && amount > 0) {
                char *haystack[amount];
                for (i = amount - 1; i > -1; i--) {
                    tok = strtok(NULL, "\r\n");
                    if (tok == NULL) {
                        free(file);
                        return ok;
                    } else {
                        haystack[i] =
                            (char *)malloc(strlen(tok) + sizeof(port) + sizeof(char) + 1);
                        if (haystack[i] == NULL) {
                            free(file);
                            return ok;
                        }
                        memset(haystack[i], '\0',
                               strlen(tok) + sizeof(port) + sizeof(char) + 1);
                        memcpy(haystack[i], tok, strlen(tok));
                        if (port > 1024 && port < 65535) {
                            memcpy(haystack[i] + strlen(tok), ":", sizeof(char));
                            sprintf(haystack[i] + strlen(tok) + sizeof(char), "%d", port);
                        }
                    }
                }

                for (i = amount - 1; i > -1; i--) {
                    if (needle != NULL && haystack[i] != NULL) {
                        if (strncasecmp(haystack[i], needle, strlen(needle)) == 0) {
                            ok = 0;
                            break;
                        } else {
                            ok = -1;
                        }
                    }
                }

                for (i = amount - 1; i > -1; i--) {
                    free(haystack[i]);
                }

            } else {
                free(file);
                return ok;
            }
        }
    }

    free(file);
    return ok;
}

int parseHeaders(char *string, ws_client *n, int port){
	if (n == NULL) {
		printf("Parsing Headers went wrong\n");
		return -1;
	}

	ws_header *h = n->headers;
	char qsession[MAX_LINE_BUF] = {0}, csrftoken[MAX_LINE_BUF] = {0};
	char *token = strtok(string, "\r\n");
	char *resource;
	int i, rc = 0;

	void *dl_handle = NULL;
	int (*is_valid_authorization)(char *, char *);

	/**
	 * Split the header string into pieces that we place in the header struct
	 * in the node parameter.
	 */
	if (token != NULL) {
		h->get = token;
		h->get_len = strlen(h->get);

		if ( strncasecmp("GET /", h->get, 5) != 0 || 
				strncasecmp(" HTTP/1.1", h->get+(h->get_len-9), 9) != 0 ) {
			handshake_error("The headerline of the request was invalid.", ERROR_BAD, 
						n);
			return -1;
		}
		
		resource = (char *) getMemory(h->get+4, h->get_len-12);
		if (resource == NULL) {
			handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}
		resource[h->get_len-13] = '\0';

		h->resourcename = resource;
		h->resourcename_len = strlen(h->resourcename);

		while ( token != NULL ) {
			if ( strncasecmp("Sec-WebSocket-Version: ", token, 23) == 0 ) {
				h->version = strtol(token + 23, (char **) NULL, 10);
				if ( h->version == 7 ) {
					h->type = HYBI07;
				} else if( h->version == 8 ) {
					h->type = HYBI10;
				} else if( h->version == 13 ) {
					h->type = RFC6455;
				}
			} else if ( strncasecmp("Upgrade: ", token, 9) == 0) {
				h->upgrade = token + 9;
				h->upgrade_len = strlen(h->upgrade);
			} else if ( strncasecmp("Origin: ", token, 8) == 0 ) {
				h->origin = token + 8;
				h->origin_len = strlen(h->origin);
			} else if ( strncasecmp("Connection: ", token, 12) == 0 ) {
				h->connection = token + 12;
			} else if ( strncasecmp("Sec-WebSocket-Protocol: ", token, 
						24) == 0 ) {
				if ( strstr(token+24, "chat") != NULL ) {
					h->protocol = CHAT;	
					h->protocol_string = (char *) getMemory("chat", 5);
					if (h->protocol_string == NULL) {
						handshake_error("Couldn't allocate memory.", 
								ERROR_INTERNAL, n);
						return -1;
					}
					h->protocol_len = strlen(h->protocol_string);
				} else if ( strstr(token+24, "echo") != NULL ) {
					h->protocol = ECHO_t;
					h->protocol_string = (char *) getMemory("echo", 5);
					if (h->protocol_string == NULL) {
						handshake_error("Couldn't allocate memory.", 
								ERROR_INTERNAL, n);
						return -1;
					}
					h->protocol_len = strlen(h->protocol_string);
				}
			} else if ( strncasecmp("Sec-WebSocket-Origin: ", token, 
						22) == 0) {
				h->origin = token + 22;
				h->origin_len = strlen(h->origin);
			} else if ( strncasecmp("Sec-WebSocket-Key: ", token, 19) == 0 ) {
				h->key = token + 19;
			} else if ( strncasecmp("Sec-WebSocket-Extensions: ", token, 
						26) == 0 ) {
				h->extension = token + 26;
				h->extension_len = strlen(h->extension);
			} else if ( strncasecmp("Host: ", token, 6) == 0 ) {
				h->host = token + 6;
				h->host_len = strlen(h->host);
			} else if ( strncasecmp("WebSocket-Protocol: ", token, 20) == 0 ) {
				h->type = HIXIE75;
				if ( strstr(token+20, "chat") != NULL ) {
					h->protocol = CHAT;	
					h->protocol_string = (char *) getMemory("chat", 5);
					if (h->protocol_string == NULL) {
						handshake_error("Couldn't allocate memory.", 
								ERROR_INTERNAL, n);
						return -1;
					}
					h->protocol_len = strlen(h->protocol_string);
				} else if ( strstr(token+20, "echo") != NULL ) {
					h->protocol = ECHO_t;
					h->protocol_string = (char *) getMemory("echo", 5);
					if (h->protocol_string == NULL) {
						handshake_error("Couldn't allocate memory.", 
								ERROR_INTERNAL, n);
						return -1;					
					}
					h->protocol_len = strlen(h->protocol_string);
				}
			} else if ( strncasecmp("Sec-WebSocket-Key1: ", token, 20) == 0 ) {
				h->type = HYBI00;
				h->key1 = token + 20;
			} else if ( strncasecmp("Sec-WebSocket-Key2: ", token, 20) == 0 ) {
				h->type = HYBI00;
				h->key2 = token + 20;
			} else if ( strncasecmp("Cookie: ", token, 8) == 0 ) {
				rc = GetAuthTokenInCookie(token, QSESSION_KEY, qsession);
				if (rc != AUTH_SUCCESS) {
					switch (rc) {
						case ERROR_BUFFER_OVERFLOW:
							handshake_error("Buffer Overflow.", ERROR_NOT_AUTH, n);
							break;
						case ERROR_INVALID_AUTH:
							handshake_error("Invalid Authentication.", ERROR_NOT_AUTH, n);
							break;
						case ERROR_INTERNAL_ERROR:
							handshake_error("Internal Error.", ERROR_NOT_AUTH, n);
							break;
						default:
							handshake_error("Internal Error.", ERROR_NOT_AUTH, n);
							break;
					}
					return -1;
				}
				rc = GetAuthTokenInCookie(token, CSRFTOKEN_KEY, csrftoken);
				if (rc != 0) {
					switch (rc) {
						case ERROR_BUFFER_OVERFLOW:
							handshake_error("Buffer Overflow.", ERROR_NOT_AUTH, n);
								break;
						case ERROR_INVALID_AUTH:
							handshake_error("Invalid Authentication.", ERROR_NOT_AUTH, n);
							break;
						case ERROR_INTERNAL_ERROR:
							handshake_error("Internal Error.", ERROR_NOT_AUTH, n);
							break;
						default:
							handshake_error("Internal Error.", ERROR_NOT_AUTH, n);
							break;
					}
					return -1;
				}
			}
			
			char *temp = strtok(NULL, "\r\n");

			if ( temp == NULL && h->type == HYBI00) {
				h->type = HYBI00;
				h->key3 = token;
			}

			token = temp;
		}
	} else {
		handshake_error("The parsing of the headers went wrong.", ERROR_BAD, n);
		return -1;
	}

	dl_handle = dlopen(QDECODER_LIB, RTLD_NOW | RTLD_NODELETE);
	if(dl_handle == NULL)
	{    
		handshake_error("Library not found.", ERROR_INTERNAL, n);
		return -1;
	}    

	is_valid_authorization = dlsym(dl_handle,"is_valid_authorization");
	if(is_valid_authorization)
	{
		if(!(is_valid_authorization(qsession, csrftoken)))
		{
			handshake_error("Invalid Authentication.", ERROR_NOT_AUTH, n);
			dlclose(dl_handle);
			return -1;
		}
	}
	
	dlclose(dl_handle);

	/**
	 * If the client header contained a host, we check whether we want to
	 * accept the host.
	 */
	if (h->host != NULL) {
		i = isNeedleInHaystack(h->host, "Hosts.dat", port);

		if (i < 0) {
			handshake_error("The requested host is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}
	}

	/**
	 * If the client header contained an origin, we check whether we want to
	 * accept the origin.
	 */
	if (h->origin != NULL && ORIGIN_REQUIRED) {
		i = isNeedleInHaystack(h->origin, "Origins.dat", 0);

		if (i < 0) {
			handshake_error("The origin requested from is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}
	} else {
		/**
		 * If you do not want to allow clients with no origin, then report
		 * an error here!
		 */
		if (ORIGIN_REQUIRED) {
			handshake_error("Client did not supply an origin in the header.", 
					ERROR_FORBIDDEN, n);
			return -1;		
		}
	}

	/**
	 * If all these conditions is true, then we assume that the websocket
	 * protocol that is trying to be reached is hixie-75.
	 */
	if (h->type == UNKNOWN && h->version == 0 && h->key1 == NULL 
			&& h->key2 == NULL && h->key3 == NULL && h->key == NULL
			&& h->upgrade != NULL && h->connection != NULL && h->host != NULL
			&& h->origin != NULL) {
		h->type = HIXIE75;
	}

	if (h->type == UNKNOWN) {
		h->type = HTTP;
	}

	/**
	 * We now check all the headers we recieved according to the protocol which
	 * is used, to assure that we are talking to a valid client. If no protocol 
	 * is selected, then we assume that the request is an ordinary HTTP 
	 * request.
	 */
	if (h->type == HTTP) {
		handshake_error("Received a HTTP Request. We are not yet ready to "
				"handle these.\n", ERROR_NOT_IMPL, n);
		return -1;
	}

	if (h->upgrade == NULL || h->host == NULL || h->connection == NULL) {
		handshake_error("The client did not send the required websocket headers.", 
				ERROR_BAD, n);
		return -1;	
	}

	if (strncasecmp("websocket", h->upgrade, 9) != 0) {
		handshake_error("Unknown upgrade value.", ERROR_BAD, n);
		return -1;
	}

	if (strstr(h->connection, "Upgrade") == NULL) {
		handshake_error("Unknown connection value.", ERROR_BAD, n);
		return -1;
	}

	if ( h->type == HYBI00 ) {
		/**
		 * Checking that all headers required has been catched and set in our
		 * header structure.
		 */
		if (h->key1 == NULL || h->key2 == NULL || h->key3 == NULL) {
			handshake_error("hybi-00: didn't receive all the headers required "
					" to evaluate if the handshake was valid", ERROR_BAD, n);
			return -1;
		}

		/**
		 * Generate the actual values of key1 and key2. 
		 */
		uint32_t key1 = ntohl(generateKey(h->key1, strlen(h->key1)));
		uint32_t key2 = ntohl(generateKey(h->key2, strlen(h->key2)));

		/**
		 * If the keys returned are zero or less, something went wrong. Else
		 * we generate the actual accept key using key1, key2 and key3.
		 */
		if (key1 > 0 && key2 > 0) {
			char unhashedKey[KEYSIZE];
			unsigned char hashedKey[KEYSIZE];
			concate(key1, key2, h->key3, unhashedKey);	

			MD5_CTX ctx;
			MD5_Init(&ctx);
			MD5_Update(&ctx,unhashedKey,KEYSIZE);
			MD5_Final(hashedKey,&ctx);
			h->accept = getMemory((char *) hashedKey, KEYSIZE);
			h->accept_len = KEYSIZE; 

			if (h->accept == NULL) {
				handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
				return -1;
			}
		} else {
			handshake_error("The keys sent in the header was invalid.", ERROR_BAD, 
					n);
			return -1;
		}

	} else if ( h->type == HYBI07 || h->type == RFC6455 || h->type == HYBI10 ) {
		/**
		 * Checking that all headers required has been catched
		 */
		if (h->key == NULL) {
			handshake_error("RFC6455: didn't receive all the headers required "
					" to evaluate if the handshake was valid", ERROR_BAD, n);
			return -1;
		}

		/**
		 * Creating acceptkey from the key we recieved in the headers. 
		 */
		SHA_CTX stx;
		char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		int magic_len = 36, length = magic_len + strlen(h->key);
		char key[length];
		unsigned char sha1Key[20];
		char *acceptKey=NULL;
		acceptKey = (char *)malloc (B64_BUFF_LEN);
		
		memset(key, '\0', length);
		memset(sha1Key, '\0', 20);

		memcpy(key, h->key, (length-magic_len));
		memcpy(key+(length-magic_len), magic, magic_len);

		SHA1_Init(&stx);
		SHA1_Update(&stx,(const unsigned char*) key,length);
		SHA1_Final(sha1Key,&stx);

		char sha1Key_padding[20 + 1];
		memset(sha1Key_padding, '\0', sizeof(sha1Key_padding));
		memcpy(sha1Key_padding, sha1Key, 20);
		Encode64nChar((char*)acceptKey, (char *)sha1Key_padding, B64_BUFF_LEN, 20);
		
		if (acceptKey == NULL) {
			handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}
		h->accept = acceptKey;
		h->accept_len = strlen(h->accept);

	} else if ( h->type != HIXIE75 ) {
		handshake_error("Something very wierd happened!?", ERROR_INTERNAL, n);
		return -1; 
	}

	return 0;
}

int sendHandshake(ws_client *n) {
	int memlen = 0, length = 0,ret=0;
	char* response = NULL;

	if ( n->headers->type == UNKNOWN || n->headers->type == HTTP ) {
		handshake_error("Type is not known, we fucked up somewhere.", 
				ERROR_INTERNAL, n);
		return -1;
	}

	if ( n->headers->type == HYBI07 || n->headers->type == RFC6455 || 
			n->headers->type == HYBI10 ) {
		length = ACCEPT_HEADER_V3_LEN  
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN
			+ ACCEPT_KEY_LEN + n->headers->accept_len 
			+ (2*3);
		if (n->headers->protocol != NONE) {
			length += ACCEPT_PROTOCOL_V2_LEN + n->headers->protocol_len+2;
		}
		response = getMemory("", length);
		
		if (response == NULL) {
			handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}
	
		memcpy(response + memlen, ACCEPT_HEADER_V3, ACCEPT_HEADER_V3_LEN);
		memlen += ACCEPT_HEADER_V3_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;
		
		if (n->headers->protocol != NONE) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V2, ACCEPT_PROTOCOL_V2_LEN);
			memlen += ACCEPT_PROTOCOL_V2_LEN;

			memcpy(response + memlen, n->headers->protocol_string, 
					n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, ACCEPT_KEY, ACCEPT_KEY_LEN);
		memlen += ACCEPT_KEY_LEN;
		
		memcpy(response + memlen, n->headers->accept, n->headers->accept_len);
		memlen += n->headers->accept_len;

		memcpy(response + memlen, "\r\n\r\n", 4);
		memlen += 4;

		if(response != NULL)
		{
			/* SCA Fix [String not null terminated]:: False Positive */ 
			/* Reason for False Positive - we limit the size to print even if it's not null-terminated. 
			 * And making response null-terminated will cause http response error */
			printf("Server responds with the following headers:\n%.*s\n", memlen, response);
			fflush(stdout);
		}

		if (memlen != length) {
			free(response);
			handshake_error("We've fucked the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	} else if ( n->headers->type == HYBI00 ) {
		length = ACCEPT_HEADER_V2_LEN  
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN 
			+ ACCEPT_LOCATION_V2_LEN + 5 
			+ n->headers->host_len
			+ n->headers->accept_len 
			+ n->headers->resourcename_len
			+ (2*3);
		if (n->headers->origin != NULL) {
			length += ACCEPT_ORIGIN_V2_LEN + n->headers->origin_len + 2;
		}
		if (n->headers->protocol != NONE) {
			length += ACCEPT_PROTOCOL_V2_LEN + n->headers->protocol_len + 2;
		}

		response = getMemory("", length);
		
		if (response == NULL) {
			handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}
	
		memcpy(response + memlen, ACCEPT_HEADER_V2, ACCEPT_HEADER_V2_LEN);
		memlen += ACCEPT_HEADER_V2_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;

		if (n->headers->origin != NULL) {
			memcpy(response + memlen, ACCEPT_ORIGIN_V2, ACCEPT_ORIGIN_V2_LEN);
			memlen += ACCEPT_ORIGIN_V2_LEN;

			memcpy(response + memlen, n->headers->origin, 
					n->headers->origin_len);
			memlen += n->headers->origin_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, ACCEPT_LOCATION_V2, ACCEPT_LOCATION_V2_LEN);
		memlen += ACCEPT_LOCATION_V2_LEN;

		memcpy(response + memlen, "ws://", 5);
		memlen += 5;

		memcpy(response + memlen, n->headers->host, n->headers->host_len);
		memlen += n->headers->host_len;

		memcpy(response + memlen, n->headers->resourcename, 
				n->headers->resourcename_len);
		memlen += n->headers->resourcename_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		if (n->headers->protocol != NONE) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V2, ACCEPT_PROTOCOL_V2_LEN);
			memlen += ACCEPT_PROTOCOL_V2_LEN;

			memcpy(response + memlen, n->headers->protocol_string, 
					n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, n->headers->accept, n->headers->accept_len);
		memlen += n->headers->accept_len;

		printf("Server responds with the following headers:\n%s\n", response);
		fflush(stdout);

		if (memlen != length) {
			free(response);
			handshake_error("We've fucked the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	} else if ( n->headers->type == HIXIE75 ) {
		length = ACCEPT_HEADER_V1_LEN 
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN
			+ ACCEPT_LOCATION_V1_LEN + 5 
			+ n->headers->host_len
			+ n->headers->resourcename_len
			+ (2*3);
		if (n->headers->origin != NULL) {
			length += ACCEPT_ORIGIN_V1_LEN + n->headers->origin_len + 2;
		}
		if (n->headers->protocol != NONE) {
			length += ACCEPT_PROTOCOL_V1_LEN + n->headers->protocol_len + 2;
		}

		response = getMemory("", length);
		
		if (response == NULL) {
			handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}
	
		memcpy(response + memlen, ACCEPT_HEADER_V1, ACCEPT_HEADER_V1_LEN);
		memlen += ACCEPT_HEADER_V1_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;

		if (n->headers->origin != NULL) {
			memcpy(response + memlen, ACCEPT_ORIGIN_V1, ACCEPT_ORIGIN_V1_LEN);
			memlen += ACCEPT_ORIGIN_V1_LEN;

			memcpy(response + memlen, n->headers->origin, 
					n->headers->origin_len);
			memlen += n->headers->origin_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, ACCEPT_LOCATION_V1, ACCEPT_LOCATION_V1_LEN);
		memlen += ACCEPT_LOCATION_V1_LEN;

		memcpy(response + memlen, "ws://", 5);
		memlen += 5;

		memcpy(response + memlen, n->headers->host, n->headers->host_len);
		memlen += n->headers->host_len;

		memcpy(response + memlen, n->headers->resourcename, 
				n->headers->resourcename_len);
		memlen += n->headers->resourcename_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		if (n->headers->protocol != NONE) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V1, 
					ACCEPT_PROTOCOL_V1_LEN);
			memlen += ACCEPT_PROTOCOL_V1_LEN;

			memcpy(response + memlen, n->headers->protocol_string, 
					n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}
		
		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		if(response != NULL)
		{
			/* SCA Fix [String not null terminated]:: False Positive */ 
			/* Reason for False Positive - we limit the size to print even if it's not null-terminated. 
			 * And making response null-terminated will cause http response error */
			printf("Server responds with the following headers:\n%.*s\n", memlen, response);
			fflush(stdout);
		}

		if (memlen != length) {
			free(response);
			handshake_error("We've fucked the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	}

	ret=send(n->socket_id, response, length, 0);
	if(ret == -1)
	{
		printf("Error in send function\n");
		if (response != NULL) {
			free(response);
			response = NULL;
		}
	}

	if (response != NULL) {
		free(response);
	}

	return 0;
}
