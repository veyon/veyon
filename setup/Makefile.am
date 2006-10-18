INCLUDES = -I$(srcdir)/../common/include -Isrc

bin_PROGRAMS = setup

%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<

%.uic: %.ui
	$(UIC) -o $@ $<

%.ts: 
	$(LUPDATE) $(srcdir)/src/*.cpp $(srcdir)/dialogs/*.ui -ts $(srcdir)/resources/$@
	
%.qm: %.ts
	$(LRELEASE) $<

setup_MOC =	./src/dialogs.moc
setup_UIC =	./dialogs/wizard.uic			\
		./dialogs/page_welcome.uic		\
		./dialogs/page_license_agreement.uic	\
		./dialogs/page_install_dir.uic		\
		./dialogs/page_select_components.uic	\
		./dialogs/page_security_options.uic	\
		./dialogs/page_key_dirs.uic

BUILT_SOURCES = $(setup_MOC) $(setup_UIC) ./setup_qrc.cpp


setup_TRANSLATIONS = $(wildcard $(srcdir)/resources/*.qm $(srcdir)/resources/*.ts)


./setup_qrc.cpp: setup.qrc
	rcc $< > $@


setup_SOURCES = $(srcdir)/src/main.cpp				\
		$(srcdir)/src/dialogs.cpp			\
		$(srcdir)/../common/src/dsa_key.cpp		\
		$(srcdir)/../common/src/local_system.cpp	\
		$(srcdir)/setup_qrc.cpp				\
		$(srcdir)/setup.qrc				\
		$(setup_TRANSLATIONS)				\
		$(wildcard $(srcdir)/dialogs/*.ui)		\
		$(wildcard $(srcdir)/resources/*.png)


CLEANFILES = $(setup_MOC) $(setup_UIC) ./setup_qrc.cpp


if HAVE_LIBSSL
LIBSSL_LDADD = -lssl
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif


setup_LDADD = $(QT_LDADD) $(LIBSSL_LDADD)
setup_LDFLAGS = $(LDFLAGS)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS)

