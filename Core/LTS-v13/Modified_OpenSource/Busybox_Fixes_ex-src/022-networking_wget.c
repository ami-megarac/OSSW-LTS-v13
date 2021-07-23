--- busybox.org/networking/wget.c	2019-01-15 14:49:24.888268797 +0800
+++ busybox/networking/wget.c	2019-01-15 13:57:52.247434081 +0800
@@ -457,7 +457,7 @@
 		G.content_len = BB_STRTOOFF(G.wget_buf + 4, NULL, 10);
 		if (G.content_len < 0 || errno) {
 			set_status(FW_UPDATE_PATH_NOT_FOUND);
-			bb_error_msg_and_die("SIZE value is garbage");
+			bb_error_msg_and_die("bad SIZE value '%s'", G.wget_buf + 4);
 		}
 		G.got_clen = 1;
 	}
@@ -619,8 +619,11 @@
 		fgets_and_trim(dfp); /* Eat empty line */
  get_clen:
 		fgets_and_trim(dfp);
+		errno = 0;
 		G.content_len = STRTOOFF(G.wget_buf, NULL, 16);
 		/* FIXME: error check? */
+		if (G.content_len < 0 || errno)
+			bb_error_msg_and_die("bad chunk length '%s'", G.wget_buf);		
 		if (G.content_len == 0)
 			break; /* all done! */
 		G.got_clen = 1;
