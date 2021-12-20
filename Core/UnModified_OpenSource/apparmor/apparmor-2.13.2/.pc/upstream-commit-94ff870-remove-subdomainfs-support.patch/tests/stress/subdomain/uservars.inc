if [ -e /sbin/subdomain_parser ] ; then
	subdomain_parser=/sbin/subdomain_parser
else
	subdomain_parser=/sbin/apparmor_parser
fi

if [ -n "`grep subdomainfs /proc/filesystems`" ]
then
	subdomainfs=/subdomain
elif [ -n "`grep securityfs /proc/filesystems`" ]
then
	if [ -e /sys/kernel/security/subdomain ] ; then
		subdomainfs=/sys/kernel/security/subdomain
	else
		subdomainfs=/sys/kernel/security/apparmor
	fi
else
	echo "$0: Unable to find subdomainfs" >&2
	exit 1
fi

if [ ! -x $subdomain_parser ]
then
	echo "$0: $subdomain_parser not executable" >&2
	exit 1
fi

if [ -z "`grep '^subdomain ' /proc/modules`" ]
then
	if [ -z "`grep '^apparmor ' /proc/modules`" ] ; then
		if [ ! -d "/sys/module/apparmor" ] ; then
			echo "$0: apparmor module not loaded" >&2
			exit 1
		fi
	fi
fi

if [ ! -w $subdomainfs/.load ]
then
	echo "$0: $subdomainfs/.load not present" >&2
	exit 1
fi
