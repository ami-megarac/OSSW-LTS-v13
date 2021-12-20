#!/bin/bash

PROFILE_COUNT=50
KEEP_FILES=0
LOAD_PROFILES=0
STRESS_ARGS=
APPARMOR_PARSER="/sbin/apparmor_parser"

usage () {
	echo 'stress.sh [-klh] [-c count] [-s seed] [-p parser]'
	echo '  -c count	generate _count_ number of profiles'
	echo '  -h		usage (this message)'
	echo '  -k		keep files after completion'
	echo '  -l		attempt to load profiles into kernel'
	echo '  -p parser	use _parser_ instead of /sbin/apparmor_parser'
	echo '  -s seed	use _seed_ as random seed value'
	echo '$Id$'
	exit 0
}

while getopts 'klc:s:p:h' OPTION ; do
	case $OPTION in
	    k) KEEP_FILES=1
	       ;;
	    l) LOAD_PROFILES=1
	       ;;
	    c) PROFILE_COUNT=$OPTARG
	       ;;
	    s) STRESS_ARGS="${STRESS_ARGS} -s ${OPTARG}"
	       ;;
	    p) APPARMOR_PARSER=$OPTARG
	       ;;
	    h) usage
	       ;;
        esac
done

# stress.rb exports the profile locations it generatees in PROFILEDIR
# and PROFILESINGLE
echo "Generating ${PROFILE_COUNT} profiles..."
eval $(./stress.rb ${STRESS_ARGS} ${PROFILE_COUNT})

if [ ! -d "${PROFILEDIR}" -o ! -f "${PROFILESINGLE}" ] ; then
	echo "Generated profiles don't exist! Aborting...."
	exit 1
fi

cleanup () {
	if [ ${KEEP_FILES} == 0 ] ; then
		rm -rf "${PROFILEDIR}"
		rm "${PROFILESINGLE}"
	else
		echo "Files kept in ${PROFILEDIR} and ${PROFILESINGLE}"
	fi
}

timedir () {
	COMMAND=$1
	DESCRIPTION=$2
	echo "$DESCRIPTION"
	time for profile in ${PROFILEDIR}/* ; do
		${COMMAND} ${profile} > /dev/null
	done
}

timesingle () {
	COMMAND=$1
	DESCRIPTION=$2
	echo "$DESCRIPTION"
	time ${COMMAND} ${PROFILESINGLE} > /dev/null
}

remove_profiles () {
	echo "Unloading profiles..."
	(for profile in $(grep "^/does/not/exist" /sys/kernel/security/apparmor/profiles | cut -d " " -f 1); do
		echo "${profile} {} "
	done) | ${APPARMOR_PARSER} -K -R > /dev/null
}

# load files into buffer cache
timedir "cat" "Loading directory of profiles into buffer cache"
timedir "${APPARMOR_PARSER} -dd -Q" "Running preprocess only parser on directory of profiles"
timedir "${APPARMOR_PARSER} -S" "Running full parser on directory of profiles"
if [ "${LOAD_PROFILES}" == 1 ] ; then
	if [ "$(whoami)" == 'root' ] ; then
		timedir "${APPARMOR_PARSER}" "Parsing/loading directory of profiles"
		remove_profiles
	else
		echo "Not root, skipping load test..."
	fi
fi

timesingle "cat" "Loading equivalent profile into buffer cache"
timesingle "${APPARMOR_PARSER} -dd -Q" "Running preprocess only parser on single equiv profile"
timesingle "${APPARMOR_PARSER} -S" "Running full parser on single equivalent profile"
if [ "${LOAD_PROFILES}" == 1 ] ; then
	if [ "$(whoami)" == 'root' ] ; then
		timesingle "${APPARMOR_PARSER}" "Parsing/loading single file of profiles"
		remove_profiles
	else
		echo "Not root, skipping load test..."
	fi
fi

cleanup
