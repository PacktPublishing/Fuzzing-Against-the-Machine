# generate Direct3D11 temporary include

ifdef HAVE_CROSS_COMPILE
IDL_INCLUDES = -I/usr/include/wine/windows/ -I/usr/include/wine/wine/windows/
else
#ugly way to get the default location of standard idl files
IDL_INCLUDES = -I/`echo $(MSYSTEM) | tr A-Z a-z`/$(BUILD)/include
endif

D3D11_COMMIT_ID := a0cd5afeb60be3be0860e9a203314c10485bb9b8
D3D11_1_COMMIT_ID := aa6ab47929a9cac6897f38e630ce0bb88458e288
D3D11_4_COMMIT_ID := 6a1e782bb60bb1a93b5ab20fe895394d9c0904c2
DXGI12_COMMIT_ID := 790a6544347b53c314b9c6f1ea757a2d5504c67e
DXGITYPE_COMMIT_ID := f4aba520d014ecfe3563e33860de001caf2804e2
D3D11_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11.idl?format=raw
D3D11_1_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_1_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_1.h?format=raw
D3D11_2_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_1_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_2.h?format=raw
D3D11_3_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_1_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_3.h?format=raw
D3D11_4_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_4_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_4.h?format=raw
DXGI12_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(DXGI12_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_2.idl?format=raw
DXGITYPE_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(DXGITYPE_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgitype.h?format=raw
DXGIFORMAT_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(DXGITYPE_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgiformat.h?format=raw
DXGI_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(DXGITYPE_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi.idl?format=raw
DST_D3D11_H = $(PREFIX)/include/d3d11.h
DST_D3D11_1_H = $(PREFIX)/include/d3d11_1.h
DST_D3D11_2_H = $(PREFIX)/include/d3d11_2.h
DST_D3D11_3_H = $(PREFIX)/include/d3d11_3.h
DST_D3D11_4_H = $(PREFIX)/include/d3d11_4.h
DST_DXGIDEBUG_H = $(PREFIX)/include/dxgidebug.h
DST_DXGITYPE_H = $(PREFIX)/include/dxgitype.h
DST_DXGIFORMAT_H = $(PREFIX)/include/dxgiformat.h
DST_DXGI_IDL = $(PREFIX)/include/dxgi.idl
DST_DXGI12_H = $(PREFIX)/include/dxgi1_2.h
DST_DXGI13_H = $(PREFIX)/include/dxgi1_3.h
DST_DXGI14_H = $(PREFIX)/include/dxgi1_4.h
DST_DXGI15_H = $(PREFIX)/include/dxgi1_5.h
DST_DXGI16_H = $(PREFIX)/include/dxgi1_6.h


ifdef HAVE_WIN32
PKGS += d3d11
endif

$(TARBALLS)/d3d11.idl:
	$(call download_pkg,$(D3D11_IDL_URL),d3d11)

$(TARBALLS)/d3d11_1.h:
	$(call download_pkg,$(D3D11_1_H_URL),d3d11)

$(TARBALLS)/d3d11_2.h:
	$(call download_pkg,$(D3D11_2_H_URL),d3d11)

$(TARBALLS)/d3d11_3.h:
	$(call download_pkg,$(D3D11_3_H_URL),d3d11)

$(TARBALLS)/d3d11_4.h:
	$(call download_pkg,$(D3D11_4_H_URL),d3d11)

$(TARBALLS)/dxgidebug.idl:
	(cd $(TARBALLS) && patch -fp1) < $(SRC)/d3d11/dxgidebug.patch

$(TARBALLS)/dxgi1_2.idl:
	$(call download_pkg,$(DXGI12_IDL_URL),d3d11)

$(TARBALLS)/dxgitype.h:
	$(call download_pkg,$(DXGITYPE_H_URL),d3d11)

$(TARBALLS)/dxgiformat.h:
	$(call download_pkg,$(DXGIFORMAT_H_URL),d3d11)

$(TARBALLS)/dxgi.idl:
	$(call download_pkg,$(DXGI_IDL_URL),d3d11)

.sum-d3d11: $(TARBALLS)/d3d11.idl $(TARBALLS)/d3d11_1.h $(TARBALLS)/d3d11_2.h $(TARBALLS)/d3d11_3.h $(TARBALLS)/d3d11_4.h $(TARBALLS)/dxgidebug.idl $(TARBALLS)/dxgi1_2.idl $(TARBALLS)/dxgitype.h $(TARBALLS)/dxgiformat.h $(TARBALLS)/dxgi.idl

d3d11: .sum-d3d11
	mkdir -p $@
	cp $(TARBALLS)/d3d11.idl $@ && cd $@ && patch -fp1 < ../$(SRC)/d3d11/processor_format.patch

dxgi12: .sum-d3d11
	mkdir -p $@
	cp $(TARBALLS)/dxgi1_2.idl $@ && cd $@ && patch -fp1 < ../$(SRC)/d3d11/dxgi12.patch

$(DST_D3D11_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $</d3d11.idl

$(DST_D3D11_1_H): $(TARBALLS)/d3d11_1.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_1.h $@

$(DST_D3D11_2_H): $(TARBALLS)/d3d11_2.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_2.h $@

$(DST_D3D11_3_H): $(TARBALLS)/d3d11_3.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_3.h $@

$(DST_D3D11_4_H): $(TARBALLS)/d3d11_4.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_4.h $@

$(DST_DXGIDEBUG_H): $(TARBALLS)/dxgidebug.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGITYPE_H): $(TARBALLS)/dxgitype.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/dxgitype.h $@

$(DST_DXGIFORMAT_H): $(TARBALLS)/dxgiformat.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/dxgiformat.h $@

$(DST_DXGI_IDL): $(TARBALLS)/dxgi.idl
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/dxgi.idl $@

$(DST_DXGI12_H): dxgi12
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $</dxgi1_2.idl

$(DST_DXGI13_H): $(SRC)/d3d11/dxgi1_3.idl $(DST_DXGI12_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI14_H): $(SRC)/d3d11/dxgi1_4.idl $(DST_DXGI13_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI15_H): $(SRC)/d3d11/dxgi1_5.idl $(DST_DXGI14_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI16_H): $(SRC)/d3d11/dxgi1_6.idl $(DST_DXGI15_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

.dxgitype: $(DST_DXGITYPE_H) $(DST_DXGIFORMAT_H) $(DST_DXGI_IDL)
	touch $@

.dxgi12: .dxgitype $(DST_DXGI12_H)
	touch $@

.dxgi13: .dxgi12 $(DST_DXGI13_H)
	touch $@

.dxgi14: .dxgi13 $(DST_DXGI14_H)
	touch $@

.dxgi15: .dxgi14 $(DST_DXGI15_H)
	touch $@

.dxgi16: .dxgi15 $(DST_DXGI16_H)
	touch $@

.d3d11: $(DST_D3D11_H) $(DST_D3D11_1_H) $(DST_D3D11_2_H) $(DST_D3D11_3_H) $(DST_D3D11_4_H) $(DST_DXGIDEBUG_H) .dxgi16
	touch $@
