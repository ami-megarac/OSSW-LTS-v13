/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */
/*****************************************************************
*
* Filename: parse-ex.c
*
* Description: Extended version of Ini Parser. Provides simpler and
*              more fuctions to handle INI files
*
******************************************************************/
#include <stdio.h>
#include "iniparser.h"
#include "dictionary.h"
#include "parse-ex.h"
#include "strlib.h"

INI_HANDLE 
IniLoadFile(char *filename)
{
    dictionary *d = NULL;
    FILE *fp;


	/* Check if file Exists and if does not exist create a new one */
    fp = fopen(filename,"rb");
    if (fp != NULL)
    {
        fclose(fp);
    }
    else
    {
        printf("Creating new %s ...\n",filename);
        fp = fopen(filename,"wb");
        if (fp == NULL)
        {
            printf("[ERROR] LoadIniFile: %s creation Failed\n",filename);
            return NULL;
        }
        fclose(fp);
    }

	/* Open the Ini File */
    d = iniparser_load(filename);
    if (d == NULL)
    {
        printf("[ERROR] LoadIniFile: %s open  Failed\n",filename);
        return NULL;
    }
    
    return (INI_HANDLE)d;
}

void
IniCloseFile(INI_HANDLE handle)
{
	iniparser_close((dictionary*)(handle));
	return;
}

void
IniAddSection(INI_HANDLE handle, char *Section)
{
    iniparser_setstring((dictionary*)handle, Section, NULL);
   	return;
}

int
IniGetNumOfSection(INI_HANDLE handle)
{
    return iniparser_getnsec((dictionary*)handle);
}

char *
IniGetSectionName(INI_HANDLE handle, int Index)
{
    return  iniparser_getsecname ((dictionary *)handle,Index);
}

void
IniDelSection(INI_HANDLE handle, char *Section)
{
    iniparser_delentry((dictionary *)handle,Section);
    return;
}


void 
IniAddEntry(INI_HANDLE handle,char *Section,  char * Key, char *Val)
{

	char FullKey[MAX_STRSIZE];	

    	if (handle ==NULL || Key==NULL || Section == NULL)
        	return;

    	if( SNPRINTF(FullKey, sizeof(FullKey), "%s:%s",Section,Key) >= (signed)sizeof(FullKey))
    	{
    		printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
    		return;
    	}

	/* Will add the Section if not exists */
    	dictionary_set((dictionary *)handle,FullKey,Val);
    	return;
	
}

void
IniDelEntry(INI_HANDLE handle, char *Section, char *Key)
{

    char FullKey[MAX_STRSIZE];	

    if (handle ==NULL || Key==NULL || Section == NULL)
        return;

    if(SNPRINTF(FullKey,sizeof(FullKey),"%s:%s",Section,Key) >= (signed)sizeof(FullKey))
    {
        printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
        return;
    }

    dictionary_unset((dictionary *)handle,FullKey);
        return;
}

char *
IniGetEntry(INI_HANDLE handle, char *Section, char *Key)
{
    char FullKey[MAX_STRSIZE];	

    if (handle ==NULL || Key==NULL || Section == NULL)
        return NULL;

    if( SNPRINTF(FullKey,sizeof(FullKey), "%s:%s",Section,Key) >= (signed)sizeof(FullKey))
    {
        printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
        return NULL;
    }

    return dictionary_get((dictionary *)handle,FullKey,NULL);
}

void
IniSetStr(INI_HANDLE handle, char *Section, char *Key, char *Val)
{
	IniAddEntry(handle,Section,Key,Val);
	return;
}

void
IniSetUInt(INI_HANDLE handle, char *Section, char *Key, unsigned long Val)
{
    char Str[MAX_STRSIZE];
    if(SNPRINTF(Str,sizeof(Str),"%lu",Val) >= 	(signed)sizeof(Str))
    {
        printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
        return;
    }
    IniAddEntry(handle,Section,Key,Str);
    return;
}

void
IniSetSInt(INI_HANDLE handle, char *Section, char *Key, long Val)
{
    char Str[MAX_STRSIZE];
	if(SNPRINTF(Str,sizeof(Str), "%ld",Val) >= (signed)sizeof(Str))
	{
		printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
		return;
	}
	IniAddEntry(handle,Section,Key,Str);
	return;
}

