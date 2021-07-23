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
 * FileName    : iniparser.c
 *
 * Description : parser for ini files
 *
 * Author      : Manoj Ashok <manoja@amiindia.co.in>
 *
 ********************************************************************/


#include <stdio.h>
#include <string.h>
#include "strlib.h"
#include "iniparser.h"

//#define TEST_PARSER

/* Function Name: hasher31
 *
 * This function uses the algorithm k=31 to generate the hash
 * values used by the hash table to store the data
 */
unsigned long hasher31(char *str)
{
    unsigned long hash = 5381;
    int tmp = 0;

    while ((tmp = *str++)) {
        hash = ((hash<<5) + hash) + tmp;
    }

    return hash % HASH_TABLE_SIZE;
}

/* Function Name: iniparser_load
 *
 * The function loads the data in the input file to the hash table
 * in the returned INIHandler. In case of failure it returns NULL.
 */

INIHandler*  iniparser_load(const char * ininame)
{
    return (iniparser_loaddef(NULL, ininame));
}

/* Function Name: iniparser_loaddef
 *
 * The function loads the data by merging the two input file to the 
 * hash table in the returned INIHandler. In case of failure it returns 
 * NULL.
 */

INIHandler*  iniparser_loaddef(const char * def_ininame, const char * ininame)
{
    char        lin[ASCIILINESZ+1];
    char        sec[ASCIILINESZ+1];
    char        key[ASCIILINESZ+1];
    char        val[ASCIILINESZ+1];
    char    *   where ;
    FILE    *   ini = NULL ;
    FILE    *   def_ini = NULL ;
    INIHandler *handler = NULL;
    unsigned char Len = 0;
#ifdef INI_LOWERCASE
    char        tmp_buf[ASCIILINESZ+1] = {0};
    int lwc_count = 0;
#endif
    
    if(def_ininame != NULL)
        def_ini = fopen(def_ininame, "r") ;

    if(ininame != NULL)
        ini = fopen(ininame, "r") ;

    if ( (NULL == def_ini) && (NULL == ini) )
        return NULL;

    handler = calloc(1, sizeof(INIHandler));
    if(handler != NULL) {
        handler->hashTable = calloc(HASH_TABLE_SIZE, sizeof(NVP_T *));
        if(!handler->hashTable)
        {
            if(def_ini != NULL)
            {
                fclose (def_ini);
            }
            if(ini != NULL)
            {
                fclose (ini);
            }
            free(handler);
            return NULL;
        }
        handler->secTable = NULL;
        handler->sec_count = 0;
    }
    else 
    {
        if(def_ini != NULL)
        {
            fclose (def_ini);
        }
        if(ini != NULL)
        {
            fclose (ini);
        }
        return NULL;
    }
    sec[0]=0;

    while ( (def_ini && (fgets(lin, ASCIILINESZ, def_ini) != NULL)) || (ini && (fgets(lin, ASCIILINESZ, ini) != NULL)) ) { /* Fortify [Buffer overflow]:: False Positive */
        where = strskp(lin); /* Skip leading spaces */
        if (*where==';' || *where=='#' || *where==0)
            continue ; /* Comment lines */
        else {
            if (sscanf(where, "[%[^]]", sec)==1) {
                /* Valid section name */
#ifdef INI_LOWERCASE
		    lwc_count = SNPRINTF(sec , sizeof(sec), "%s", strlwc(sec, tmp_buf, sizeof(tmp_buf)));
		    if(lwc_count < 0 || lwc_count >= (int)sizeof(sec)) {
			    printf("Buffer Overflow in File:%s Line: %d \n", \
				   __FILE__, __LINE__);
			    if(handler != NULL) {
				    if(!handler->hashTable)
					    free(handler->hashTable);
				    free(handler);
			    }
			    if ( NULL != ini )
				    fclose(ini);			    
			    if ( NULL != def_ini )
				    fclose(def_ini);
			    return NULL;
		    }
#endif
                iniparser_add_entry(handler, sec, NULL, NULL);
            } else if (strstr (where, "=") == NULL) {
                continue;    // Some Junk line no need to process. but if the junk has = what todo? :)
            }
            else if (sscanf (where, "%[^=]=%[^;#]",     key, val) == 2) {
#ifdef INI_LOWERCASE
		    strcrop(key);
		    lwc_count = SNPRINTF(key, sizeof(key), "%s", strlwc(key, tmp_buf, sizeof(tmp_buf)));
		    if(lwc_count < 0 || lwc_count >= (int)sizeof(sec)) {
			    printf("Buffer Overflow in File:%s Line: %d \n", \
				   __FILE__, __LINE__);	
			    if(handler != NULL) {
				    if(!handler->hashTable)
					    free(handler->hashTable);
				    free(handler);
			    }
			    if ( NULL != ini )
				    fclose(ini);
			    if ( NULL != def_ini )
				    fclose(def_ini);
			    return NULL;
		    }
#endif
                Len = strlen(val);

                if(((val[0] == '\"') && ((val[Len-1] == '\"') || (val[Len-2] == '\"')))
                  ||((val[0] == '\'') && ((val[Len-1] =='\'') || (val[Len-2] == '\''))))

                {
                    if ((sscanf (where, "%[^=]=\"%[^\"]\"", key, val) == 2)
                       ||(sscanf (where, "%[^=]='%[^\']'",   key, val) == 2))
                    {
                    }
                }

                /*
                 * sscanf cannot handle "" or '' as empty value,
                 * this is done here
                 */
                if (!strcmp(val, "\"\"") || !strcmp(val, "''")) {
                    val[0] = (char)0;
                } else {
		    if (strlen(strcrop(val)) + 1 > sizeof(val))
        	    {
                	printf ("Buffer size very small");
        	    }
	            strcrop(val);
                }
                iniparser_add_entry(handler, sec, key, val);
            }
        }
    }

    if ( NULL != ini )
        fclose(ini);

    if ( NULL != def_ini )
        fclose(def_ini);
    return handler;
}

