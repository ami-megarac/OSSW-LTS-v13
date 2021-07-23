
/*-------------------------------------------------------------------------*/
/**
  @file     strlib.h
  @author   N. Devillard
  @date     Jan 2001
  @version  $Revision: 1.4 $
  @brief    Various string handling routines to complement the C lib.

  This modules adds a few complementary string routines usually missing
  in the standard C library.
*/
/*--------------------------------------------------------------------------*/

/*
	$Id: strlib.h,v 1.4 2006-09-27 11:04:11 ndevilla Exp $
	$Author: ndevilla $
	$Date: 2006-09-27 11:04:11 $
	$Revision: 1.4 $
*/

#ifndef _STRLIB_H_
#define _STRLIB_H_

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"{
#endif

#if defined(WIN32) || defined(WIN64)
#if _MSC_VER < 1900 //vs2015 already have this function
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif
#define spstrcasecmp _stricmp 
#define FUNCTION_NAME __FUNCTION__
#else
#define SNPRINTF snprintf
#define FUNCTION_NAME __func__
#define spstrcasecmp strcasecmp
#endif
/*---------------------------------------------------------------------------
  							Function codes
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  @brief    Convert a string to lowercase.
  @param    s   String to convert.
  @param	l	Output Buffer
  @param	size Output buffer size
  @return   ptr to statically allocated string.

  This function returns a pointer to a statically allocated string
  containing a lowercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * strlwc(const char * s, char * l, int size);

/*-------------------------------------------------------------------------*/
/**
  @brief    Convert a string to uppercase.
  @param    s   String to convert.
  @param	l	Output buffer
  @param	size	Output buffer size
  @return   ptr to statically allocated string.

  This function returns a pointer to a statically allocated string
  containing an uppercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * strupc(char * s, char * l, int size);

/*-------------------------------------------------------------------------*/
/**
  @brief    Skip blanks until the first non-blank character.
  @param    s   String to parse.
  @return   Pointer to char inside given string.

  This function returns a pointer to the first non-blank character in the
  given string.
 */
/*--------------------------------------------------------------------------*/
char * strskp(char * s);

/*-------------------------------------------------------------------------*/
/**
  @brief    Remove blanks at the end of a string.
  @param    s   String to parse.
  @return   ptr to statically allocated string.

  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * strcrop(char * s);

/*-------------------------------------------------------------------------*/
/**
  @brief    Remove blanks at the beginning and the end of a string.
  @param    s   String to parse.
  @param	l Output buffer
  @param	size output buffer size
  @return   ptr to statically allocated string.

  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end and the beg. of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * strstrip(char * s, char * l, int size) ;

#ifdef __cplusplus
}
#endif
#endif