void
IniSetDouble(INI_HANDLE handle, char *Section, char *Key, double Val)
{
    char Str[MAX_STRSIZE];
    sprintf(Str,"%f",Val);	
    IniAddEntry(handle,Section,Key,Str);
    return;
}

void
IniSetBool(INI_HANDLE handle, char *Section, char *Key, int Bool)
{
	if (Bool)
		IniAddEntry(handle,Section,Key,"TRUE");
	else
		IniAddEntry(handle,Section,Key,"FALSE");
	return;
}

void
IniSetChar(INI_HANDLE handle, char *Section, char *Key, char Val)
{
	char Str[2];
	Str[0]=Val;
	Str[1]=0;
	IniAddEntry(handle,Section,Key,Str);
	return;
}

char*
IniGetStr(INI_HANDLE handle, char *Section, char *Key, char *Def)
{
	char *s = IniGetEntry(handle,Section,Key);
	if (s == NULL)
		return Def;
	return s;
}

long
IniGetSInt(INI_HANDLE handle, char *Section, char *Key, long  Def)
{
	char *s = IniGetEntry(handle,Section,Key);
	if (s == NULL)
		return Def;
	return strtol(s,NULL,0);
}

unsigned long
IniGetUInt(INI_HANDLE handle, char *Section, char *Key, unsigned long  Def)
{
	char *s = IniGetEntry(handle,Section,Key);
	if (s == NULL)
		return Def;
    return strtoul(s,NULL,0);
}

double
IniGetDouble(INI_HANDLE handle, char *Section, char *Key, double Def)
{
    char *s = IniGetEntry(handle,Section,Key);
    if (s == NULL)
        return Def;
    return strtod(s,NULL);
}

int
IniGetBool(INI_HANDLE handle, char *Section, char *Key, int Def)
{
    char *s = IniGetEntry(handle,Section,Key);
	if (s == NULL)
		return Def;
	if (strlen(s) == 1)
	{
   	 	if (s[0]=='y' || s[0]=='Y' || s[0]=='1' || s[0]=='t' || s[0]=='T') 
			return 1;
   	 	if (s[0]=='n' || s[0]=='N' || s[0]=='0' || s[0]=='f' || s[0]=='F') 
			return 0;
	}
    if ( !strcasecmp(s,"YES") || !strcasecmp(s,"TRUE"))
        return 1;
    if ( !strcasecmp(s,"NO") || !strcasecmp(s,"FALSE"))
        return 0;

    return Def;
}

char
IniGetChar(INI_HANDLE handle, char *Section, char *Key, char Def)
{
	char *s = IniGetEntry(handle,Section,Key);
	if (s == NULL)
		return Def;
	return s[0];
}

int
IniSaveFile(INI_HANDLE handle,char *filename)
{
    	FILE *fp;
	char bkp_filename[MAX_STRSIZE];

	/* Save the INI in a backup file. So during failure (poweroff/reset) 
	   during write will not corrupt original file */
    	if(SNPRINTF(bkp_filename,sizeof(bkp_filename),"%s%s",filename,".tmp") >= (signed)sizeof(bkp_filename))
    	{
    		printf("Buffer Overflow in File:%s Line: %d  Function : %s\n",__FILE__, __LINE__, FUNCTION_NAME);
    		return -1;
    	}
    	fp = fopen(bkp_filename,"w");
    	if(fp == NULL)
    	{
		printf("[ERROR] SaveIniFile: %s open  Failed\n",filename);
        	return -1;
    	}
    	iniparser_dump_ini((dictionary *)handle,fp);
    	fclose(fp);

	/* Now rename backup file to original file - This is a atmoic
           operation of changing the inode link. So depending upon the 
	   time of failure the file will be intact either with old or new
	   contents */
	if ( -1 == rename(bkp_filename,filename) )
	    printf("[ERROR] IniSaveFile: Name changed %s to %s Failed\n", bkp_filename, filename);

    	return 0;
}

void
IniDump(INI_HANDLE handle)
{
   	printf("\n#-----------------------------------------------------------------------\n");
    	iniparser_dump_ini((dictionary *)handle,stdout);
   	printf("\n#-----------------------------------------------------------------------\n");
}

/**************************************************************************************************/

		
