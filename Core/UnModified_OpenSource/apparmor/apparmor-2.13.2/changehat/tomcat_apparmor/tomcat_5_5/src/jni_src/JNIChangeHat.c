/*
  ------------------------------------------------------------------

    Copyright (C) 2002-2005 Novell/SUSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License published by the Free Software Foundation.

  ------------------------------------------------------------------
 */


#include "jni.h"
#include <errno.h>
#include <sys/apparmor.h>
#include "com_novell_apparmor_JNIChangeHat.h"

/* c intermediate lib call for Java -> JNI -> c library execution of the change_hat call */

JNIEXPORT jint Java_com_novell_apparmor_JNIChangeHat_changehat_1in
  (JNIEnv *env, jobject obj, jstring hatnameUTF, jint token)
{

  int len;
  jint result = 0;
  
  if ( hatnameUTF == NULL ) {
  	  	return ( EINVAL );
  } 
  len = (*env)->GetStringLength(env, hatnameUTF);
  if ( len > 0 ) {
     if ( len > 128 ) {
       len = 128;
     }
     char hatname[128];
     (*env)->GetStringUTFRegion(env, hatnameUTF, 0, len, hatname);
     result = (jint) change_hat(hatname, (unsigned int) token);
     if ( result ) {
       return errno;
     } else {
       return result;
     }
  } 
  
  return (jint) result;

}

JNIEXPORT jint JNICALL Java_com_novell_apparmor_JNIChangeHat_changehat_1out
  (JNIEnv *env, jobject obj, jint token)
{
  
  jint result = 0;
  result =  (jint) change_hat(NULL, (unsigned int) token);
  if ( result ) {
    return errno;
  } else {
    return result;
  }

}


