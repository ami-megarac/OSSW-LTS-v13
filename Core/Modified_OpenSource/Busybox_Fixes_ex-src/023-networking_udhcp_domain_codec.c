--- busybox.org/networking/udhcp/domain_codec.c	2019-01-15 13:59:53.819557205 +0800
+++ busybox/networking/udhcp/domain_codec.c	2019-01-15 14:01:51.529514288 +0800
@@ -63,11 +63,10 @@
 				if (crtpos + *c + 1 > clen) /* label too long? abort */
 					return NULL;
 				if (dst)
-					memcpy(dst + len, c + 1, *c);
+					/* \3com ---> "com." */
+					((char*)mempcpy(dst + len, c + 1, *c))[0] = '.';
 				len += *c + 1;
 				crtpos += *c + 1;
-				if (dst)
-					dst[len - 1] = '.';
 			} else {
 				/* NUL: end of current domain name */
 				if (retpos == 0) {
@@ -78,7 +77,10 @@
 					crtpos = retpos;
 					retpos = depth = 0;
 				}
-				if (dst)
+				if (dst && len != 0)
+					/* \4host\3com\0\4host and we are at \0:
+					 * \3com was converted to "com.", change dot to space.
+					 */
 					dst[len - 1] = ' ';
 			}
 
@@ -228,6 +230,9 @@
 	int len;
 	uint8_t *encoded;
 
+    uint8_t str[6] = { 0x00, 0x00, 0x02, 0x65, 0x65, 0x00 };
+    printf("NUL:'%s'\n",   dname_dec(str, 6, ""));	
+	
 #define DNAME_DEC(encoded,pre) dname_dec((uint8_t*)(encoded), sizeof(encoded), (pre))
 	printf("'%s'\n",       DNAME_DEC("\4host\3com\0", "test1:"));
 	printf("test2:'%s'\n", DNAME_DEC("\4host\3com\0\4host\3com\0", ""));
