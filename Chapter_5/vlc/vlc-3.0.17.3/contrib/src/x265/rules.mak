# x265

#X265_GITURL := https://github.com/videolan/x265
X265_VERSION := 2.7
X265_SNAPURL := https://bitbucket.org/multicoreware/x265/get/$(X265_VERSION).tar.bz2

ifdef BUILD_ENCODERS
ifdef GPL
PKGS += x265
endif
endif

ifeq ($(call need_pkg,"x265 >= 0.6"),)
PKGS_FOUND += x265
endif

$(TARBALLS)/x265-git.tar.xz:
	$(call download_git,$(X265_GITURL))

$(TARBALLS)/x265-$(X265_VERSION).tar.bz2:
	$(call download_pkg,$(X265_SNAPURL),x265)

.sum-x265: x265-$(X265_VERSION).tar.bz2

x265: x265-$(X265_VERSION).tar.bz2 .sum-x265
	rm -Rf $@-$(X265_VERSION)
	mkdir -p $@-$(X265_VERSION)
	tar xvjfo "$<" --strip-components=1 -C $@-$(X265_VERSION)
	$(APPLY) $(SRC)/x265/x265-ldl-linking.patch
	$(APPLY) $(SRC)/x265/x265-no-pdb-install.patch
	$(call pkg_static,"source/x265.pc.in")
ifndef HAVE_WIN32
	$(APPLY) $(SRC)/x265/x265-pkg-libs.patch
endif
	$(MOVE)

.x265: x265 toolchain.cmake
	$(REQUIRE_GPL)
	cd $</source && $(HOSTVARS_PIC) $(CMAKE) -DENABLE_SHARED=OFF -DCMAKE_SYSTEM_PROCESSOR=$(ARCH) -DENABLE_CLI=OFF
	cd $< && $(CMAKEBUILD) source --target install
	sed -e s/'[^ ]*clang_rt[^ ]*'//g -i.orig "$(PREFIX)/lib/pkgconfig/x265.pc"
	touch $@
