--- busybox.org/libbb/lineedit.c	2019-01-15 13:58:35.067205738 +0800
+++ busybox/libbb/lineedit.c	2019-01-15 13:59:03.457802112 +0800
@@ -627,6 +627,18 @@
 
 static void add_match(char *matched)
 {
+	unsigned char *p = (unsigned char*)matched;
+	while (*p) {
+		/* ESC attack fix: drop any string with control chars */
+		if (*p < ' '
+		 || (!ENABLE_UNICODE_SUPPORT && *p >= 0x7f)
+		 || (ENABLE_UNICODE_SUPPORT && *p == 0x7f)
+		) {
+			free(matched);
+			return;
+		}
+		p++;
+	}	
 	matches = xrealloc_vector(matches, 4, num_matches);
 	matches[num_matches] = matched;
 	num_matches++;
