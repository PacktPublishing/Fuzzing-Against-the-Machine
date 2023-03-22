# PROJECTM
PROJECTM_VERSION := 2.0.1
PROJECTM_URL := $(SF)/projectm/$(PROJECTM_VERSION)/projectM-$(PROJECTM_VERSION)-Source.tar.gz

ifdef HAVE_WIN32
ifneq ($(ARCH),arm)
ifneq ($(ARCH),aarch64)
PKGS += projectM
endif
endif
endif
ifeq ($(call need_pkg,"libprojectM"),)
PKGS_FOUND += projectM
endif

$(TARBALLS)/projectM-$(PROJECTM_VERSION)-Source.tar.gz:
	$(call download_pkg,$(PROJECTM_URL),projectM)

.sum-projectM: projectM-$(PROJECTM_VERSION)-Source.tar.gz

projectM: projectM-$(PROJECTM_VERSION)-Source.tar.gz .sum-projectM
	$(UNPACK)
ifdef HAVE_WIN64
	$(APPLY) $(SRC)/projectM/win64.patch
endif
ifdef HAVE_WIN32
	$(APPLY) $(SRC)/projectM/win32.patch
endif
	$(APPLY) $(SRC)/projectM/gcc6.patch
	$(APPLY) $(SRC)/projectM/clang6.patch
	$(MOVE)

DEPS_projectM = glew $(DEPS_glew)

.projectM: projectM toolchain.cmake
	cd $< && rm -f CMakeCache.txt
	cd $< && $(HOSTVARS) $(CMAKE) \
		-DINCLUDE-PROJECTM-LIBVISUAL:BOOL=OFF \
		-DDISABLE_NATIVE_PRESETS:BOOL=ON \
		-DUSE_FTGL:BOOL=OFF \
		-DINCLUDE-PROJECTM-PULSEAUDIO:BOOL=OFF \
		-DINCLUDE-PROJECTM-QT:BOOL=OFF \
		-DBUILD_PROJECTM_STATIC:BOOL=ON .
	cd $< && $(CMAKEBUILD) . --target install
	-cd $<; cp Renderer/libRenderer.a MilkdropPresetFactory/libMilkdropPresetFactory.a $(PREFIX)/lib
	touch $@
