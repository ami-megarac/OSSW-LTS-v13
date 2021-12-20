# vim:syntax=sh
#
# Test infrastructure support.
#
# Copyright 2010 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.
#
# This file should be included by each test case
# It does a lot of hidden 'magic',  Downside is that
# this magic makes debugging fauling tests more difficult.
# Running the test with the '-r' option can help.
#
# Userchangeable variables (tmpdir etc) should be specified in
# uservars.inc
# 
# Cleanup is automatically performed by epilogue.inc
#
# For this file, functions are first, entry point code is at end, see "MAIN"

#use $() to retreive the failure message or "true" if success

# kernel_features_istrue() - test whether boolean files are true
# $@: path(s) to test if true
# Returns: 0 and "true" if all specified paths exist and are true
#          1 and error message if features directory is not available
#          2 and error message if feature file does not exist
#          3 and error message if feature path is not a file
kernel_features_istrue()
{
	if [ ! -e "/sys/kernel/security/apparmor/features/" ] ; then
		echo "Kernel feature masks not supported."
		return 1;
	fi

	for f in $@ ; do
		if [ ! -e "/sys/kernel/security/apparmor/features/$f" ] ; then
			echo "Required feature '$f' not available."
			return 2;
		fi
		if [ ! -f "/sys/kernel/security/apparmor/features/$f" -o \
		     `cat "/sys/kernel/security/apparmor/features/$f"` == 'no' ] ; then
			echo "Required feature '$f' not available."
			return 3;
		fi
        done

	echo "true"
	return 0;
}

# kernel_features - test whether path(s) are present
# $@: feature path(s) to test
# Returns: 0 and outputs "true" if all paths exist
#          1 and error message if features dir is not available
#          2 and error message if path does not exist
kernel_features()
{
	if [ ! -e "/sys/kernel/security/apparmor/features/" ] ; then
		echo "Kernel feature masks not supported."
		return 1;
	fi

	for f in $@ ; do
		if [ ! -e "/sys/kernel/security/apparmor/features/$f" ] ; then
			echo "Required feature '$f' not available."
			return 2;
		fi
	done

	echo "true"
	return 0;
}

# requires_kernel_features() - exit if kernel feature does not exist
# $@: feature path(s) to test
requires_kernel_features()
{
	local res=$(kernel_features $@)
	if [ "$res" != "true" ] ; then
		echo "$res. Skipping tests ..."
		exit 0
	fi
}

# requires_namespace_interface() - exit if namespace interface is not available
requires_namespace_interface()
{
	if [ ! -e "/sys/kernel/security/apparmor/policy/namespaces" ]
	then
		echo "Namespaces in apparmorfs policy interface not supported. Skipping tests ..."
		exit 0
	fi
}

# requires_query_interface() - exit if the query interface is not available
requires_query_interface()
{
	if [ ! -e "/sys/kernel/security/apparmor/.access" ]
	then
		echo "Kernel query interface not supported. Skipping tests ..."
		exit 0
	fi
}

# parser_supports() - test if the parser supports the following rules
# $@: rules to test, use quotes if the rule contains whitespace
# Returns: 0 and output "true" if all rules supported
#          1 and error message if compiler does not support rule
parser_supports()
{
	for R in "$@" ; do
		echo "/test { $R }" | $subdomain ${parser_args} -qQT 2>/dev/null 1>/dev/null
		if [ $? -ne 0 ] ; then
			echo "Compiler does not support rule '$R'"
			return 1;
		fi
	done

	echo "true"
	return 0;
}

#requires_parser_support() - exit if the parser does not support the rules
# $@: rules to test
requires_parser_support()
{
	local res=$(parser_supports $@)
	if [ "$res" != "true" ] ; then
		echo "$res. Skipping tests ..."
		exit 0
	fi
}


fatalerror()
{
	# global _fatal
	if [ -z "$_fatal" ]
	then
		_fatal=true	# avoid cascading fatal errors
		echo "Fatal Error ($testname): $*" >&2
		exit 127
	fi
}

testerror()
{
	fatalerror "Unable to run test sub-executable"
}

