AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = lib ica ima setup $(subdirs) $(EXTRA_TARGETS)

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = build_mingw32 tightvnc4win_integrate tightvnc4win_italc.diff README.LZO

nobase_doc_DATA = AUTHORS ChangeLog COPYING INSTALL README README.LZO TODO

%.ts:
	cd lib && make $@

%.qm:
	cd lib && make $@


dist-hook:
	rm -rf `find $(distdir) -name *.moc`
	rm -rf `find $(distdir) -name *_qrc.cpp`

# strip all binaries after installation
install-exec-hook:
	cd $(DESTDIR)$(bindir) ; \
	strip ica; \
	strip italc


if HAVE_RPM
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif


#CLEANFILES = config.log config.status
clean: clean-recursive
	rm -f config.log
	rm -f config.status

clean-am: clean-generic clean-libtool mostlyclean-am

if BUILD_WIN32
win32-pkg: all
	mkdir -p tmp/italc
	cp lib/italc_core.dll tmp/italc
	cp ica/.libs/ica.exe tmp/italc
	cp ica/vnchooks.dll tmp/italc
	cp ima/.libs/italc.exe tmp/italc
	cp setup/.libs/setup.exe tmp/italc
	cp /opt/mingw32/bin/QtCore4.dll tmp/italc
	cp /opt/mingw32/bin/QtGui4.dll tmp/italc
	cp /opt/mingw32/bin/QtNetwork4.dll tmp/italc
	cp /opt/mingw32/bin/QtXml4.dll tmp/italc
	cp /opt/mingw32/bin/libeay32.dll tmp/italc
	cp /opt/mingw32/bin/libjpeg-8.dll tmp/italc
	cp /opt/mingw32/bin/libssl32.dll tmp/italc
	cp -L COPYING tmp/italc/LICENSE.TXT
	cp -L README tmp/italc/README.TXT
	/opt/mingw32/bin/i686-w64-mingw32-strip tmp/italc/ica.exe
	/opt/mingw32/bin/i686-w64-mingw32-strip tmp/italc/italc.exe
	/opt/mingw32/bin/i686-w64-mingw32-strip tmp/italc/setup.exe
	mv tmp/italc tmp/italc-$(VERSION)
	cd tmp && zip -9 ../$(PACKAGE)-$(VERSION)-bin-win32.zip italc-$(VERSION)/*
	rm -rf tmp

win32-nsi: win32-pkg
	unzip $(PACKAGE)-$(VERSION)-bin-win32.zip
	makensis iTALC.nsi
	rm -rf italc-$(VERSION)
	mv italc-$(VERSION)-setup.exe italc-$(VERSION)-win32-setup.exe


win64-pkg: all
	mkdir -p tmp/italc
	cp lib/italc_core.dll tmp/italc
	cp ica/.libs/ica.exe tmp/italc
	cp ica/vnchooks.dll tmp/italc
	cp ima/.libs/italc.exe tmp/italc
	cp setup/.libs/setup.exe tmp/italc
	cp /opt/mingw64/bin/QtCore4.dll tmp/italc
	cp /opt/mingw64/bin/QtGui4.dll tmp/italc
	cp /opt/mingw64/bin/QtNetwork4.dll tmp/italc
	cp /opt/mingw64/bin/QtXml4.dll tmp/italc
	cp /opt/mingw64/bin/libeay32.dll tmp/italc
	cp /opt/mingw64/bin/libjpeg-8.dll tmp/italc
	cp /opt/mingw64/bin/libssl32.dll tmp/italc
	cp -L COPYING tmp/italc/LICENSE.TXT
	cp -L README tmp/italc/README.TXT
	/opt/mingw64/bin/x86_64-w64-mingw32-strip tmp/italc/ica.exe
	/opt/mingw64/bin/x86_64-w64-mingw32-strip tmp/italc/italc.exe
	/opt/mingw64/bin/x86_64-w64-mingw32-strip tmp/italc/setup.exe
	mv tmp/italc tmp/italc-$(VERSION)
	cd tmp && zip -9 ../$(PACKAGE)-$(VERSION)-bin-win64.zip italc-$(VERSION)/*
	rm -rf tmp

win64-nsi: win64-pkg
	unzip $(PACKAGE)-$(VERSION)-bin-win64.zip
	makensis iTALC.nsi
	rm -rf italc-$(VERSION)
	mv italc-$(VERSION)-setup.exe italc-$(VERSION)-win64-setup.exe
endif
