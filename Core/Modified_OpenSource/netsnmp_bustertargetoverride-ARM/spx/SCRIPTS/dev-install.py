#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import os
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#                             Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
        TARGET=PrjVars["TARGETDIR"]
        MULTIARCH=PrjVars["MULTI_ARCH_LIB"]
        PkgDir="%s/%s"%(PrjVars["BUILD"],PrjVars["PACKAGE"])

        retval = Py_CopyFile("./snmpd",TARGET+"/usr/sbin/snmpd")
        if retval != 0:
                print("Error in copy snmpd")
                return retval
        
        retval = Py_CopyFile("./snmptrap",TARGET+"/usr/bin/snmptrap")
        if retval != 0:
                print("Error in copy snmptrap")
                return retval

        for file in os.listdir("./net-snmp_libraries"):
                retval = Py_CopyFile("./net-snmp_libraries/"+file,TARGET+"/usr/lib/arm-linux-gnueabi/"+file)
                if retval != 0:
                        print("Error in copying libraries")
                        return retval

        retval = Py_MkdirClean(TARGET+"/usr/include/net-snmp/")
        if retval != 0:
                return retval

        retval = Py_CopyDir("./net-snmp_headers",TARGET+"/usr/include/net-snmp/")
        if retval != 0:
                return retval

        Py_Cwd(TARGET+"/usr/lib/arm-linux-gnueabi/")
        Py_RemoveSoftLink("libnetsnmpagent.so")
        Py_RemoveSoftLink("libnetsnmpagent.so.30")
        Py_RemoveSoftLink("libnetsnmphelpers.so")
        Py_RemoveSoftLink("libnetsnmphelpers.so.30")
        Py_RemoveSoftLink("libnetsnmpmibs.so")
        Py_RemoveSoftLink("libnetsnmpmibs.so.30")
        Py_RemoveSoftLink("libnetsnmp.so")
        Py_RemoveSoftLink("libnetsnmp.so.30")
        Py_RemoveSoftLink("libnetsnmptrapd.so")
        Py_RemoveSoftLink("libnetsnmptrapd.so.30")
        Py_Delete("libnetsnmpagent.so.30.0.3")
        Py_Delete("libnetsnmphelpers.so.30.0.3")
        Py_Delete("libnetsnmpmibs.so.30.0.3")
        Py_Delete("libnetsnmp.so.30.0.3")
        Py_Delete("libnetsnmptrapd.so.30.0.3")
        Py_SoftLink("libnetsnmpagent.so.35.0.0","libnetsnmpagent.so")
        Py_SoftLink("libnetsnmpagent.so.35.0.0","libnetsnmpagent.so.35")
        Py_SoftLink("libnetsnmphelpers.so.35.0.0","libnetsnmphelpers.so")
        Py_SoftLink("libnetsnmphelpers.so.35.0.0","libnetsnmphelpers.so.35")
        Py_SoftLink("libnetsnmpmibs.so.35.0.0","libnetsnmpmibs.so")
        Py_SoftLink("libnetsnmpmibs.so.35.0.0","libnetsnmpmibs.so.35")
        Py_SoftLink("libnetsnmp.so.35.0.0","libnetsnmp.so")
        Py_SoftLink("libnetsnmp.so.35.0.0","libnetsnmp.so.35")
        Py_SoftLink("libnetsnmptrapd.so.35.0.0","libnetsnmptrapd.so")
        Py_SoftLink("libnetsnmptrapd.so.35.0.0","libnetsnmptrapd.so.35")


        return 0


#-------------------------------------------------------------------------------------------------------
#                               Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
        return 0

#-------------------------------------------------------------------------------------------------------
                                                                                                            
