#ifndef __FLEX_TABLES_H
#define __FLEX_TABLES_H

#include <stdlib.h>
#include <stdint.h>

#define YYTH_MAGIC	0xF13C57B1
#define YYTH_FLAG_DIFF_ENCODE 1

struct table_set_header {
	uint32_t	th_magic;	/* TH_MAGIC */
	uint32_t	th_hsize;
	uint32_t	th_ssize;
	uint16_t	th_flags;
/*	char		th_version[];
	char		th_name[];
	char		th_pad64[];*/
} __attribute__ ((packed));

#define	YYTD_ID_ACCEPT	1
#define YYTD_ID_BASE	2
#define YYTD_ID_CHK	3
#define YYTD_ID_DEF	4
#define YYTD_ID_EC	5
#define YYTD_ID_META	6
#define YYTD_ID_ACCEPT2 7
#define YYTD_ID_NXT	8

#define YYTD_DATA8	1
#define YYTD_DATA16	2
#define YYTD_DATA32	4

struct table_header {
	uint16_t	td_id;
	uint16_t	td_flags;
	uint32_t	td_hilen;
	uint32_t	td_lolen;
/*	char		td_data[];
	char		td_pad64[];*/
} __attribute__ ((packed));

#endif