/* Function Name: iniparser_close
 *
 * The function deallocates the memory used by the INIHandler
 */
void iniparser_close(INIHandler *handler)
{
    int i;
    if(!handler)
        return;
    for (i = 0; i <  HASH_TABLE_SIZE; i++) {
        Free_NVP(handler->hashTable[i]);
    }
    free(handler->hashTable);
    handler->hashTable = NULL;
    Free_SEC(handler->secTable);
    free(handler);
    handler = NULL;
}

/* Function Name: iniparser_add_section
 * This function adds the section name to the secTable in the INIHandler
 */
void iniparser_add_section(INIHandler *handler, char * name)
{
    SEC_T * sec_ptr = NULL;
    SEC_T * tmp_ptr = NULL;
    SEC_T * sectionTable = NULL;
    if(!handler || !name || (name[0] == '\0'))
        return;
    if (strnlen(name, MAX_CONF_STR_LENGTH) >= MAX_CONF_STR_LENGTH) {
        return;
    }
    sectionTable = handler->secTable;
    if(sectionTable == NULL) {
        if ( NULL == (sec_ptr = calloc(1, sizeof(SEC_T))) )
            return;
        if ((sec_ptr->name = strdup(name)) == NULL) {
            free(sec_ptr);
            return;
        }
        sec_ptr->next = NULL;
        sectionTable = sec_ptr;
        handler->sec_count = 0;
    }
    else
    {
// allocate memory only if it is necessary
        tmp_ptr = sectionTable;
        while(tmp_ptr != NULL) {
            if(strcmp(name, tmp_ptr->name) == 0) {
                        return;
                }
            if(tmp_ptr->next == NULL) {
                if ( NULL == (sec_ptr = calloc(1, sizeof(SEC_T))) )
                    return;
                if ((sec_ptr->name = strdup(name)) == NULL) {
                    free(sec_ptr);
                    return;
                }
                sec_ptr->next = NULL;
                tmp_ptr->next = sec_ptr;
                break;
            }
            tmp_ptr = tmp_ptr->next;
        }
    }
    handler->secTable = sectionTable;
    handler->sec_count++;
    return;
}

