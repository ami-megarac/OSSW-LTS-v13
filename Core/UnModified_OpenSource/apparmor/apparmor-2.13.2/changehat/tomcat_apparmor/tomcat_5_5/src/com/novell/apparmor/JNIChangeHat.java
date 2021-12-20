/* ------------------------------------------------------------------
 * 
 *    Copyright (C) 2002-2005 Novell/SUSE
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *    License published by the Free Software Foundation.
 * 
 * ------------------------------------------------------------------ */
package com.novell.apparmor;

import java.nio.*;

/**
  *
  * JNI interface to AppArmor change_hat(2) call
  *
  **/
public class JNIChangeHat
{
  public static int EPERM  = 1;
  public static int ENOMEM = 12;
  public static int EACCES = 13;
  public static int EFAULT = 14;

	
  //  Native 'c' function delcaration
  public native int changehat_in(String subdomain, int magic_token);
  
  //  Native 'c' function delcaration
  public native int changehat_out(int magic_token);
  
  static
  {
    // The runtime system executes a class's static initializer 
    // when it loads the class.
     System.loadLibrary("JNIChangeHat");
  }
  
}