testfailed()
{
	# global num_testfailures teststatus
	num_testfailures=$(($num_testfailures + 1))
	teststatus="fail"

	# if we are retaining the tmpdir we are debugging failures so
	# stop so it can be looked at
	if [ $retaintmpdir == "true" ] ; then
	   exit 127
	fi
}

error_handler()
{
	fatalerror "Unexpected shell error. Run with -x to debug"
}

# invoked whenever we exit (normally, interrupt 
# or exit due to testfailure()/error_handler()
exit_handler()
{
	# global bin
	if [ -d "$bin" ]
	then
		. $bin/epilogue.inc
	fi
}

genrunscript()
{
	# create a log so we can run test again if -retain specified

	local runfile
	#global tmpdir profile outfile
	
	if [ "$retaintmpdir" = "true" ]
	then
		runfile=$tmpdir/runtest
		echo "$subdomain ${parser_args} $profile" > $runfile
		echo -n "$testexec " >> $runfile
		while [ $# -gt 0 ] ; do
		      echo -n "\"$1\" " >> $runfile
		      shift
		done
		echo "2>&1 > $outfile" >> $runfile
		echo "echo $testname: \`cat $outfile\`" >> $runfile
		echo "$subdomain ${parser_args} -R $profile" >> $runfile
	fi
}

runtestbg()
{
	if [ -z "${__NO_TRAP_ERR}" ]
	then
		trap "error_handler" ERR
	fi

	# global _testdesc _pfmode _known _pid outfile
	
	_testdesc=$1
	if [ ${2:0:1} == "x" ] ; then
		_pfmode=${2#x}
		_known=" (known problem)"
        else
		_pfmode=$2
		_known=""
        fi

	shift 2
	
	genrunscript "$@"
	
	$testexec "$@" > $outfile 2>&1 &
	
	_pid=$!
}

checktestbg()
{
	# global _pid _rc outfile 
	local rc

	wait $_pid
	rc=$?
	if [ $rc -gt 128 ]
	then
		echo "SIGNAL$(($rc - 128))" > $outfile
	fi
	checktestfg "$@"
}

runtestfg()
{
	# global _testdesc _pfmode _known outfile
	_testdesc=$1
	if [ ${2:0:1} == "x" ] ; then
		_pfmode=${2#x}
		_known=" (known problem)"
        else
		_pfmode=$2
		_known=""
        fi
	shift 2
	
	genrunscript "$@"
	
	$testexec "$@" > $outfile 2>&1 &
	_pid=$!
	wait $_pid
	test_rc=$?
	if [ $test_rc -gt 128 ]
	then
		echo "SIGNAL$(($test_rc - 128))" > $outfile
	fi
}

checktestfg()
{
	# global _pfmode _known _testdesc outfile teststatus testname
	local ret expectedsig killedsig

	ret=`cat $outfile 2>/dev/null`
	teststatus=pass
	
	case "$ret" in
		PASS)	if [ "$_pfmode" != "pass" -a -z "${_known}" ]
			then
				echo "Error: ${testname} passed. Test '${_testdesc}' was expected to '${_pfmode}'"
				testfailed
				return
			elif [ "$_pfmode" == "pass" -a -n "${_known}" ]
			then
				echo "Alert: ${testname} passed. Test '${_testdesc}' was marked as expected pass but known problem (xpass)"
			fi
			;;
		FAIL*)  if [ "$_pfmode" != "fail" -a -z "${_known}" ]
			then
				echo "Error: ${testname} failed. Test '${_testdesc}' was expected to '${_pfmode}'. Reason for failure '${ret}'"
				testfailed
				return
			elif [ "$_pfmode" == "fail" -a -n "${_known}" ]
			then
				echo "Alert: ${testname} failed. Test '${_testdesc}' was marked as expected fail but known problem (xfail)."
			fi
			;;
		SIGNAL*) killedsig=`echo $ret | sed 's/SIGNAL//'`
			case "$_pfmode" in
			signal*) expectedsig=`echo ${_pfmode} | sed 's/signal//'`
				if [ -n "${expectedsig}" -a ${expectedsig} != ${killedsig} ]
				then
					echo "Error: ${testname} failed. Test '${_testdesc}' was expected to terminate with signal ${expectedsig}${_known}. Instead it terminated with signal ${killedsig}"
					testfailed
					return
				fi
				;;
			*) 	echo "Error: ${testname} failed. Test '${_testdesc}' was expected to '${_pfmode}'${_known}. Reason for failure 'killed by signal ${killedsig}'"
				testfailed
				return
				;;	
			esac
			;;
		*)	testerror
			return
			;;
	esac

	if [ $# -gt 0 ]
	then
		$1
		if [ "$teststatus" != "pass" ]
		then
			return
		fi
	fi

	# warn about expected failures
	if [ -n "${_known}" ] ; then
		echo "x${_pfmode}: ${_testdesc}"
	elif [ -n "$VERBOSE" ]; then
		echo "ok: ${_testdesc}"
	fi
}

