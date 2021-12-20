# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2004, 2005, 2006 NOVELL (All rights reserved)
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
# ----------------------------------------------------------------------
NAME 		= tomcat_apparmor
all:
COMMONDIR=../../../common/

include $(COMMONDIR)/Make.rules

LIB		= lib
CATALINA_HOME 	= /usr/share/tomcat5

# By default build 1.4 bytecode
all:
	ant -Dtarget=1.4 jar jni_so

clean:
	ant clean

install:
	 ant  -Dversion=$(VERSION) -Drelease=$(MAN_RELEASE) -Dcatalina_home=${CATALINA_HOME} -Dinstall_lib=${LIB} install_jar install_jni
