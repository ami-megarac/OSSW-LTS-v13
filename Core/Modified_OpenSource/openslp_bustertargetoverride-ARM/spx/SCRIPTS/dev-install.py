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
	MULTIARCH=PrjVars["MULTI_ARCH_LIB"]
	IMAGETREE=PrjVars["IMAGE_TREE"]
	
	PkgDir="%s/%s"%(PrjVars["BUILD"],PrjVars["PACKAGE"])
	retval = Py_CopyFile("./slpd",TARGET+"/usr/sbin/slpd")
	if retval != 0:
		print("Error in coping slpd to ImageTree")
		return retval
	retval = Py_CopyFile("./slp.spi",TARGET+"/etc/slp.spi")
	if retval != 0:
		print("Error in coping slp.spi to ImageTree")
		return retval
	retval = Py_CopyFile("./slp.reg",TARGET+"/etc/slp.reg")
	if retval != 0:
		print("Error in coping slp.reg to ImageTree")
		return retval
	retval = Py_CopyFile("./slp.conf",TARGET+"/etc/slp.conf")
	if retval != 0:
		print("Error in coping slp.conf to ImageTree")
		return retval
	retval = Py_CopyFile("./slpd-init",TARGET+"/etc/init.d/slpd")
	if retval != 0:
		print("Error in coping slp.conf to ImageTree")
		return retval
	retval = Py_CopyFile("./libslp.so.1.0.1",TARGET+"/usr/lib/"+MULTIARCH+"/libslp.so.1.0.1")
	if retval != 0:
		print("Error in copying libslp")
		return retval	
	retval = Py_CopyFile("./slp.h",TARGET+"/usr/include/slp.h")
	if retval != 0:
		return retval
	retval = Py_Cwd(TARGET+"/usr/lib/"+MULTIARCH)
	if retval != 0:
		print("error in cwd")
		return retval
	retval = Py_SoftLink("./libslp.so.1.0.1","libslp.so")
	if retval != 0:
		print("Error in creating softlink")
		return retval


	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
