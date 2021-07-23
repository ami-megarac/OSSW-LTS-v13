
/*-------------------------------------------------------------------------*/
/**
  @file		strlib.c
  @author	N. Devillard
  @date		Jan 2001
  @version	$Revision: 1.9 $
  @brief	Various string handling routines to complement the C lib.

  This modules adds a few complementary string routines usually missing
  in the standard C library.
*/
/*--------------------------------------------------------------------------*/

/*
	$Id: strlib.c,v 1.9 2006-09-27 11:04:11 ndevilla Exp $
	$Author: ndevilla $
	$Date: 2006-09-27 11:04:11 $
	$Revision: 1.9 $
*/

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/

#include <string.h>
#include <ctype.h>

#include "strlib.h"

/*---------------------------------------------------------------------------
   							    Defines	
 ---------------------------------------------------------------------------*/
#define ASCIILINESZ	1024

/*---------------------------------------------------------------------------
  							Function codes
 ---------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/**
  @brief	Convert a string to lowercase.
  @param	s	String to convert.
  @param	l Output  buffer
  @param	size	output buffer size
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string
  containing a lowercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
/*--------------------------------------------------------------------------*/

char * strlwc(const char * s, char *l, int size)
{
    int i ;

    if ((s==NULL) || (l==NULL)) return NULL ;
    memset(l, 0, size);
    i=0 ;
    while (s[i] && i<size-1) {
        l[i] = (char)tolower((int)s[i]);
        i++ ;
    }
    l[i]=(char)0;
    return l ;
}



/*-------------------------------------------------------------------------*/
/**
  @brief	Convert a string to uppercase.
  @param	s	String to convert.
  @param	l	Output buffer
  @param	size Output buffer size
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string
  containing an uppercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
/*--------------------------------------------------------------------------*/

char * strupc(char * s, char * l, int size)
{
    int i ;

    if ((s==NULL) || (l == NULL)) return NULL ;
    memset(l, 0, size);
    i=0 ;
    while (s[i] && i<size-1) {
        l[i] = (char)toupper((int)s[i]);
        i++ ;
    }
    l[i]=(char)0;
    return l ;
}



/*-------------------------------------------------------------------------*/
/**
  @brief	Skip blanks until the first non-blank character.
  @param	s	String to parse.
  @return	Pointer to char inside given string.

  This function returns a pointer to the first non-blank character in the
  given string.
 */
/*--------------------------------------------------------------------------*/

char * strskp(char * s)
{
    char * skip = s;
	if (s==NULL) return NULL ;
    while (isspace((int)*skip) && *skip) skip++;
    return skip ;
} 



/*-------------------------------------------------------------------------*/
/**
  @brief	Remove blanks at the end of a string.
  @param	s	String to parse.
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
/*--------------------------------------------------------------------------*/

char * strcrop(char * s)
{
	char * last ;

	    if (s==NULL) return NULL ;
	last = s + strlen(s);
	while (last > s) {
	if (!isspace((int)*(last-1)))
	break ;
	last -- ;
	}
	*last = (char)0;
	    return s;
}



/*-------------------------------------------------------------------------*/
/**
  @brief	Remove blanks at the beginning and the end of a string.
  @param	s	String to parse.
  @param	l output buffer
  @param	size	Output buffer size
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end and the beg. of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * strstrip(char * s, char * l, int size)
{
	char * last ;
	
    if ((s==NULL) || (l==NULL)) return NULL ;
    
	while (isspace((int)*s) && *s) s++;
	
	memset(l, 0, size);
	strncpy(l, s,size-1);
	last = l + strlen(l);
	while (last > l) {
		if (!isspace((int)*(last-1)))
			break ;
		last -- ;
	}
	*last = (char)0;

	return (char*)l ;
}

/* Test code */
#ifdef TEST
int main(int argc, char * argv[])
{
	char * str ;
	char tmp_buf[ASCIILINESZ+1];

	str = "\t\tI'm a lumberkack and I'm OK      " ;
	printf("lowercase: [%s]\n", strlwc(str, tmp_buf, sizeof(tmp_buf)));
	printf("uppercase: [%s]\n", strupc(str, tmp_buf, sizeof(tmp_buf)));
	printf("skipped  : [%s]\n", strskp(str));
	printf("cropped  : [%s]\n", strcrop(str));
	printf("stripped : [%s]\n", strstrip(str, tmp_buf, sizeof(tmp_buf)));

	return 0 ;
}
#endif
/* vim: set ts=4 et sw=4 tw=75 */
