AUTOMAKE_OPTIONS = foreign 1.4


SUBDIRS = common ica ima

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