runchecktest()
{
	if [ -z "${__NO_TRAP_ERR}" ]
	then
		trap "error_handler" ERR
	fi

	runtestfg "$@"
	checktestfg
}

runchecktest_errno()
{
	local errno=$(perl -MPOSIX -e 'printf "%d\n", '$1';')
	shift 1

	if [ -z "${__NO_TRAP_ERR}" ]
	then
		trap "error_handler" ERR
	fi

	runtestfg "$@"
	if [ "$test_rc" == "$errno" ] ; then
		checktestfg
	else
		echo "Error: ${testname} failed. Test '${_testdesc}' was expected to '${_pfmode}'${_known}. Reason for failure expect errno ${errno} != ${test_rc}"
		testfailed
	fi
}

emit_profile()
{
	if [ -z "${__NO_TRAP_ERR}" ]	
	then
		trap "error_handler" ERR
	fi

	#global name outfile profile profilenames

	name=$1; shift 1

	$bin/mkprofile.pl ${mkflags} "$name" ${outfile}:w "$@" >> $profile

	echo $name >> $profilenames
}
		
genprofile()
{
if [ -z "${__NO_TRAP_ERR}" ]
then
	trap "error_handler" ERR
fi
	
	local num_emitted imagename hat args arg names1 names2
	#global complainflag escapeflag nodefaults profile profilenames

	complainflag=""
	mkflags=""
	while /bin/true
	do
		case "$1" in
			"-C") complainflag="-C"
			      ;;
			"-E") mkflags="${mkflags} -E"
			      ;;
			"-N") mkflags="${mkflags} -N"
			      ;;
			"-I") mkflags="${mkflags} -I"
			      ;;
			*) break
			   ;;
		esac
		shift
	done

	# save previous profile
	if [ -f $profile ]
	then
		mv $profile ${profile}.old
		mv $profilenames ${profilenames}.old
	fi

	num_emitted=0

	while /bin/true
	do
		imagename=$test

		# image/subhat allows overriding of the default
		# imagename which is based on the testname
		#
		# it is most often used after --, in fact it is basically
		# mandatory after --
		case "$1" in
			image=*) imagename=`echo $1 | sed 's/^image=\([^:]*\).*$/\1/'`
				 num_emitted=0
				 shift
				 ;;
			subhat=*) fatalerror "'subhat=hatname' is no longer supported ('$1')"
				 shift
				 ;;
		esac

		num_args=0
		while [ $# -gt 0 ]
		do
			arg="$1"
			shift

			# -- is the separator between profiles
			if [ "$arg" == "--" ]
			then
				eval emit_profile \"$imagename\" \
					$(for i in $(seq 0 $((${num_args} - 1))) ; do echo \"\${args[${i}]}\" ; done)
				num_emitted=$((num_emitted + 1))
				num_args=0
				continue 2
			else
				args[${num_args}]=${arg}
				num_args=$(($num_args + 1))
			fi
		done

		# output what is in args, or force empty profile
		if [ -n "$args" -o $num_emitted -eq 0 ] ; then
			eval emit_profile \"$imagename\" \
				$(for i in $(seq 0 $((${num_args} - 1))) ; do echo \"\${args[${i}]}\" ; done)
		fi

		break
	done

	# if old and new profiles consist of the same entries
	# we can do a replace, else remove/reload
	if [ $profileloaded -eq 1 ]
	then
		names1=$tmpdir/sorted1
		names2=$tmpdir/sorted2
		sort $profilenames > $names1
		sort ${profilenames}.old > $names2

		if cmp -s $names1 $names2
		then
			replaceprofile
		else	
			removeprofile ${profile}.old
			loadprofile
		fi

		rm -f $names1 $names2
	
	else
		loadprofile
	fi

	if [ -e ${sys_profiles} ] ; then
		#check to see if the profiles are actually loaded
		for f in `cat $profilenames` ; do
			grep -Eq "^$f"' \([^)]+\)$' ${sys_profiles}
			rc=$?
			if [ $rc -ne 0 ] ; then
				echo "Genprofile failed to load profile \"$f\""
				exit 1
			fi
		done
	fi

	rm -f ${profile}.old ${profilenames}.old
}

