#       Copyright (c) 2010 - 2014
#       Canonical Ltd. (All rights reserved)
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

## v5 ptrace tests except with failures where appropriate. Testing that capability ptrace
## does not grant ptrace perms

## Note: ptrace tests need signal permissions to function correctly
##       signal permissions are not actually needed by all tests to function but
##	 we grant signal perms to all to be consistent

echo "   using ptrace v6 tests ..."

################################################################################
# v5 ptrace tests without ptrace rules
################################################################################

#unconfined tracing confined helper
#confined helper asking unconfined process to ptrace it
genprofile image=$helper signal:ALL ptrace:tracedby:peer=unconfined

runchecktest "test 3 -h" pass -h -n 100 $helper
runchecktest "test 3 -hc " pass -h -c -n 100 $helper
# can't exec ${bin_true} so fail
runchecktest "test 3 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 3 -hc prog" fail -h -c -n 100 $helper ${bin_true}

# lack of 'r' perm is currently not working
genprofile image=$helper $helper:ix signal:ALL
runchecktest "test 4 -h" pass -h -n 100 $helper
runchecktest "test 4 -hc " pass -h -c -n 100 $helper
# can't exec ${bin_true} so fail
runchecktest "test 4 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 4 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile image=$helper $helper:rix signal:ALL
runchecktest "test 5 -h" pass -h -n 100 $helper
runchecktest "test 5 -hc " pass -h -c -n 100 $helper
# can't exec ${bin_true} so fail
runchecktest "test 5 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 5 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile image=$helper $helper:ix ${bin_true}:rix signal:ALL
runchecktest "test 6 -h" pass -h -n 100 $helper
runchecktest "test 6 -hc " pass -h -c -n 100 $helper
runchecktest "test 6 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 6 -hc prog" pass -h -c -n 100 $helper ${bin_true}

#traced child can ptrace_me to unconfined have unconfined trace them
genprofile image=${bin_true} signal:ALL
runchecktest "test 7" pass -n 100 ${bin_true}
# pass - ptrace_attach is done in unconfined helper
runchecktest "test 7 -c " pass -c -n 100 ${bin_true}
runchecktest "test 7 -h" pass -h -n 100 $helper
# pass - ptrace_attach is done in unconfined helper
runchecktest "test 7 -hc " pass -h -c -n 100 $helper
runchecktest "test 7 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 7 -hc prog" pass -h -c -n 100 $helper ${bin_true}

genprofile image=$helper $helper:ix ${bin_true}:rix signal:ALL
runchecktest "test 7a" pass -n 100 ${bin_true}
# pass - ptrace_attach is allowed from confined process to unconfined
runchecktest "test 7a -c " pass -c -n 100 ${bin_true}
runchecktest "test 7a -h" pass -h -n 100 $helper
# pass - ptrace_attach is allowed from confined process to unconfined
runchecktest "test 7a -hc " pass -h -c -n 100 $helper
runchecktest "test 7a -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 7a -hc prog" pass -h -c -n 100 $helper ${bin_true}

#traced helper from unconfined
genprofile image=$helper $helper:ix ${bin_true}:rpx signal:ALL -- image=${bin_true} signal:ALL
runchecktest "test 8" pass -n 100 ${bin_true}
# pass - ptrace_attach is done before exec
runchecktest "test 8 -c " pass -c -n 100 ${bin_true}
runchecktest "test 8 -h" pass -h -n 100 $helper
runchecktest "test 8 -hc " pass -h -c -n 100 $helper
# pass - can px if tracer can ptrace target
runchecktest "test 8 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 8 -hc prog" pass -h -c -n 100 $helper ${bin_true}

