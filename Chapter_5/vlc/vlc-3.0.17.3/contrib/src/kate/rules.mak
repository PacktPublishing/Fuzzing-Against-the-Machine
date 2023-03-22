# Kate

KATE_VERSION := 0.4.1
KATE_URL := http://libkate.googlecode.com/files/libkate-$(KATE_VERSION).tar.gz

PKGS += kate
ifeq ($(call need_pkg,"kate >= 0.1.5"),)
PKGS_FOUND += kate
endif

$(TARBALLS)/libkate-$(KATE_VERSION).tar.gz:
	$(call download_pkg,$(KATE_URL),kate)

.sum-kate: libkate-$(KATE_VERSION).tar.gz

libkate: libkate-$(KATE_VERSION).tar.gz .sum-kate
	$(UNPACK)
	$(UPDATE_AUTOCONFIG)
	$(MOVE)
	mv libkate/config.sub libkate/config.guess libkate/misc/autotools

DEPS_kate = ogg $(DEPS_ogg)

.kate: libkate
	cd $< && $(HOSTVARS) ./configure $(HOSTCONF) \
		--disable-valgrind \
		--disable-doc
	cd $< && $(MAKE) SUBDIRS=. install
	touch $@
