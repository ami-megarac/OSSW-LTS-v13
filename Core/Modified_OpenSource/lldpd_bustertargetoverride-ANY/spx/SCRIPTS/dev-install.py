#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
	TARGET=PrjVars["TARGETDIR"]
	retval = Py_CopyFile("./lldpd",TARGET+"/usr/sbin/lldpd")
	if retval != 0:
		return retval
	retval = Py_CopyFile("./lldpctl",TARGET+"/usr/sbin/lldpctl")
	if retval != 0:
		return retval
	retval = Py_StripUnneeded(TARGET+"/usr/sbin/lldpd")
	if retval != 0:
		return retval
	retval = Py_StripUnneeded(TARGET+"/usr/sbin/lldpctl")
	if retval != 0:
		return retval
	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
