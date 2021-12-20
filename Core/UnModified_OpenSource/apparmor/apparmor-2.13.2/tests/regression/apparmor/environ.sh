#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME environ
#=DESCRIPTION
# verify bprm_unsafe filtering occurs for Px and Ux.
#
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

helper=$pwd/env_check
setuid_helper=${tmpdir}/env_check
helper_sh=$pwd/env_check.sh
read_locale="/usr/lib/locale/**:r"

# TEST environment filtering on elf binaries
genprofile $helper:ux
runchecktest "ENVIRON (elf): ux & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): ux & sensitive env" pass $helper LD_LIBRARY_PATH=.

genprofile $helper:Ux
runchecktest "ENVIRON (elf): Ux & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): Ux & sensitive env" fail $helper LD_LIBRARY_PATH=.

genprofile $helper:rix
runchecktest "ENVIRON (elf): ix & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): ix & sensitive env" pass $helper LD_LIBRARY_PATH=.

genprofile $helper:px -- image=$helper
runchecktest "ENVIRON (elf): px & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): px & sensitive env" pass $helper LD_LIBRARY_PATH=.

genprofile $helper:Px -- image=$helper
runchecktest "ENVIRON (elf): Px & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): Px & sensitive env" fail $helper LD_LIBRARY_PATH=.

genprofile image=$helper
runchecktest "ENVIRON (elf): unconfined --> confined & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): unconfined --> confined & sensitive env" pass $helper LD_LIBRARY_PATH=.

genprofile -C
runchecktest "ENVIRON (elf): confined/complain & regular env" pass $helper FOO=BAR
runchecktest "ENVIRON (elf): confined/complain & sensitive env" pass $helper LD_LIBRARY_PATH=.

# TEST environment filtering on shell scripts
genprofile ${helper_sh}:ux
runchecktest "ENVIRON (shell script): ux & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): ux & sensitive env" pass ${helper_sh} LD_LIBRARY_PATH=.

genprofile ${helper_sh}:Ux
runchecktest "ENVIRON (shell script): Ux & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): Ux & sensitive env" fail ${helper_sh} LD_LIBRARY_PATH=.

genprofile ${helper_sh}:px -- image=${helper_sh} "$read_locale"
runchecktest "ENVIRON (shell script): px & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): px & sensitive env" pass ${helper_sh} LD_LIBRARY_PATH=.

genprofile ${helper_sh}:Px -- image=${helper_sh} "$read_locale"
runchecktest "ENVIRON (shell script): Px & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): Px & sensitive env" fail ${helper_sh} LD_LIBRARY_PATH=.

genprofile addimage:${helper_sh} "$read_locale"
runchecktest "ENVIRON (shell script): ix & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): ix & sensitive env" pass ${helper_sh} LD_LIBRARY_PATH=.

genprofile image=${helper_sh} "$read_locale"
runchecktest "ENVIRON (shell script): unconfined --> confined & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): unconfined --> confined & sensitive env" pass ${helper_sh} LD_LIBRARY_PATH=.

genprofile -C
runchecktest "ENVIRON (shell script): confined/complain & regular env" pass ${helper_sh} FOO=BAR
runchecktest "ENVIRON (shell script): confined/complain & sensitive env" pass ${helper_sh} LD_LIBRARY_PATH=.

# TEST environment filtering still works on setuid apps
removeprofile

cp $helper ${setuid_helper}
chown nobody ${setuid_helper}
chmod u+s ${setuid_helper}
runchecktest "ENVIRON (elf): unconfined setuid helper" pass ${setuid_helper} FOO=BAR
runchecktest "ENVIRON (elf): unconfined setuid helper" fail ${setuid_helper} LD_LIBRARY_PATH=.