/* Function Name: iniparser_add_entry
 * 
 * This function adds the entry to hashTable in the INIHandler.
 * The key is framed as section:prop and added in the hash table
 * If the Prop value is NULL then the section name is added to the
 * secTable.
 */
void iniparser_add_entry(INIHandler *handler, char *sec, char *key, char * val)
{
    char hashstr[ASCIILINESZ+1] = {0};
    if(!handler || !sec )
        return;
    if (key!=NULL) {
        if(SNPRINTF(hashstr, sizeof(hashstr),"%s:%s", sec, key) >= (signed)sizeof(hashstr))
        {
            printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
            return;
        }
    } else {
        iniparser_add_section(handler, sec);
        return;
    }
    iniparser_setstring(handler, hashstr, val);
    return;
}

/* Function Name: iniparser_setstring
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found, a new
 * entry is added to the INIHandler's hashTable. If key is found the value
 * is overridden with the value in val pointer.
 */
void iniparser_setstring(INIHandler *handler, char *hashstr, char * val)
{
	int retval = 0;
        unsigned long hashval = 0;
        NVP_T *pnvp = NULL;
        NVP_T **hashTable = NULL;
        char writeVal[ASCIILINESZ+1] = { 0 }; /* new temporary buffer for write val */
        if(!handler || !(handler->hashTable) || (!hashstr) || (hashstr[0] == '\0')) {
                        return;
        }
	
	/*we shouldn't check for the size of val as this causes issue because size of val may be 256 also*/
	/*for eg: set username or Domain name in RIS configuration command in which max bytes is 256*/
        /*source Path data lenght can be more that 256 as it will be encrypted*/

        /* Copy val to local buffer to avoid encountering changes to its contents during execution of this function */
        if (val) {
            retval = SNPRINTF(writeVal, sizeof(writeVal),"%s",val);
	    if(retval < 0 || retval >= (signed int)sizeof(writeVal))
	    {
		    printf("Buffer Overflow in iniparser_setstring function\n");
		    return;
	    }
        }
        hashTable = handler->hashTable;
        hashval = hasher31(hashstr);
    if(hashTable[hashval] == NULL) {
        pnvp = calloc(1, sizeof(NVP_T));
        if(!pnvp)
            return;
        if ((pnvp->key = strdup(hashstr)) == NULL) {
            free(pnvp);
            return;
        }
        pnvp->val = val ? strdup(writeVal) : NULL;  /* use local copy of val in place of external pointer */ 
        if (val && !pnvp->val) {
            free(pnvp->key);
            free(pnvp);
            return;
        }
        hashTable[hashval] = pnvp;	/* Fortify [Buffer overflow]:: False Positive */
        return;
    }
    else{
        pnvp = hashTable[hashval];
        while(pnvp) {
            if(pnvp->key) {
                if(strcmp(pnvp->key, hashstr) == 0) {
                	// memory allocation is only necessary if the string length has changed
                    if(pnvp->val) {
                    	if ((val == NULL) || (strlen(pnvp->val) != strlen(writeVal))) { /* use local copy of val in place of external pointer */
							free(pnvp->val);
                                                        pnvp->val = val ? strdup(writeVal) : NULL; /* use local copy of val in place of external pointer */
                    	} else {
                    		// copy the new value to be sure that the correct value is stored
                                        strcpy(pnvp->val, writeVal);	/* Fortify [Buffer overflow]:: False Positive */ /* use local copy of val in place of external pointer */
                    	}
                    } else {
                        pnvp->val = val ? strdup(writeVal) : NULL; /* use local copy of val in place of external pointer */
                    }
                    return;
                }
            }

            if(pnvp->next == NULL)
                break;
            pnvp = pnvp->next;
        }
        if ( NULL != (pnvp->next = calloc(1, sizeof(NVP_T))) )  {
            if ((pnvp->next->key = strdup(hashstr)) == NULL) {
                free(pnvp->next);
                return;
            }
            pnvp->next->val = val ? strdup(writeVal) : NULL; /* use local copy of val in place of external pointer */
            if (val && !pnvp->next->val) {
                free(pnvp->next->key);
                free(pnvp->next);
                return;
            }
		}
    }
        return;

}


