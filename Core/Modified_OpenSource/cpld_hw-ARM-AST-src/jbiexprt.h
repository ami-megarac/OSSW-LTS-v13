/****************************************************************************/
/*																			*/
/*	Module:			jbiexprt.h												*/
/*																			*/
/*					Copyright (C) Altera Corporation 1998-2001				*/
/*																			*/
/*	Description:	Jam STAPL ByteCode Player Export Header File			*/
/*																			*/
/*	Revisions:																*/
/*																			*/
/****************************************************************************/

#ifndef INC_JBIEXPRT_H
#define INC_JBIEXPRT_H

/*
*	Return codes from most JBI functions
*/

#define JBI_RETURN_TYPE int

#define JBIC_SUCCESS            0
#define JBIC_OUT_OF_MEMORY      1
#define JBIC_IO_ERROR           2
/* #define JAMC_SYNTAX_ERROR       3 */
#define JBIC_UNEXPECTED_END     4
#define JBIC_UNDEFINED_SYMBOL   5
/* #define JAMC_REDEFINED_SYMBOL   6 */
#define JBIC_INTEGER_OVERFLOW   7
#define JBIC_DIVIDE_BY_ZERO     8
#define JBIC_CRC_ERROR          9
#define JBIC_INTERNAL_ERROR    10
#define JBIC_BOUNDS_ERROR      11
/* #define JAMC_TYPE_MISMATCH     12 */
/* #define JAMC_ASSIGN_TO_CONST   13 */
/* #define JAMC_NEXT_UNEXPECTED   14 */
/* #define JAMC_POP_UNEXPECTED    15 */
/* #define JAMC_RETURN_UNEXPECTED 16 */
/* #define JAMC_ILLEGAL_SYMBOL    17 */
#define JBIC_VECTOR_MAP_FAILED 18
#define JBIC_USER_ABORT        19
#define JBIC_STACK_OVERFLOW    20
#define JBIC_ILLEGAL_OPCODE    21
/* #define JAMC_PHASE_ERROR       22 */
/* #define JAMC_SCOPE_ERROR       23 */
#define JBIC_ACTION_NOT_FOUND  24

/*
*	Macro Definitions
*/
#define PROGRAM_PTR unsigned char *

#define GET_BYTE(x) (program[x])

#define GET_WORD(x) \
	(((((unsigned short) GET_BYTE(x)) << 8) & 0xFF00) | \
	(((unsigned short) GET_BYTE((x)+1)) & 0x00FF))

#define GET_DWORD(x) \
	(((((unsigned long) GET_BYTE(x)) << 24L) & 0xFF000000L) | \
	((((unsigned long) GET_BYTE((x)+1)) << 16L) & 0x00FF0000L) | \
	((((unsigned long) GET_BYTE((x)+2)) << 8L) & 0x0000FF00L) | \
	(((unsigned long) GET_BYTE((x)+3)) & 0x000000FFL))

/*
*	Structured Types
*/

typedef struct JBI_PROCINFO_STRUCT
{
	char *name;
	unsigned char attributes;
	struct JBI_PROCINFO_STRUCT *next;
}
JBI_PROCINFO;

/*
*	Global Data Prototypes
*/

extern PROGRAM_PTR jbi_program;

extern char *jbi_workspace;

extern long jbi_workspace_size;

/*
*	Function Prototypes
*/

JBI_RETURN_TYPE jbi_execute
(
	PROGRAM_PTR program,
	long program_size,
	char *workspace,
	long workspace_size,
	char *action,
	char **init_list,
	int reset_jtag,
	long *error_address,
	int *exit_code,
	int *format_version
);

JBI_RETURN_TYPE jbi_check_crc
(
	PROGRAM_PTR program,
	long program_size,
	unsigned short *expected_crc,
	unsigned short *actual_crc
);

int jbi_jtag_io
(
	int tms,
	int tdi,
	int read_tdo
);

void jbi_message
(
	char *message_text
);

void jbi_delay
(
	long microseconds
);


int jbi_vector_io
(
	int signal_count,
	long *dir_vect,
	long *data_vect,
	long *capture_vect
);

void *jbi_malloc
(
	unsigned int size
);

void jbi_free
(
	void *ptr
);

#endif /* INC_JBIEXPRT_H */