#traced helper from unconfined
genprofile image=$helper $helper:ix ${bin_true}:rux signal:ALL -- image=${bin_true} signal:ALL
runchecktest "test 9" pass -n 100 ${bin_true}
# pass - ptrace_attach is done before exec
runchecktest "test 9 -c " pass -c -n 100 ${bin_true}
runchecktest "test 9 -h" pass -h -n 100 $helper
runchecktest "test 9 -hc " pass -h -c -n 100 $helper
# pass - can ux if tracer can ptrace target
runchecktest "test 9 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 9 -hc prog" pass -h -c -n 100 $helper ${bin_true}

genprofile signal:ALL
# fail due to no exec permission
runchecktest "test 10" fail -n 100 ${bin_true}
runchecktest "test 10 -c" fail -c -n 100 ${bin_true}
runchecktest "test 10 -h" fail -h -n 100 $helper
runchecktest "test 10 -hc" fail -h -c -n 100 $helper
runchecktest "test 10 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 10 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile ${bin_true}:ix $helper:ix signal:ALL
# fail due to missing r permission
#runchecktest "test 11" fail -n 100 ${bin_true}
#runchecktest "test 11 -c" fail -c -n 100 ${bin_true}
#runchecktest "test 11 -h" fail -h -n 100 $helper
#runchecktest "test 11 -hc" fail -h -c -n 100 $helper
#runchecktest "test 11 -h prog" fail -h -n 100 $helper ${bin_true}
#runchecktest "test 11 -hc prog" fail -h -c -n 100 $helper ${bin_true}

# fail was pass in v5 allowed to ix self
genprofile ${bin_true}:rix $helper:rix signal:ALL
runchecktest "test 12" fail -n 100 ${bin_true}
runchecktest "test 12 -c" fail -c -n 100 ${bin_true}
runchecktest "test 12 -h" fail -h -n 100 $helper
runchecktest "test 12 -hc" fail -h -c -n 100 $helper
runchecktest "test 12 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 12 -hc prog" fail -h -c -n 100 $helper ${bin_true}

#ptraced confined app traced by unconfined can px
genprofile image=$helper $helper:rix ${bin_true}:rpx signal:ALL -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13u -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13u -hc prog" pass -h -c -n 100 $helper ${bin_true}

#ptraced confined app traced by profile without ptrace on targeted can't px
genprofile ${bin_true}:rpx signal:ALL -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13 -hc prog" fail -h -c -n 100 $helper ${bin_true}


#ptraced confined app can ux - if the tracer is unconfined
#
genprofile image=$helper $helper:rix ${bin_true}:rux signal:ALL
runchecktest "test 14a -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 14a -hc prog" pass -h -c -n 100 $helper ${bin_true}
#ptraced confined app can't ux - if the tracer can't trace unconfined
genprofile $helper:rpx signal:ALL -- image=$helper $helper:rix ${bin_true}:rux signal:ALL
runchecktest "test 14b -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 14b -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an unconfined app
genprofile $helper:rux signal:ALL
runchecktest "test 15 -h" fail -h -n 100 $helper
runchecktest "test 15 -h prog" fail -h -n 100 $helper ${bin_true}
#an unconfined app can't ask a confined app to trace it
runchecktest "test 15 -hc" fail -h -c -n 100 $helper
runchecktest "test 15 -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an app confined by a different profile
genprofile $helper:rpx signal:ALL -- image=$helper signal:ALL
runchecktest "test 15 -h" fail -h -n 100 $helper
runchecktest "test 15 -h prog" fail -h -n 100 $helper ${bin_true}
#a confined app can't ask another confined app with a different profile to
#trace it
runchecktest "test 15 -hc" fail -h -c -n 100 $helper
runchecktest "test 15 -hc prog" fail -h -c -n 100 $helper ${bin_true}

################### cap:sys_ptrace doesn't change results from above ##########################
# fail was pass in v5 allowed to ix self
genprofile ${bin_true}:rix $helper:rix signal:ALL cap:sys_ptrace
runchecktest "test 12c" fail -n 100 ${bin_true}
runchecktest "test 12c -c" fail -c -n 100 ${bin_true}
runchecktest "test 12c -h" fail -h -n 100 $helper
runchecktest "test 12c -hc" fail -h -c -n 100 $helper
runchecktest "test 12c -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 12c -hc prog" fail -h -c -n 100 $helper ${bin_true}