void iniparser_del_sec_key(INIHandler *handler, char *hashstr)
{
    unsigned long hashval;
    NVP_T *pnvp = NULL , *tmp_ptr=NULL, *prev_ptr = NULL;
    NVP_T **hashTable = NULL;

    if(!handler || !(handler->hashTable))
        return;

    hashTable = handler->hashTable;
    hashval = hasher31(hashstr);
    if(hashTable[hashval] == NULL) {
        return;
    }
    else{
        pnvp = hashTable[hashval];
        while(pnvp) {
            if(pnvp->key) {
                if(strcmp(pnvp->key, hashstr) == 0)
                {
                    tmp_ptr = pnvp->next;
                    if (pnvp == hashTable[hashval]) {
                        hashTable[hashval] = tmp_ptr;
                    }
                    if(pnvp->val){
                        free(pnvp->val);
                        pnvp->val = NULL;
                    }
                    free(pnvp->key);
                    pnvp->key = NULL;
                    free(pnvp);
                    pnvp = tmp_ptr;
                    if (prev_ptr) {
                        prev_ptr->next = tmp_ptr;
                    }
                    break;
                }
            }
            prev_ptr = pnvp;
            pnvp=pnvp->next;
        }
    }
}

void iniparser_del_section(INIHandler *handler, char * name)
{
	SEC_T *prev_ptr = NULL, *cur_ptr=NULL;
	int i = 0 , j =0;
	int nsec=0,seclen = 0;
	char *secname = NULL;
	char    keym[ASCIILINESZ+1];
	NVP_T **hashTable = NULL;
	NVP_T *nvp_ptr = NULL,*ptr=NULL;
		
	if (handler==NULL || handler->secTable==NULL)
		return;
	
	hashTable = handler->hashTable;
	
	nsec = iniparser_getnsec(handler);

	for (i=0 ; i < nsec ; i++) {
		   secname = iniparser_getsecname(handler, i) ;
		   if(NULL == secname) {
			   continue;
		   }
		   seclen  = SNPRINTF(keym,sizeof(keym) ,"%s:", name);
		   if(seclen <= 0 || seclen >= (signed)sizeof(keym))
		   {
			   printf("Buffer Overflow in File :%s Line: %d  Function : %s\n",__FILE__, __LINE__,FUNCTION_NAME);
			   continue;
		   }
		   
		   for (j=0 ; j < HASH_TABLE_SIZE ; j++) {
			   if (hashTable[j] == NULL)
				   continue;

	
			   nvp_ptr = hashTable[j];

		  	   if(nvp_ptr != NULL && 
			   	nvp_ptr->key != NULL && 
				strncmp(nvp_ptr->key, keym, seclen) == 0) {
				   while(nvp_ptr != NULL){
						ptr = nvp_ptr->next;
						free(nvp_ptr->key);
						free(nvp_ptr->val);
						free(nvp_ptr);
						nvp_ptr = ptr;
					}
					hashTable[j] = NULL;
				}
		  }	
	}   

	for (cur_ptr = handler->secTable;cur_ptr != NULL; 
		prev_ptr = cur_ptr, cur_ptr = cur_ptr->next) {

		    if (strcmp(name,cur_ptr->name) == 0) {  
			    if (prev_ptr== NULL) 
			    {        
			        handler->secTable = cur_ptr->next;
			    }else 
			    {
			        prev_ptr->next = cur_ptr->next;
      			    }
			    free(cur_ptr->name);
			    free(cur_ptr);
			    return;
    		    }
  	}
	return;		
}


/* Function Name: iniparser_delentry
 *
 * This function deletes the values specified by the char pointer, entry from
 * INIHandler. If that pointer is a section name, then it deletes the whole section
 * from the handler. If the value is a combination of section:property then that 
 * entry is removed.
 */