loadprofile()
{
	#global complainflaf profile profileloaded

	$subdomain ${parser_args} $complainflag $profile > /dev/null
	if [ $? -ne 0 ]
	then
		removeprofile
		fatalerror "Unable to load profile $profile"
	else
		profileloaded=1
	fi
}

replaceprofile()
{
	#global complainflag profile

	$subdomain ${parser_args} -r $complainflag $profile > /dev/null
	if [ $? -ne 0 ]
	then
		fatalerror "Unable to replace profile $profile"
	fi
}

removeprofile()
{
	local remprofile
	#global profile profileloaded

	if [ -f "$1" ]
	then
		remprofile=$1
	else
		remprofile=$profile
	fi

	$subdomain ${parser_args} -R $remprofile > /dev/null
	if [ $? -ne 0 ]
	then
		fatalerror "Unable to remove profile $remprofile"
	else
		profileloaded=0
	fi
}

settest()
{
	if [ -z "${__NO_TRAP_ERR}" ]
	then
		trap "error_handler" ERR
	fi

	#global test testname testexec outfile profileloaded

	#testname is the basename of the test, i,e 'open'
	#test is the full path to the test executable.
	#testexec is the path than will be run, normally this is the same
	#  as $test, but occasionally, you may want to invoke a wrapper which
	#  will run the test. In this case 'settest <testname> "wrapper {}'
	#  will result in testexec invoking wrapper. {} will be replaced with 
	#  $test 

	testname=$1

	if [ $# -eq 1 ]
	then
		test=$bin/$1
		testexec=$test
	elif [ $# -eq 2 ]
	then
		test=$bin/$1
		testexec=`echo $2 | sed "s~{}~$test~"`
	else
		fatalerror "settest, illegal usage"
	fi

	outfile=$tmpdir/output.$1

	# Remove any current profile if loaded
	if [ $profileloaded -eq 1 ]
	then
		removeprofile
	fi
}

# ----------------------------------------------------------------------------

# MAIN

trap "exit_handler" EXIT
trap "error_handler" ERR 2> /dev/null
if [ $? -ne 0 ]
then
	__NO_TRAP_ERR="true"
fi


if [ `whoami` != "root" ]
then
	fatalerror "Must be root to run $0"
fi

if [ ! -d "$bin" ]
then
	fatalerror "$0 requires \$bin pointing to binary directory"
fi

# parse arguments. 
# -r/-retain: flag to retain last failing testcase in tmpdir
if [ "$1" == "-retain" -o "$1" == "-r" ]
then
	retaintmpdir=true
else
	retaintmpdir=false
fi

# load user changeable variables
. $bin/uservars.inc

if [ ! -x $subdomain ]
then
	fatalerror "AppArmor parser '$subdomain' is not executable"
fi

profileloaded=0

tmpdir=$(mktemp -d $tmpdir-XXXXXX)
chmod 755 ${tmpdir}
export tmpdir

#set initial testname based on name of script
settest `basename $0 .sh`

profile=$tmpdir/profile
profilenames=$tmpdir/profile.names
num_testfailures=0	# exit code of script is set to #failures
