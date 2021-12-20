#!/bin/sh

. ./uservars.inc

if [ `whoami` != root ]
then
	echo "$0: must be root" >&2
	exit 1
fi

$subdomain_parser -R change_hat.profile 2>&1 > /dev/null
$subdomain_parser change_hat.profile 

./change_hat > /dev/null 2>&1 &

while :
do
	$subdomain_parser -r change_hat.profile > /dev/null 2>&1 &
done &