void iniparser_delentry(INIHandler *handler,char *key)
{
    char *tok = NULL;
	
    if(!handler || !key )
		return;	
	
    tok = strchr(key, ':');
    if (tok != NULL){//sec:name
    	iniparser_del_sec_key(handler, key);
	return;
    }
    //only section
    iniparser_del_section(handler, key);	
	
    return;
}



int iniparser_setstr(INIHandler *handler, char *hashstr, char * val)
{
    iniparser_setstring(handler, hashstr, val);
    return 0;
}

/* Function Name: iniparser_getnsec
 *
 * This function returns number of sections in the INIHandler.
 * Returns -1 in case of an error.
 */
int iniparser_getnsec(INIHandler *handler)
{
    if(!handler)
                return -1;
    return handler->sec_count;
}

/* Function Name: iniparser_getsecname
 *
 * This function locates the n-th section in a INIHandler and returns
 * its name as a pointer to a string in the INIHandler's secTable structure 
 * Note:
 *     Do not free or modify the returned string!
 */
char * iniparser_getsecname(INIHandler *handler, int n)
{
    int i = 0;
    SEC_T * sec_ptr = NULL;
    if( !handler || !handler->secTable)
        return NULL;
    sec_ptr = handler->secTable;
    for(i = 0; (i < n) && (sec_ptr); i++) {
        sec_ptr = sec_ptr->next;
    }
    if(!sec_ptr)
        return NULL;
    return sec_ptr->name;
}

/* Function Name: iniparser_getstring
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned.
 * Note:
 *   The returned char pointer is pointing to a string allocated in
 *   the dictionary, do not free or modify it.
 */
char * iniparser_getstring(INIHandler *handler, char *key, char *defVal)
{
    unsigned short hashval;
    NVP_T *pnvp = NULL;
    NVP_T **hashTable = NULL;

    if (handler == NULL || key == NULL)
        return defVal;
    hashTable = handler->hashTable;
    if(hashTable == NULL)
        return defVal;
    /* Make a key as section:keyword */
    hashval = hasher31(key);
    pnvp = hashTable[hashval];
	while(pnvp != NULL) {
		if((pnvp->key != NULL) && (strcmp(pnvp->key, key) == 0)) {
			return pnvp->val;
		}
		pnvp = pnvp->next;
	}
    return defVal;
}

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
char * iniparser_getstr(INIHandler *handler, char *key)
{
    return iniparser_getstring(handler, key, NULL);
}

/* Function Name: iniparser_getchar
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the char value is returned.
 */
char iniparser_getchar(INIHandler *handler, char *key, char defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return str[0];
}

/* Function Name: iniparser_getboolean
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the boolean value 
 * (TRUE or FALSE) is returned.
 */
int iniparser_getboolean(INIHandler *handler, char *key, int defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    if((spstrcasecmp(str, "TRUE") == 0) ||
                (spstrcasecmp(str, "YES") == 0) ||
                (spstrcasecmp(str, "1") == 0) ||
                str[0] == 'Y' || str[0] == 'y') {
        return TRUE;
    }
    else if((spstrcasecmp(str, "FALSE") == 0) ||
                (spstrcasecmp(str, "NO") == 0) ||
                (spstrcasecmp(str, "0") == 0) ||
                str[0] == 'N' || str[0] == 'n') {
        return FALSE;
    }
    return defVal;
}

/* Function Name: iniparser_getint
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the integer value found
 * is returned.
 */
int iniparser_getint(INIHandler *handler, char *key, int defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return (int)strtol(str, NULL, 0);
}

/* Function Name: iniparser_getuint
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the unsigned integer value
 * found is returned.
 */
unsigned int iniparser_getuint(INIHandler *handler, char *key, unsigned int defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return (unsigned int)strtoul(str, NULL, 0);
}

/* Function Name: iniparser_getlong
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the long integer value
 * found is returned.
 */
long iniparser_getlong(INIHandler *handler, char *key, long defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return strtol(str, NULL, 0);
}