#ptraced confined app traced by unconfined can px
genprofile image=$helper $helper:rix ${bin_true}:rpx signal:ALL cap:sys_ptrace -- image=${bin_true} ${bin_true}:rix cap:sys_ptrace
runchecktest "test 13cu -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13cu -hc prog" pass -h -c -n 100 $helper ${bin_true}

#ptraced confined app traced by profile without ptrace on targeted can't px
genprofile ${bin_true}:rpx signal:ALL cap:sys_ptrace -- image=${bin_true} ${bin_true}:rix cap:sys_ptrace
runchecktest "test 13c -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13c -hc prog" fail -h -c -n 100 $helper ${bin_true}


#ptraced confined app can ux - if the tracer is unconfined
#
genprofile image=$helper $helper:rix ${bin_true}:rux signal:ALL cap:sys_ptrace
runchecktest "test 14ca -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 14ca -hc prog" pass -h -c -n 100 $helper ${bin_true}
#ptraced confined app can't ux - if the tracer can't trace unconfined
genprofile $helper:rpx signal:ALL -- image=$helper $helper:rix ${bin_true}:rux signal:ALL
runchecktest "test 14cb -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 14cb -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an unconfined app
genprofile $helper:rux signal:ALL cap:sys_ptrace
runchecktest "test 15c -h" fail -h -n 100 $helper
runchecktest "test 15c -h prog" fail -h -n 100 $helper ${bin_true}
#an unconfined app can't ask a confined app to trace it
runchecktest "test 15c -hc" fail -h -c -n 100 $helper
runchecktest "test 15c -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an app confined by a different profile
genprofile $helper:rpx signal:ALL cap:sys_ptrace -- image=$helper signal:ALL cap:sys_ptrace
runchecktest "test 15c -h" fail -h -n 100 $helper
runchecktest "test 15c -h prog" fail -h -n 100 $helper ${bin_true}
#a confined app can't ask another confined app with a different profile to
#trace it
runchecktest "test 15c -hc" fail -h -c -n 100 $helper
runchecktest "test 15c -hc prog" fail -h -c -n 100 $helper ${bin_true}


################################################################################
# v5 ptrace tests with ptrace rules
################################################################################

##### Now do tests with ptrace rules in profiles #######
# pass in v5 allowed to ix self
genprofile ${bin_true}:rix $helper:rix signal:ALL ptrace:ALL
runchecktest "test 12p" pass -n 100 ${bin_true}
runchecktest "test 12p -c" pass -c -n 100 ${bin_true}
runchecktest "test 12p -h" pass -h -n 100 $helper
runchecktest "test 12p -hc" pass -h -c -n 100 $helper
runchecktest "test 12p -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 12p -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rix $helper:rix signal:ALL ptrace:peer=$test
runchecktest "test 12p1" pass -n 100 ${bin_true}
runchecktest "test 12p1 -c" pass -c -n 100 ${bin_true}
runchecktest "test 12p1 -h" pass -h -n 100 $helper
runchecktest "test 12p1 -hc" pass -h -c -n 100 $helper
runchecktest "test 12p1 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 12p1 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rix $helper:rix signal:ALL ptrace:peer=notaprofile
runchecktest "test 12p2" fail -n 100 ${bin_true}
runchecktest "test 12p2 -c" fail -c -n 100 ${bin_true}
runchecktest "test 12p2 -h" fail -h -n 100 $helper
runchecktest "test 12p2 -hc" fail -h -c -n 100 $helper
runchecktest "test 12p2 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 12p2 -hc prog" fail -h -c -n 100 $helper ${bin_true}


#ptraced confined app traced by profile can px
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p1 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p2 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p3 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p4 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p5 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p6 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p7 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p8 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p9 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe -hc prog" fail -h -c -n 100 $helper ${bin_true}


genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p11 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p21 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p31 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p41 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p51 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p61 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p71 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p81 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p91 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa1 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb1 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc1 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:trace:peer=${bin_true} -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd1 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe1 -hc prog" fail -h -c -n 100 $helper ${bin_true}


genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p12 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p22 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p32 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p42 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p52 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 13p62 -hc prog" pass -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p72 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p82 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p92 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa2 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb2 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc2 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:ALL -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd2 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe2 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p13 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p23 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p33 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p43 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p53 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p63 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p73 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p83 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p93 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa3 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb3 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc3 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd3 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe3 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p14 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p24 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p34 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p44 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p54 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p64 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p74 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p84 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p94 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa4 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb4 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc4 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:tracedby:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd4 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe4 -hc prog" fail -h -c -n 100 $helper ${bin_true}

genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix
runchecktest "test 13p15 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p25 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby
runchecktest "test 13p35 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p45 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=$test
runchecktest "test 13p55 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p65 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:tracedby:peer=notaprofile
runchecktest "test 13p75 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13p85 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace
runchecktest "test 13p95 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pa5 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=$test
runchecktest "test 13pb5 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pc5 -hc prog" fail -h -c -n 100 $helper ${bin_true}
genprofile ${bin_true}:rpx $helper:rix signal:ALL ptrace:peer=$test ptrace:peer=notaprofile -- image=${bin_true} ${bin_true}:rix ptrace:trace:peer=notaprofile
runchecktest "test 13pd5 -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 13pe5 -hc prog" fail -h -c -n 100 $helper ${bin_true}


### todo Variations of below tests


#ptraced confined app can ux - if the tracer is unconfined
#
genprofile image=$helper $helper:rix ${bin_true}:rux signal:ALL
runchecktest "test 14pa -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 14pa -hc prog" pass -h -c -n 100 $helper ${bin_true}
#ptraced confined app can't ux - if the tracer can't trace unconfined
genprofile $helper:rpx signal:ALL -- image=$helper $helper:rix ${bin_true}:rux signal:ALL
runchecktest "test 14pb -h prog" fail -h -n 100 $helper ${bin_true}
runchecktest "test 14pb -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an unconfined app
genprofile $helper:rux signal:ALL
runchecktest "test 15p -h" fail -h -n 100 $helper
runchecktest "test 15p -h prog" fail -h -n 100 $helper ${bin_true}
#an unconfined app can't ask a confined app to trace it
runchecktest "test 15p -hc" fail -h -c -n 100 $helper
runchecktest "test 15p -hc prog" fail -h -c -n 100 $helper ${bin_true}

#confined app can't ptrace an app confined by a different profile
genprofile $helper:rpx signal:ALL -- image=$helper signal:ALL
runchecktest "test 15p -h" fail -h -n 100 $helper
runchecktest "test 15p -h prog" fail -h -n 100 $helper ${bin_true}
#a confined app can't ask another confined app with a different profile to
#trace it
runchecktest "test 15p -hc" fail -h -c -n 100 $helper
runchecktest "test 15p -hc prog" fail -h -c -n 100 $helper ${bin_true}

# Test LP: #1390592
# The bug was a policy compilation bug that triggers in a rule such as
# 'ptrace peer=ABC,'. The first character of the peer conditional value must be
# a-f|A-F|0-9 to trigger the bug. A parser affected by this bug will create a
# bad binary policy that causes the kernel to unexpectedly deny the ptrace
# 'trace' of a process confined by profile ABC.
genprofile "$helper rpx -> ABC" signal:ALL ptrace:trace:peer=ABC -- image=ABC addimage:$helper ${bin_true}:rix signal:ALL ptrace:tracedby:peer=$test
runchecktest "test LP: #1390592 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test LP: #1390592 -hc prog" pass -h -c -n 100 $helper ${bin_true}

## TODO: ptrace read tests
## TODO: ptrace + change_profile
## TODO: ptrace + change_hat
