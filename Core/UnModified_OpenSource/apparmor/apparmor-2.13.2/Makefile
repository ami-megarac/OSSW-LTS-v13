#
#
.PHONY: all
all:
	@echo "*** See README for information how to build AppArmor ***"
	exit 1

COMMONDIR=common
include ${COMMONDIR}/Make.rules

DIRS=libraries/libapparmor \
     binutils \
     parser \
     utils \
     changehat/mod_apparmor \
     changehat/pam_apparmor \
     profiles \
     tests

# with conversion to git, we don't export from the remote
REPO_URL?=git@gitlab.com:apparmor/apparmor.git
REPO_BRANCH?=apparmor-2.13

COVERITY_DIR=cov-int
RELEASE_DIR=apparmor-${VERSION}
__SETUP_DIR?=.

# We create a separate version for tags because git can't handle tags
# with embedded ~s in them. No spaces around '-' or they'll get
# embedded in ${VERSION}
# apparmor version tag format 'vX.Y.ZZ'
# apparmor branch name format 'apparmor-X.Y'
TAG_VERSION="v$(subst ~,-,${VERSION})"

# Add exclusion entries arguments for tar here, of the form:
#   --exclude dir_to_exclude --exclude other_dir
TAR_EXCLUSIONS=

.PHONY: tarball
tarball: clean
	REPO_VERSION=`$(value REPO_VERSION_CMD)` && \
	$(MAKE) export_dir __EXPORT_DIR=${RELEASE_DIR} __REPO_VERSION=$${REPO_VERSION} && \
	$(MAKE) setup __SETUP_DIR=${RELEASE_DIR} && \
	tar ${TAR_EXCLUSIONS} -cvzf ${RELEASE_DIR}.tar.gz ${RELEASE_DIR}

.PHONY: snapshot
snapshot: clean
	$(eval REPO_VERSION:=$(shell $(value REPO_VERSION_CMD)))
	$(eval SNAPSHOT_NAME=apparmor-$(VERSION)~$(shell echo $(REPO_VERSION) | cut -d '-' -f 2-))
	$(MAKE) export_dir __EXPORT_DIR=${SNAPSHOT_NAME} __REPO_VERSION=${REPO_VERSION} && \
	$(MAKE) setup __SETUP_DIR=${SNAPSHOT_NAME} && \
	tar ${TAR_EXCLUSIONS} -cvzf ${SNAPSHOT_NAME}.tar.gz ${SNAPSHOT_NAME}

.PHONY: coverity
coverity: snapshot
	cd $(SNAPSHOT_NAME)/libraries/libapparmor && ./configure --with-python
	$(foreach dir, $(filter-out utils profiles tests, $(DIRS)), \
		cov-build --dir $(COVERITY_DIR) -- $(MAKE) -C $(SNAPSHOT_NAME)/$(dir); \
		mv $(COVERITY_DIR)/build-log.txt $(COVERITY_DIR)/build-log-$(subst /,.,$(dir)).txt ;)
	$(foreach dir, libraries/libapparmor utils, \
		cov-build --dir $(COVERITY_DIR) --no-command --fs-capture-search $(SNAPSHOT_NAME)/$(dir); \
		mv $(COVERITY_DIR)/build-log.txt $(COVERITY_DIR)/build-log-python-$(subst /,.,$(dir)).txt ;)
	tar -cvzf $(SNAPSHOT_NAME)-$(COVERITY_DIR).tar.gz $(COVERITY_DIR)

.PHONY: export_dir
export_dir:
	mkdir $(__EXPORT_DIR)
	/usr/bin/git archive --prefix=$(__EXPORT_DIR)/ --format tar $(__REPO_VERSION) | tar xv
	echo "$(REPO_URL) $(REPO_BRANCH) $(__REPO_VERSION)" > $(__EXPORT_DIR)/common/.stamp_rev

.PHONY: clean
clean:
	-rm -rf ${RELEASE_DIR} ./apparmor-${VERSION}~* ${COVERITY_DIR}
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: setup
setup:
	cd $(__SETUP_DIR)/libraries/libapparmor && ./autogen.sh
	# parser has an extra doc to build
	$(MAKE) -C $(__SETUP_DIR)/parser extra_docs
	# libraries/libapparmor needs configure to have run before
	# building docs
	$(foreach dir, $(filter-out libraries/libapparmor tests, $(DIRS)), \
		$(MAKE) -C $(__SETUP_DIR)/$(dir) docs;)

.PHONY: tag
tag:
	git tag -m 'AppArmor $(VERSION)' -s $(TAG_VERSION)
