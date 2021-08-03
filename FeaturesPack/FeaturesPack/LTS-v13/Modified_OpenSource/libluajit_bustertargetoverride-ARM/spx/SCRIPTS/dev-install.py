#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
import os
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
	Py_MkdirClean(TARGET+"/usr/share/luajit-2.0.5")
	retval = Py_MkdirClean(TARGET+"/usr/local/share/luajit-2.0.5")
	if retval != 0: 
		return retval
	Py_CopyDir("./jit",TARGET+"/usr/share/luajit-2.0.5/jit")
	Py_CopyDir("./jit",TARGET+"/usr/local/share/luajit-2.0.5/jit")
	Py_CopyFile("./luajit-2.0.5",TARGET+"/usr/local/bin/luajit-2.0.5")
	retval = Py_CopyFile("./luajit-2.0.5",TARGET+"/usr/bin/luajit-2.0.5")
	if retval != 0: 
		return retval
	Py_Cwd(TARGET+"/usr/bin/")
	retval = Py_SoftLink("luajit-2.0.5","luajit")
	if retval != 0:
		return retval
	Py_Cwd(TARGET+"/usr/local/bin/")
	retval = Py_SoftLink("luajit-2.0.5","luajit")
	if retval != 0:
		return retval
	
	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
