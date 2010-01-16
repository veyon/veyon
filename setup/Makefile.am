AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(srcdir)/../lib/include -Isrc

noinst_PROGRAMS = setup


%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<

%.uic: %.ui
	$(UIC) -o $@ $<

%.ts: 
	$(LUPDATE) $(srcdir)/src/*.cpp $(srcdir)/dialogs/*.ui -ts $(srcdir)/resources/$@ -locations none -no-obsolete
	
%.qm: %.ts
	$(LRELEASE) $(srcdir)/resources/$< -qm $(srcdir)/resources/$@

setup_MOC =	./src/dialogs.moc
setup_UIC =	./dialogs/wizard.uic			\
		./dialogs/page_welcome.uic		\
		./dialogs/page_security_options.uic	\
		./dialogs/page_setup_finished.uic	\
		./dialogs/page_key_dirs.uic

BUILT_SOURCES = $(setup_MOC) $(setup_UIC) ./setup_qrc.cpp


setup_TRANSLATIONS = $(wildcard $(srcdir)/resources/*.qm $(srcdir)/resources/*.ts)


./setup_qrc.cpp: setup.qrc
	$(RCC) $< > $@


setup_SOURCES = $(srcdir)/src/main.cpp				\
		$(srcdir)/src/dialogs.cpp			\
		$(srcdir)/src/dialogs.h				\
		$(srcdir)/setup_qrc.cpp				\
		$(srcdir)/setup.qrc				\
		$(setup_TRANSLATIONS)				\
		$(wildcard $(srcdir)/dialogs/*.ui)		\
		$(wildcard $(srcdir)/resources/*.png)

EXTRA_DIST =	setup.ico

CLEANFILES = $(setup_MOC) $(setup_UIC) ./setup_qrc.cpp


if HAVE_LIBSSL
LIBSSL_LDADD = -lssl -lcrypto
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif

if BUILD_WIN32
setup_win_resources.o: setup.rc
	$(WINDRES) -o $@ $<
PLATFORM_LDADD = setup_win_resources.o -L../lib/ -litalc_core
else
PLATFORM_LDADD = ../lib/libitalc_core-static.a
endif

setup_LDADD = $(QT_LDADD) $(QT_LIB_GUI) $(LIBSSL_LDADD) -ljpeg $(PLATFORM_LDADD)
setup_LDFLAGS = $(LDFLAGS) -mwindows

AM_CXXFLAGS = $(QT_CXXFLAGS)

