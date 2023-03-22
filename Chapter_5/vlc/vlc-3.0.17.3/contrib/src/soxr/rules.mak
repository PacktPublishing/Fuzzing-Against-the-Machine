# SoXR

SOXR_VERSION := 0.1.3
SOXR_URL := http://vorboss.dl.sourceforge.net/project/soxr/soxr-$(SOXR_VERSION)-Source.tar.xz

ifeq ($(call need_pkg,"soxr >= 0.1"),)
PKGS_FOUND += soxr
endif
DEPS_soxr = ffmpeg $(DEPS_ffmpeg)

$(TARBALLS)/soxr-$(SOXR_VERSION)-Source.tar.xz:
	$(call download_pkg,$(SOXR_URL),soxr)

.sum-soxr: soxr-$(SOXR_VERSION)-Source.tar.xz

soxr: soxr-$(SOXR_VERSION)-Source.tar.xz .sum-soxr
	$(UNPACK)
	$(APPLY) $(SRC)/soxr/0001-always-generate-.pc.patch
	$(APPLY) $(SRC)/soxr/0002-expose-Libs.private-in-.pc.patch
	$(APPLY) $(SRC)/soxr/0003-add-aarch64-support.patch
	$(APPLY) $(SRC)/soxr/0004-arm-fix-SIGILL-when-doing-divisions-on-some-old-arch.patch
	$(APPLY) $(SRC)/soxr/find_ff_pkgconfig.patch
	$(call pkg_static,"src/soxr.pc.in")
	$(MOVE)

# Force CMAKE_CROSSCOMPILING to True
ifdef HAVE_CROSS_COMPILE
SOXR_EXTRA_CONF=-DCMAKE_SYSTEM_NAME=Generic
endif

.soxr: soxr toolchain.cmake
	rm -f $</CMakeCache.txt
	cd $< && $(HOSTVARS_PIC) $(CMAKE) \
		$(SOXR_EXTRA_CONF) \
		-DBUILD_EXAMPLES=OFF \
		-DBUILD_TESTS=OFF \
		-DWITH_LSR_BINDINGS=OFF \
		-DWITH_OPENMP=OFF \
		-DWITH_AVFFT=ON \
		-Wno-dev
	cd $< && $(CMAKEBUILD) . --target install
	touch $@
