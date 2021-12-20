#!/bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME owlsm
#=DESCRIPTION 
# AppArmor implements a portion of the OWLSM functionality related to hard and 
# symbolic links. Creating a hard link (as a non root user or more accurately, 
# without CAP_FUSER) to a file owned by another user is disallowed.  Following
# a symbolic link in a directory with the sticky bit set is not allowed if the 
# link is owned by a different user than the directory. Note that these 
# restrictions are for all processes, not just confined ones.
#=END

# Test whether or not the owlsm stuff is functional
# don't need to generate subdomain profiles for this.

echo "owlsm mediation in AppArmor has been removed"; exit 1

pwd=$(dirname $0)
pwd=$(cd ${pwd} ; /bin/pwd)

bin=${pwd}

. ${bin}/prologue.inc

target=file1
source=file2

OWLSM_TOGGLE_FILE=/subdomain/control/owlsm
# Save the current state of the owlsm module, and disable it
if [ -f "${OWLSM_TOGGLE_FILE}" ] ; then 
	OWLSM_SAVED=$(cat ${OWLSM_TOGGLE_FILE})
	OWLSM_TOGGLE=true
fi

dir=$(mktemp -d ${tmpdir}/tmp.XXXXXXXXXXXXXXX)
chmod 1775 ${dir}
touch ${dir}/${target}

# test making a hardlink (as non-root) to a file owned by someone else.
settest dropprivs_wrapper
if [ -n "${OWLSM_TOGGLE}" ] ; then 
	echo 0 >  ${OWLSM_TOGGLE_FILE}
	runchecktest "[owlsm-disabled] hardlink to unowned file" pass link ${dir}/${target} ${dir}/${source}
	echo 1 >  ${OWLSM_TOGGLE_FILE}
	rm -f ${dir}/${source}
fi

runchecktest "[owlsm] hardlink to unowned file" fail link ${dir}/${target} ${dir}/${source}
rm -f ${dir}/${source}

#
# test not following a symlink owned by someone else in a +t directory
#
settest open
ln -s ${target} ${dir}/${source}
chown --no-dereference nobody ${dir}/${source}

if [ -n "${OWLSM_TOGGLE}" ] ; then 
	echo 0 >  ${OWLSM_TOGGLE_FILE}
	runchecktest "[owlsm-disabled] following non-owned symlink" pass ${dir}/${source}
	echo 1 >  ${OWLSM_TOGGLE_FILE}
fi

runchecktest "[owlsm] following non-owned symlink" fail ${dir}/${source}

# cleanup -- be paranoid in case there was an error in the mktemp call.
rm -f ${dir}/${target} ${dir}/${source}
rmdir ${dir}
if [ -n "${OWLSM_TOGGLE}" ] ; then 
	echo ${OWLSM_SAVED}  > ${OWLSM_TOGGLE_FILE}
fi

