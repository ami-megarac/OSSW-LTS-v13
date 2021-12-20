#!/bin/sh

. ./uservars.inc

${subdomain_parser} change_hat.profile child.profile open.profile 

rm -f /tmp/foobar && touch /tmp/foobar

./open & ./open /tmp/foobar &

./child & ./child &

./change_hat > /dev/null 2>&1 & ./change_hat /tmp/foo > /dev/null 2>&1 &

while :
do
	${subdomain_parser} -r change_hat.profile > /dev/null 2>&1 &
	${subdomain_parser} -r child.profile > /dev/null 2>&1 &
	${subdomain_parser} -r open.profile > /dev/null 2>&1 &
done &

