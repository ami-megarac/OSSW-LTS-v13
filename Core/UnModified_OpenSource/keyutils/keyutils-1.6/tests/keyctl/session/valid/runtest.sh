#!/bin/bash

. ../../../prepare.inc.sh
. ../../../toolbox.inc.sh


# ---- do the actual testing ----

result=PASS
echo "++++ BEGINNING TEST" >$OUTPUTFILE

# describe the keyring created for a named session
marker "NAMED SESSION"
new_session qwerty keyctl rdescribe @s "@"
expect_key_rdesc rdesc "keyring@.*@.*@.*@qwerty"

# check the session keyring ID is shown
seskeyring="`tail -2 $OUTPUTFILE | head -1`"
if ! expr "$seskeyring" : "Joined session keyring: [0-9]*" >&/dev/null
then
    failed
fi

echo "++++ FINISHED TEST: $result" >>$OUTPUTFILE

# --- then report the results in the database ---
toolbox_report_result $TEST $result
