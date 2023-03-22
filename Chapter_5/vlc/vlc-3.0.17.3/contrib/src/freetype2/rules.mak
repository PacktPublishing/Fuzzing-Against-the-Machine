# freetype2

FREETYPE2_VERSION := 2.11.1
FREETYPE2_URL := $(SF)/freetype/freetype2/$(FREETYPE2_VERSION)/freetype-$(FREETYPE2_VERSION).tar.xz

PKGS += freetype2
ifeq ($(call need_pkg,"freetype2"),)
PKGS_FOUND += freetype2
endif

$(TARBALLS)/freetype-$(FREETYPE2_VERSION).tar.xz:
	$(call download_pkg,$(FREETYPE2_URL),freetype2)

.sum-freetype2: freetype-$(FREETYPE2_VERSION).tar.xz

freetype: freetype-$(FREETYPE2_VERSION).tar.xz .sum-freetype2
	$(UNPACK)
	$(APPLY) $(SRC)/freetype2/0001-builds-windows-Guard-some-non-ancient-API.patch
	$(APPLY) $(SRC)/freetype2/0001-builds-windows-Add-support-for-legacy-UWP-builds.patch
	$(call pkg_static, "builds/unix/freetype2.in")
	$(MOVE)

DEPS_freetype2 = zlib $(DEPS_zlib)

.freetype2: freetype
ifndef AD_CLAUSES
	$(REQUIRE_GPL)
endif
	cd $< && cp builds/unix/install-sh .
	sed -i.orig s/-ansi// $</builds/unix/configure
	cd $< && GNUMAKE=$(MAKE) $(HOSTVARS) ./configure --with-harfbuzz=no --with-zlib=yes --without-png --with-bzip2=no $(HOSTCONF)
	cd $< && $(MAKE) && $(MAKE) install
	touch $@