/* Function Name: iniparser_getshort
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the short integer value
 * found is returned.
 */
short iniparser_getshort(INIHandler *handler, char *key, short defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return (short)strtol(str, NULL, 0);
}

/* Function Name: iniparser_getdouble
 *
 * This function queries the INIHandler for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'defVal' is returned, else the double value
 * found is returned.
 */
double iniparser_getdouble(INIHandler *handler, char *key, double defVal)
{
    char *str = NULL;
    str = iniparser_getstring(handler, key, NULL);
    if(str == NULL)
        return defVal;
    return strtod(str, NULL);
}

// Free functions
void Free_SEC(SEC_T *psec)
{
    SEC_T * sec_ptr = psec;
    SEC_T * free_ptr = NULL;
    while(sec_ptr) {
        if(sec_ptr->name) {
            free(sec_ptr->name);
            sec_ptr->name = NULL;
        }
        free_ptr = sec_ptr;
        sec_ptr = sec_ptr->next;
        free(free_ptr);
        free_ptr = NULL;
    }
}

void Free_NVP(NVP_T *pnvp)
{
    NVP_T *nvp_ptr = pnvp;
    NVP_T * free_ptr = NULL;
    while(nvp_ptr) {
        if(nvp_ptr->key) {
            free(nvp_ptr->key);
            nvp_ptr->key = NULL;
        }
        if(nvp_ptr->val) {
            free(nvp_ptr->val);
            nvp_ptr->val = NULL;
        }
        free_ptr = nvp_ptr;
        nvp_ptr = nvp_ptr->next;
        free(free_ptr);
        free_ptr = NULL;

    }
    return;
}

// debug function
void print_tab(INIHandler *handler)
{
    int i, n = 0;
    NVP_T *nvp_ptr = NULL;
    NVP_T **hashTable = NULL;
    printf("print_tab\n");
    if(handler == NULL)
        return;
    hashTable = handler->hashTable;
    

    for (i = 0; i <  HASH_TABLE_SIZE; i++) {
        if (hashTable[i] != NULL) {
            printf("%07d : %s %s\n", i, hashTable[i]->key, hashTable[i]->val);
            nvp_ptr = hashTable[i]->next;
            while(nvp_ptr != NULL) {
                printf("--->%07d : %s %s\n", i, nvp_ptr->key, nvp_ptr->val);
                nvp_ptr = nvp_ptr->next;
            }
        }
        else
                printf("%07d : NULL\n", i);
    }
    n = iniparser_getnsec(handler);
    for(i = 0; i < n; i++)
           printf("%d : %s\n", i, iniparser_getsecname(handler, i));
    return;
}

/* Function Name: iniparser_find_entry
 *
 * The function returns TRUE if the key value is found in the hash table 
 * or in the section table, else returns FALSE.
 */
int iniparser_find_entry(INIHandler *handler, char *key)
{
	if(key == NULL)
		return FALSE;
	if(strchr(key,':') == NULL)
		return iniparser_findsection(handler, key);
	if(iniparser_getstring(handler, key, INI_INVALID_KEY) != INI_INVALID_KEY)
		return TRUE;
	return FALSE;
}

/* Function Name: iniparser_findsection
 *
 * The function returns TRUE if the name is found in the section table 
 * else returns FALSE.
 */
int iniparser_findsection(INIHandler *handler, char *sec)
{
    SEC_T *sectab = NULL;
    if(!handler || !handler->secTable)
        return FALSE;
    sectab = handler->secTable;
    while(sectab) {
        if(0 == strcmp(sectab->name, sec))
            return TRUE;
        sectab = sectab->next;
    }
    return FALSE;
}

/* Function Name: iniparser_dump_ini
 *
 * This function dumps a given INIHandler into a loadable ini file.
 * It is Ok to specify stderr or stdout as output files
 */
