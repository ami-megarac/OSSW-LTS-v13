SUBDIRS=apparmor

.PHONY: clean
clean:
	for dir in $(SUBDIRS) ; do \
		$(MAKE) clean -C $${dir} ; \
	done
