/********************************************************************
 * ******************************************************************
 * ***                                                            ***
 * ***        (C)Copyright 2013, American Megatrends Inc.         ***
 * ***                                                            ***
 * ***                    All Rights Reserved                     ***
 * ***                                                            ***
 * ***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
 * ***                                                            ***
 * ***                     Phone 770.246.8600                     ***
 * ***                                                            ***
 * ******************************************************************
 * ******************************************************************
 * ******************************************************************
 *
 * FileName    : iniparser.h
 *
 * Description : parser for ini files
 *
 * Author      : Manoj Ashok <manoja@amiindia.co.in>
 *
 ********************************************************************/

#ifndef __INIPARSER_H__
#define __INIPARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASCIILINESZ     (1024)
#define KEYSIZESZ       (128)
#define INI_INVALID_KEY     ((char*)-1)
#define HASH_TABLE_SIZE (0x2000)
#define TRUE 1
#define FALSE 0
#define INI_INVALID_KEY     ((char*)-1)
#define MAX_CONF_STR_LENGTH 256
#define VERSION_SECTION     "version"
#define VERSION_KEYNAME     "version"
#define CFG_NOT_UPDATED   0
#define CFG_UPDATED       1

//Macros to maintain backward compatibility
#define dictionary INIHandler
#define iniparser_freedict iniparser_close
#define iniparser_unset iniparser_delentry