void iniparser_dump_ini(INIHandler *handler, FILE *file)
{
    int     i, j ;
    char    keym[ASCIILINESZ+1];
    int     nsec ;
    char *  secname , *saveptr = NULL;
    int     seclen ;
    NVP_T **hashTable = NULL;
    NVP_T *nvp_ptr = NULL;
    if (handler == NULL || file == NULL || handler->hashTable == NULL) {
        return ;
    }
    hashTable = handler->hashTable;
    for (i=0 ; i < HASH_TABLE_SIZE ; i++) {
        if (hashTable[i] == NULL)
            continue;
        nvp_ptr = hashTable[i];
        while(nvp_ptr != NULL) {
            char *str = nvp_ptr->key ? strdup(nvp_ptr->key) : NULL;
            char *tok = NULL;
            saveptr = NULL;
            if(str) {
#ifdef _WIN32
            	tok = strtok_s(str, ":", &saveptr);
#else
            	tok = strtok_r(str, ":", &saveptr);
#endif
                if( tok != NULL ) {
                    iniparser_add_section(handler, tok);
                }
                free(str);
                str = NULL;
            }
            nvp_ptr = nvp_ptr->next;
        }
    }

    nsec = iniparser_getnsec(handler);
    if (nsec<1) {
        for (i=0 ; i < HASH_TABLE_SIZE ; i++) {
            if (hashTable[i] == NULL)
                continue;
            nvp_ptr = hashTable[i];
            while(nvp_ptr != NULL) {
                fprintf(file, "%s=%s\n", nvp_ptr->key, nvp_ptr->val);
                nvp_ptr = nvp_ptr->next;
            }
        }
        return ;
    }
    for (i=0 ; i < nsec ; i++) {
        secname = iniparser_getsecname(handler, i) ;
        if(NULL == secname) {
            continue;
        }
        seclen  = (int)strlen(secname);
        fprintf(file, "\n[%s]\n", secname);
        if(SNPRINTF(keym, sizeof(keym),"%s:", secname) >= (signed)sizeof(keym))
        {
            printf("Buffer Overflow in File:%s Line: %d  Function : %s for i=%d\n",__FILE__, __LINE__, FUNCTION_NAME,i);
            continue;
        }
        for (j=0 ; j < HASH_TABLE_SIZE ; j++) {
            if (hashTable[j] == NULL)
                continue;

            nvp_ptr = hashTable[j];
            while(nvp_ptr != NULL && nvp_ptr->key != NULL) {
                if (!strncmp(nvp_ptr->key, keym, seclen + 1)) {
                    fprintf(file, "%s=%s\n",
                        nvp_ptr->key+seclen+1,
                        nvp_ptr->val ? nvp_ptr->val : "");
                }
                nvp_ptr = nvp_ptr->next;
            }
        }
    }
    fprintf(file, "\n");
    return ;
}
void iniparser_dump(INIHandler *handler, FILE *file)
{
    int i = 0;
    NVP_T **hashtable = NULL;
    if(!handler || !file || !handler->hashTable)
        return;
    hashtable = handler->hashTable;
    for(i = 0; i < HASH_TABLE_SIZE; i++) {
        if(hashtable[i] == NULL)
            continue;
        if(hashtable[i]->val != NULL)
            fprintf(file, "[%s]=[%s]\n", hashtable[i]->key, hashtable[i]->val);
        else
            fprintf(file, "[%s]=UNDEF\n", hashtable[i]->key);
    }
    return;
}

/* Function Name: iniparser_dump_file
 *
 * This function dumps a given INIHandler into a loadable ini file.
 * It is Ok to specify stderr or stdout as output files
 */
int iniparser_dump_file(INIHandler *handle, char *filename)
{
    FILE *fp;
    char bkp_filename[MAX_CONF_STR_LENGTH];

    /* Save the INI in a backup file. So during failure (poweroff/reset) 
       during write will not corrupt original file */
    if(SNPRINTF(bkp_filename,sizeof(bkp_filename),"%s%s",filename,".tmp") >= (signed)sizeof(bkp_filename))
    {
        return -1;
    }
    fp = fopen(bkp_filename,"w");
    if(fp == NULL)
    {
        return -1;
    }
    iniparser_dump_ini(handle,fp);
    fclose(fp);

    /* Now rename backup file to original file - This is a atmoic
       operation of changing the inode link. So depending upon the 
       time of failure the file will be intact either with old or new
	   contents */
    if ( -1 == rename(bkp_filename,filename) )
        printf("[ERROR] IniSaveFile: Name changed %s to %s Failed\n", bkp_filename, filename);
    return 0;
}


