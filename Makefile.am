AUTOMAKE_OPTIONS = foreign 1.4

if BUILD_WIN32
EXTRA_TARGETS = utils-win32
endif

SUBDIRS = common ica ima setup $(subdirs) $(EXTRA_TARGETS)

EXTRA_DIST = build_mingw32 tightvnc4win_integrate tightvnc4win_italc.diff

nobase_doc_DATA = AUTHORS ChangeLog COPYING INSTALL README TODO

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


if BUILD_WIN32
win32-pkg: all
	mkdir -p tmp/italc
	cp ica/ica.exe tmp/italc
	cp ima/italc.exe tmp/italc
	cp setup/setup.exe tmp/italc
	cp utils-win32/userinfo.exe tmp/italc
	cp utils-win32/wake.exe tmp/italc
	cp lupus/lupus/lupus.exe tmp/italc
#	cp lupus/service/lupus_service.exe tmp/italc
	cp /opt/mingw/bin/QtCore4.dll tmp/italc
	cp /opt/mingw/bin/QtGui4.dll tmp/italc
	cp /opt/mingw/bin/QtNetwork4.dll tmp/italc
	cp /opt/mingw/bin/QtXml4.dll tmp/italc
	cp /opt/mingw/bin/libeay32.dll tmp/italc
	cp /opt/mingw/bin/libjpeg.dll tmp/italc
	cp /opt/mingw/bin/libssl32.dll tmp/italc
	cp /opt/mingw/bin/libz.dll tmp/italc
	cp /opt/mingw/i586-mingw32/bin/mingwm10.dll tmp/italc
	/opt/mingw/bin/i586-mingw32-strip tmp/italc/ica.exe
	/opt/mingw/bin/i586-mingw32-strip tmp/italc/italc.exe
	/opt/mingw/bin/i586-mingw32-strip tmp/italc/setup.exe
	/opt/mingw/bin/i586-mingw32-strip tmp/italc/userinfo.exe
	/opt/mingw/bin/i586-mingw32-strip tmp/italc/lupus.exe
#	/opt/mingw/bin/i586-mingw32-strip tmp/italc/lupus_service.exe
	cd tmp && zip -9 ../$(PACKAGE)-$(VERSION).zip italc/*
	rm -rf tmp
endif
