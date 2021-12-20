#!/bin/bash

#
APPARMOR_PARSER="${APPARMOR_PARSER:-../apparmor_parser}"

# Format of -D dfa-states
# dfa-states output is split into 2 parts:
#   the accept state infomation
#       {state} (allow deny audit XXX)  ignore XXX for now
#    followed by the transition table information
#       {Y} -> {Z}: 0xXX Char   #0xXX is the hex dump of Char
# where the start state is always shown as
# {1} <==
#
# Eg. echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, deny /** w, }" | ./apparmor_parser -QT -O minimize -D dfa-states --quiet
#
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 0/2800a/0/2800a)
# {4} (0x 10004/2800a/0/2800a)
# {7} (0x 40010/2800a/0/2800a)
# {8} (0x 80020/2800a/0/2800a)
# {9} (0x 100040/2800a/0/2800a)
# {c} (0x 40030/0/0/0)
#
# {1} -> {2}: 0x2f /
# {2} -> {4}: 0x61 a
# {2} -> {3}: 0x62 b
# {2} -> {3}: 0x63 c
# {2} -> {7}: 0x64 d
# {2} -> {8}: 0x65 e
# {2} -> {9}: 0x66 f
# {2} -> {3}: [^\0x0/]
# {3} -> {3}: [^\0x0]
# {4} -> {3}: [^\0x0]
# {7} -> {a}: 0x0
# {7} -> {3}: []
# {8} -> {3}: [^\0x0]
# {9} -> {3}: [^\0x0]
# {a} -> {b}: 0x2f /
# {b} -> {c}: [^/]
# {c} -> {c}: []
#
# These tests currently only look at the accept state permissions
#
# To view any of these DFAs as graphs replace --D dfa-states with -D dfa-graph
# strip of the test stuff around the parser command and use the the dot
# command to convert
# Eg.
# echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, deny /** w, }" | ./apparmor_parser -QT -O minimize -D dfa-graph --quiet 2>min.graph
# dot -T png -o min.png min.graph
# and then view min.png in your favorite viewer
#
#------------------------------------------------------------------------
# test to see if minimization is eliminating non-redundant accept state
# Test xtrans and regular perms separately.  The are the same basic failure
# but can xtrans has an extra code path.
#
# The permission test is setup to have all the none xtrans permissions show
# up once on unique paths and have a global write permission that adds to
# it.
# This should result in a un-minimized dump looking like. Notice it has 6
# states with accept information, 1 for each rule except for the 'w'
# permission which is combined into a single state for /b and /**
#
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 2800a/0/0/0)
# {4} (0x 3800e/0/0/0)
# {5} (0x 6801a/0/0/0)
# {6} (0x a802a/0/0/0)
# {7} (0x 12804a/0/0/0)
# {a} (0x 40030/0/0/0)
# A dump of minimization that is not respecting the uniqueness of the
# permissions on the states looks like below.  Notice it has only 3 states
# with accept information
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 1b806e/0/0/0)
# {5} (0x 6801a/0/0/0)
# {a} (0x 40030/0/0/0)

echo -n "Minimize profiles basic perms "
if [ `echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, /** w, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 6 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"

# same test as above except with audit perms added
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 2800a/0/2800a/0)
# {4} (0x 3800e/0/2800a/0)
# {7} (0x 6801a/0/2800a/0)
# {8} (0x a802a/0/2800a/0)
# {9} (0x 12804a/0/2800a/0)
# {c} (0x 40030/0/0/0)
echo -n "Minimize profiles audit perms "
if [ `echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, audit /** w, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 6 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"

# same test as above except with deny 'w' perm added to /**, this does not
# elimnates the states with 'w' and 'a' because the quiet information is
# being carried
#
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 0/2800a/0/2800a)
# {4} (0x 10004/2800a/0/2800a)
# {7} (0x 40010/2800a/0/2800a)
# {8} (0x 80020/2800a/0/2800a)
# {9} (0x 100040/2800a/0/2800a)
# {c} (0x 40030/0/0/0)

echo -n "Minimize profiles deny perms "
if [ `echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, deny /** w, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 6 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"

# same test as above except with audit deny 'w' perm added to /**, with the
# parameter this elimnates the states with 'w' and 'a' because
# the quiet information is NOT being carried
#
# {1} <== (allow/deny/audit/quiet)
# {4} (0x 10004/0/0/0)
# {7} (0x 40010/0/0/0)
# {8} (0x 80020/0/0/0)
# {9} (0x 100040/0/0/0)
# {c} (0x 40030/0/0/0)

echo -n "Minimize profiles audit deny perms "
if [ `echo "/t { /a r, /b w, /c a, /d l, /e k, /f m, audit deny /** w, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 5 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"


# The x transition test profile is setup so that there are 3 conflicting x
# permissions, two are on paths that won't collide during dfa creation.  The
# 3rd is a generic permission that should be overriden during dfa creation.
#
# This should result in a dfa that specifies transitions on 'a' and 'b' to
# unique states that store the alternate accept information.  However
# minimization can remove the unique accept permission states if x permissions
# are treated as a single accept state.
#
# The minimized dump should retain the 'a' and 'b' transitions accept states.
# notice the below dump has 3 states with accept information {3}, {4}, {5}
#
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 2914a45/0/0/0)
# {4} (0x 4115045/0/0/0)
# {5} (0x 2514945/0/0/0)
#
# A dump of minimization that is not respecting the uniqueness of the
# permissions on the states transitioned to by 'a' and 'b' looks like
# below.  Notice that only state {3} has accept information
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 2514945/0/0/0)
#

echo -n "Minimize profiles xtrans "
if [ `echo "/t { /b px, /* Pixr, /a Cx -> foo, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 3 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"

# same test as above + audit
echo -n "Minimize profiles audit xtrans "
if [ `echo "/t { /b px, audit /* Pixr, /a Cx -> foo, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 3 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"


# now try denying x and make sure perms are cleared
# notice that only deny and quiet information is being carried
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 0/fe17f85/0/14005)

echo -n "Minimize profiles deny xtrans "
if [ `echo "/t { /b px, deny /* xr, /a Cx -> foo, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 1 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"

# now try audit + denying x and make sure perms are cleared
# notice that the deny info is being carried, by an artifical trap state
# {1} <== (allow/deny/audit/quiet)
# {3} (0x 0/fe17f85/0/0)

echo -n "Minimize profiles audit deny xtrans "
if [ `echo "/t { /b px, audit deny /* xr, /a Cx -> foo, }" | ${APPARMOR_PARSER} -M features_files/features.nopolicydb -QT -O minimize -D dfa-states 2>&1 | grep -v '<==' | grep '^{.*} (.*)$' | wc -l` -ne 0 ] ; then
    echo "failed"
    exit 1;
fi
echo "ok"
