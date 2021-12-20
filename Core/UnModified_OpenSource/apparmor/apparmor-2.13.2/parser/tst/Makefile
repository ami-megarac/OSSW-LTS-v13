#
PROVE=/usr/bin/prove
TESTS=simple.pl
PARSER_DIR=..
PARSER_BIN=apparmor_parser
PARSER=$(PARSER_DIR)/$(PARSER_BIN)
# parser.conf to use in tests. Note that some test scripts have the parser options hardcoded, so passing PARSER_ARGS=... is not enough to override it.
PARSER_ARGS=--config-file=./parser.conf
PROVE_ARG=-f

ifeq ($(VERBOSE),1)
  PROVE_ARG+=-v
  PYTEST_ARG = -v
else
  VERBOSE=
endif

all: tests

.PHONY: tests error_output gen_dbus gen_xtrans parser_sanity caching minimize equality valgrind
tests: error_output caching minimize equality parser_sanity

GEN_TRANS_DIRS=simple_tests/generated_x/ simple_tests/generated_perms_leading/ simple_tests/generated_perms_safe/ simple_tests/generated_dbus

gen_xtrans: $(GEN_TRANS_DIRS)
	./gen-xtrans.pl

$(GEN_TRANS_DIRS):
	mkdir $@

gen_dbus: $(GEN_TRANS_DIRS)
	./gen-dbus.pl

error_output: $(PARSER)
	LANG=C $(PARSER) $(PARSER_ARGS) -S -I errors >/dev/null errors/okay.sd
	LANG=C $(PARSER) $(PARSER_ARGS) -S -I errors 2>&1 >/dev/null errors/single.sd | \
		grep -q "AppArmor parser error for errors/single.sd in errors/single.sd at line 3: Could not open 'failure'"
	LANG=C $(PARSER) $(PARSER_ARGS) -S -I errors 2>&1 >/dev/null errors/double.sd | \
		grep -q "AppArmor parser error for errors/double.sd in errors/includes/busted at line 66: Could not open 'does-not-exist'"
	LANG=C $(PARSER) $(PARSER_ARGS) -S -I errors 2>&1 >/dev/null errors/modefail.sd | \
		grep -q "AppArmor parser error for errors/modefail.sd in errors/modefail.sd at line 6: syntax error"
	LANG=C $(PARSER) $(PARSER_ARGS) -S -I errors 2>&1 >/dev/null errors/multi_include.sd | \
		grep -q "AppArmor parser error for errors/multi_include.sd in errors/multi_include.sd at line 12: Could not open 'failure'"
	@echo "Error Output: PASS"

parser_sanity: $(PARSER) gen_xtrans gen_dbus
	$(Q)LANG=C APPARMOR_PARSER="$(PARSER)" ${PROVE} ${PROVE_ARG} ${TESTS}

caching: $(PARSER)
	LANG=C ./caching.py -p "$(PARSER)" $(PYTEST_ARG)

minimize: $(PARSER)
	LANG=C APPARMOR_PARSER="$(PARSER) $(PARSER_ARGS)" ./minimize.sh

equality: $(PARSER)
	LANG=C APPARMOR_PARSER="$(PARSER) $(PARSER_ARGS)" ./equality.sh

valgrind: $(PARSER) gen_xtrans gen_dbus
	LANG=C ./valgrind_simple.py -p "$(PARSER) $(PARSER_ARGS)" -v simple_tests

$(PARSER):
	$(MAKE) -C $(PARSER_DIR) $(PARSER_BIN)

clean:
	find $(GEN_TRANS_DIRS) -type f | xargs rm -f
	rm -f gmon.out