/* Function Name: iniparser_getsecnkeys

  @brief    Get the number of keys in a section of a dictionary.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   Number of keys in section
*/
int iniparser_getsecnkeys(dictionary * d, char * s)
{
	int seclen, nkeys ;
	char keym[ASCIILINESZ+1];
	int j;

	nkeys = 0;

	if (d==NULL)
		return nkeys;
	if (! iniparser_find_entry(d, s))
	{
		fprintf (stderr, "\n%s:%d (%s) iniparser_find_entry()=NULL, nkeys=0\n", __FILE__, __LINE__, FUNCTION_NAME);
		return nkeys;
	}

	seclen  = (int)strlen(s);
	if(SNPRINTF(keym, sizeof(keym),"%s:", s) >= (signed)sizeof(keym))
	{
		printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
		return -1;
	}

	j=0;
	while (j < HASH_TABLE_SIZE)
	{
		NVP_T *tmp = (NVP_T *) d->hashTable[j];
		while (tmp)
		{
			if (tmp->key!=NULL)
			{
				if (!strncmp(tmp->key, keym, seclen+1))
					nkeys++;
			}
			tmp = tmp->next;
		}
		j++;
	}

	return nkeys;
}

/* Function Name: iniparser_getseckeys
 
  @brief    Get all keys in a given section.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   pointer to statically allocated character strings

  This function queries a dictionary and finds all keys in a given section.
  Each pointer in the returned char pointer-to-pointer is pointing to
  a string allocated in the dictionary; do not free or modify them,but free
  the memory pointed by pointer-to-pointer.

  This function returns NULL in case of error.
 **/
char** iniparser_getseckeys(dictionary * d, char * s)
{
	char **keys;
	int i, j ;
	char keym[ASCIILINESZ+1];
	int seclen, nkeys ;

	keys = NULL;

	if (d==NULL)
		return keys;
	if (! iniparser_find_entry(d, s))
	{
		fprintf (stderr, "\n%s:%d (%s) iniparser_find_entry()=NULL, keys=NULL\n", __FILE__, __LINE__, FUNCTION_NAME);
		return keys;
	}

	nkeys = iniparser_getsecnkeys(d, s);

	keys = (char**) malloc(nkeys*sizeof(char*));//This allocated memory should be free by the calling function;do free(keys) and do not free(keys[i]).

	seclen  = (int)strlen(s);
	if(SNPRINTF(keym,sizeof(keym),"%s:", s) >= (signed)sizeof(keym))
	{
		free(keys);
		printf("Buffer Overflow in File: %s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
		return NULL;
	}

	i = 0;
	j = 0;
	while (j < HASH_TABLE_SIZE)
	{
		NVP_T *tmp = (NVP_T *) d->hashTable[j];
		while (tmp)
		{
			if (tmp->key!=NULL)
			{
				if (!strncmp(tmp->key, keym, seclen+1))
				{
					keys[i] = tmp->key;
					i++;
				}
			}
			tmp = tmp->next;
		}
		j++;
	}

	return keys;
}

#ifdef TEST_PARSER
int main (int argc, char *argv[])
{
    FILE *f;

    if (argc != 2) {
        printf ("Usage:%s <test ini file name> \n", argv[0]);
        return 0;
    }

    INIHandler *iniHandler;
    printf ("Loading the ini\n");
    iniHandler = iniparser_load(argv[1]);

    if (iniHandler == NULL) {
        printf ("Unable to load ini : %s\n", argv[1]);
        return 0;
    }
print_tab(iniHandler);
    f = fopen("out.ini","w");
    iniparser_dump_ini(iniHandler, f);
    fclose(f);
    iniparser_close(iniHandler);
    return 0;

}
#endif