/* Structures used by the parser */
#ifdef __cplusplus
extern "C"{
#endif
// the structure for hash entry
typedef struct nvpair {
        char *key;  /* key is combination of "section:name" */
        char *val;
        struct nvpair *next;
} NVP_T;

// the structure to store the section names
typedef struct section {
        char *name;
        struct section *next;
} SEC_T;

// structure to store the ini file data
typedef struct ini_handler {
	NVP_T **hashTable;
	SEC_T *secTable;
	int sec_count;
} INIHandler;

/* Function Name: hasher31
 *
 * This function uses the algorithm k=31 to generate the hash
 * values used by the hash table to store the data
 */
unsigned long hasher31(char *str);

/* Function Name: iniparser_add_entry
 * 
 * This function adds the entry to hashTable in the INIHandler.
 * The key is framed as section:prop and added in the hash table
 * If the Prop value is NULL then the section name is added to the
 * secTable.
 */
void iniparser_add_entry(INIHandler *handler, char *sec, char *Prop, char * val);

/* Function Name: iniparser_add_section
 * This function adds the section name to the secTable in the INIHandler
 */
void iniparser_add_section(INIHandler *handler, char * name);

/* Function Name: iniparser_setstring
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found, a new
 * entry is added to the INIHandler's hashTable. If key is found the value
 * is overridden with the value in val pointer.
 */
void iniparser_setstring(INIHandler *handler, char *hashstr, char * val);
int iniparser_setstr(INIHandler *handler, char *hashstr, char * val);
//Get Functions
/* Function Name: iniparser_getnsec
 *
 * This function returns number of sections in the INIHandler.
 * Returns -1 in case of an error.
 */
int iniparser_getnsec(INIHandler *handler);

/* Function Name: iniparser_getsecname
 *
 * This function locates the n-th section in a INIHandler and returns
 * its name as a pointer to a string in the INIHandler's secTable structure 
 * Note:
 *     Do not free or modify the returned string!
 */
char * iniparser_getsecname(INIHandler *handler, int n);

/* Function Name: iniparser_getstring
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned.
 * Note:
 *   The returned char pointer is pointing to a string allocated in
 *   the dictionary, do not free or modify it.
 */
char * iniparser_getstring(INIHandler *handler, char *key, char *defVal);

/* Function Name: iniparser_getstr
 *
 * This function also queries the INIHandler for a key. A key as read 
 * from an ini file is given as "section:key". If the key cannot be 
 * found, NULL is returned. This function is maintained for backward 
 * compatibility
 * Note:
 *   The returned char pointer is pointing to a string allocated in
 *   the dictionary, do not free or modify it. 
 */
char * iniparser_getstr(INIHandler *handler, char *key);

/* Function Name: iniparser_getchar
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the char value is returned.
 */
char iniparser_getchar(INIHandler *handler, char *key, char defVal);

/* Function Name: iniparser_getboolean
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the boolean value 
 * (TRUE or FALSE) is returned.
 */
int iniparser_getboolean(INIHandler *handler, char *key, int defVal);

/* Function Name: iniparser_getint
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the integer value found
 * is returned.
 */
int iniparser_getint(INIHandler *handler, char *key, int defVal);

/* Function Name: iniparser_getuint
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the unsigned integer value
 * found is returned.
 */
unsigned int iniparser_getuint(INIHandler *handler, char *key, unsigned int defVal);

/* Function Name: iniparser_getshort
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the short integer value
 * found is returned.
 */
short iniparser_getshort(INIHandler *handler, char *key, short defVal);

/* Function Name: iniparser_getlong
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the long integer value
 * found is returned.
 */
long iniparser_getlong(INIHandler *handler, char *key, long defVal);

/* Function Name: iniparser_getdouble
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the double value
 * found is returned.
 */
double iniparser_getdouble(INIHandler *handler, char *key, double defVal);


//Load and Unload functions

/* Function Name: iniparser_load
 *
 * The function loads the data in the input file to the hash table
 * in the returned INIHandler. In case of failure it returns NULL.
 */
INIHandler*  iniparser_load(const char * ininame);

/* Function Name: iniparser_loaddef
 *
 * The function loads the data by merging the two input file to the 
 * hash table in the returned INIHandler. In case of failure it returns 
 * NULL.
 */
INIHandler*  iniparser_loaddef(const char * def_ininame, const char * ininame);

/* Function Name: iniparser_close
 *
 * The function deallocates the memory used by the INIHandler
 */
void iniparser_close(INIHandler *handler);

//Free Functions - 
void Free_NVP(NVP_T *pnvp);
void Free_SEC(SEC_T *psec);

/* Function Name: iniparser_dump_ini
 *
 * This function dumps a given INIHandler into a loadable ini file.
 * It is Ok to specify stderr or stdout as output files
 */
void iniparser_dump_ini(INIHandler *handler, FILE *out);

/* Function Name: iniparser_dump
 *
 * This function dumps a given dictionary into a loadable ini file.
 * It is Ok to specify @c stderr or @c stdout as output files
 */
void iniparser_dump(INIHandler *handler, FILE *out);

/* Function Name: iniparser_dump_file
 *
 * This function dumps a given INIHandler into a loadable ini file.
 * It is Ok to specify stderr or stdout as output files
 */
int iniparser_dump_file(INIHandler *handle, char *filename);

//Debug function - prints the hash table and section table in the stdout
void print_tab(INIHandler *handler);

/* Function Name: iniparser_find_entry
 *
 * The function returns TRUE if the key value is found in the hash table 
 * or in the section table, else returns FALSE.
 */
int iniparser_find_entry(INIHandler *handler, char *key);

/* Function Name: iniparser_findsection
 *
 * The function returns TRUE if the name is found in the section table 
 * else returns FALSE.
 */
int iniparser_findsection(INIHandler *handler, char *name);

/* Function Name: iniparser_delentry
 *
 * This function deletes the values specified by the char pointer, entry from
 * INIHandler. If that pointer is a section name, then it deletes the whole section
 * from the handler. If the value is a combination of section:property then that 
 * entry is removed.
 */
void iniparser_delentry(INIHandler *handler, char *entry);

/* Function Name: iniparser_getseckeys
 * This function queries a dictionary and finds all keys in a given section.
 * Each pointer in the returned char pointer-to-pointer is pointing to
 * a string allocated in the dictionary; do not free or modify them,but free
 * the memory pointed by pointer-to-pointer.
 * 
 * This function returns NULL in case of error.
 */
char** iniparser_getseckeys(dictionary * d, char * s);

/* Function Name: iniparser_getsecnkeys

  @brief    Get the number of keys in a section of a dictionary.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   Number of keys in section
*/
int iniparser_getsecnkeys(dictionary * d, char * s);

#ifdef __cplusplus
}
#endif
#endif
