--- busybox.org/archival/libarchive/decompress_gunzip.c	2019-01-15 14:20:08.642680201 +0800
+++ busybox/archival/libarchive/decompress_gunzip.c	2019-01-15 14:22:29.391100249 +0800
@@ -305,11 +305,11 @@
 	unsigned i;             /* counter, current code */
 	unsigned j;             /* counter */
 	int k;                  /* number of bits in current code */
-	unsigned *p;            /* pointer into c[], b[], or v[] */
+	const unsigned *p;      /* pointer into c[], b[], or v[] */
 	huft_t *q;              /* points to current table */
 	huft_t r;               /* table entry for structure assignment */
 	huft_t *u[BMAX];        /* table stack */
-	unsigned v[N_MAX];      /* values in order of bit length */
+	unsigned v[N_MAX+1];    /* values in order of bit length */
 	int ws[BMAX + 1];       /* bits decoded stack */
 	int w;                  /* bits decoded */
 	unsigned x[BMAX + 1];   /* bit offsets, then code stack */
@@ -324,7 +324,7 @@
 
 	/* Generate counts for each bit length */
 	memset(c, 0, sizeof(c));
-	p = (unsigned *) b; /* cast allows us to reuse p for pointing to b */
+	p = b;
 	i = n;
 	do {
 		c[*p]++; /* assume all entries <= BMAX */
@@ -365,7 +365,7 @@
 	}
 
 	/* Make a table of values in order of bit lengths */
-	p = (unsigned *) b;
+	p = b;
 	i = 0;
 	do {
 		j = *p++;
@@ -432,7 +432,9 @@
 
 			/* set up table entry in r */
 			r.b = (unsigned char) (k - w);
-			if (p >= v + n) {
+			if (/*p >= v + n || -- redundant, caught by the second check: */
+				*p == UINT_MAX /* do we access uninited v[i]? (see memset(v))*/
+			) {
 				r.e = 99; /* out of values--invalid code */
 			} else if (*p < s) {
 				r.e = (unsigned char) (*p < 256 ? 16 : 15);	/* 256 is EOB code */
