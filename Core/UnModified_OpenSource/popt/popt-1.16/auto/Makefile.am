AUTOMAKE_OPTIONS = 1.4 foreign

AUTOTEST =      api-sanity-checker

TDIRS =		descriptors_storage header_compile_errors test_results tests

clean-local:
	rm -rf $(TDIRS)

check-local:
	-[ -d tests ] && ${AUTOTEST} -l popt -d popt.xml -clean
	-${AUTOTEST} -l popt -d popt.xml -st types -gen -build -run
