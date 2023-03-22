# libvpx

VPX_VERSION := 1.9.0
VPX_URL := http://github.com/webmproject/libvpx/archive/v${VPX_VERSION}.tar.gz

PKGS += vpx
ifeq ($(call need_pkg,"vpx >= 1.5.0"),)
PKGS_FOUND += vpx
endif

$(TARBALLS)/libvpx-$(VPX_VERSION).tar.gz:
	$(call download_pkg,$(VPX_URL),vpx)

.sum-vpx: libvpx-$(VPX_VERSION).tar.gz

libvpx: libvpx-$(VPX_VERSION).tar.gz .sum-vpx
	$(UNPACK)
	$(APPLY) $(SRC)/vpx/libvpx-ios.patch
ifdef HAVE_ANDROID
	$(APPLY) $(SRC)/vpx/libvpx-android.patch
	cp "${ANDROID_NDK}"/sources/android/cpufeatures/cpu-features.c $(UNPACK_DIR)/vpx_ports
	cp "${ANDROID_NDK}"/sources/android/cpufeatures/cpu-features.h $(UNPACK_DIR)
endif
ifdef HAVE_MACOSX
	$(APPLY) $(SRC)/vpx/darwin-do-not-overwrite-ld.patch
ifeq ($(ARCH),aarch64)
	$(APPLY) $(SRC)/vpx/libvpx-darwin-aarch64.patch
endif
endif
	$(MOVE)

DEPS_vpx =

ifdef HAVE_WIN32
DEPS_vpx += pthreads $(DEPS_pthreads)
endif

ifdef HAVE_CROSS_COMPILE
VPX_CROSS := $(HOST)-
else
VPX_CROSS :=
endif

VPX_LDFLAGS := $(LDFLAGS)

ifeq ($(ARCH),arm)
ifdef HAVE_IOS
ifneq ($(filter armv7s%,$(subst -, ,$(HOST))),)
VPX_ARCH := armv7s
else
VPX_ARCH := armv7
endif
else
VPX_ARCH := armv7
endif
else ifeq ($(ARCH),i386)
VPX_ARCH := x86
else ifeq ($(ARCH),mips)
VPX_ARCH := mips32
else ifeq ($(ARCH),ppc)
VPX_ARCH := ppc32
else ifeq ($(ARCH),ppc64)
VPX_ARCH := ppc64
else ifeq ($(ARCH),sparc)
VPX_ARCH := sparc
else ifeq ($(ARCH),x86_64)
VPX_ARCH := x86_64
else ifeq ($(ARCH),aarch64)
VPX_ARCH := arm64
endif

ifdef HAVE_ANDROID
VPX_OS := android
else ifdef HAVE_LINUX
VPX_OS := linux
else ifdef HAVE_DARWIN_OS
VPX_CROSS :=
ifeq ($(ARCH),$(filter $(ARCH), arm aarch64))
VPX_OS := darwin
else
VPX_OS := darwin11
endif
else ifdef HAVE_SOLARIS
VPX_OS := solaris
else ifdef HAVE_WIN64 # must be before WIN32
VPX_OS := win64
else ifdef HAVE_WIN32
VPX_OS := win32
else ifdef HAVE_BSD
VPX_OS := linux
endif

VPX_TARGET := generic-gnu
ifdef VPX_ARCH
ifdef VPX_OS
VPX_TARGET := $(VPX_ARCH)-$(VPX_OS)-gcc
endif
endif

VPX_CONF := \
	--disable-docs \
	--disable-examples \
	--disable-unit-tests \
	--disable-install-bins \
	--disable-install-docs \
	--disable-dependency-tracking \
	--enable-vp9-highbitdepth \
	--disable-tools

ifneq ($(filter arm aarch64, $(ARCH)),)
# Only enable runtime cpu detect on architectures other than arm/aarch64
# when building for Windows and Darwin
ifndef HAVE_WIN32
ifndef HAVE_DARWIN_OS
VPX_CONF += --enable-runtime-cpu-detect
endif
endif
endif

ifndef BUILD_ENCODERS
VPX_CONF += --disable-vp8-encoder --disable-vp9-encoder
endif

ifndef HAVE_WIN32
VPX_CONF += --enable-pic
else
VPX_CONF += --extra-cflags="-mstackrealign"
endif
ifdef HAVE_DARWIN_OS
VPX_CONF += --enable-vp8-decoder --disable-tools
VPX_CONF += --extra-cflags="$(CFLAGS) $(EXTRA_CFLAGS)"
ifdef HAVE_IOS
ifdef HAVE_TVOS
VPX_LDFLAGS := -L$(IOS_SDK)/usr/lib -isysroot $(IOS_SDK) -mtvos-version-min=9.0
else
VPX_LDFLAGS := -L$(IOS_SDK)/usr/lib -isysroot $(IOS_SDK) -miphoneos-version-min=8.4
endif
endif
ifdef HAVE_MACOSX
VPX_LDFLAGS := -L$(MACOSX_SDK)/usr/lib -isysroot $(MACOSX_SDK) -mmacosx-version-min=10.7
endif
VPX_LDFLAGS += -arch $(PLATFORM_SHORT_ARCH)
endif

ifneq ($(filter i386 x86_64,$(ARCH)),)
# broken text relocations or invalid register for .seh_savexmm with gcc8
VPX_CONF += --disable-mmx
endif

ifndef WITH_OPTIMIZATION
VPX_CONF += --enable-debug --disable-optimizations
endif

ifdef HAVE_ANDROID
# Starting NDK19, standalone toolchains are deprecated and gcc is not shipped.
# The presence of gcc can be used to detect if we are using an old standalone
# toolchain. Unfortunately, libvpx buildsystem only work with standalone
# toolchains, therefore pass the HOSTVARS directly to bypass any detection.
ifneq ($(shell $(VPX_CROSS)gcc -v >/dev/null 2>&1 || echo FAIL),)
VPX_HOSTVARS = $(HOSTVARS)
endif
endif

.vpx: libvpx
	rm -rf $(PREFIX)/include/vpx
	cd $< && LDFLAGS="$(VPX_LDFLAGS)" CROSS=$(VPX_CROSS) $(VPX_HOSTVARS) ./configure --target=$(VPX_TARGET) \
		$(VPX_CONF) --prefix=$(PREFIX)
	cd $< && $(MAKE)
	$(call pkg_static,"vpx.pc")
	cd $< && $(MAKE) install
	touch $@
