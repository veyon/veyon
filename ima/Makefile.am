AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(srcdir)/../lib/include -Isrc

bin_PROGRAMS = italc

%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<

%.uic: %.ui
	$(UIC) -o $@ $<

%.ts: 
	$(LUPDATE) $(srcdir)/src/*.cpp $(srcdir)/dialogs/*.ui -ts $(srcdir)/resources/$@
	
%.qm: %.ts
	$(LRELEASE) $(srcdir)/resources/$< -qm $(srcdir)/resources/$@


italc_MOC =	./src/client.moc			\
		./src/classroom_manager.moc		\
		./src/cmd_input_dialog.moc		\
		./src/config_widget.moc			\
		./src/dialogs.moc			\
		./src/italc_side_bar.moc		\
		./src/kmultitabbar.moc			\
		./src/main_window.moc			\
		./src/overview_widget.moc		\
		./src/remote_control_widget.moc		\
		./src/snapshot_list.moc			\
		./src/tool_bar.moc			\
		./src/tool_button.moc

italc_UIC =	./dialogs/client_settings.uic		\
		./dialogs/config.uic			\
		./dialogs/support.uic			\
		./dialogs/text_message.uic		\
		./dialogs/remote_logon.uic		\
		./dialogs/overview.uic			\
		./dialogs/snapshots.uic			\
		./dialogs/about.uic

BUILT_SOURCES = $(italc_MOC) $(italc_UIC) ./italc_qrc.cpp


italc_TRANSLATIONS = $(wildcard $(srcdir)/resources/*.qm $(srcdir)/resources/*.ts)


./italc_qrc.cpp: italc.qrc
	$(RCC) $< > $@


italc_SOURCES = $(srcdir)/src/client.cpp			\
		$(srcdir)/src/classroom_manager.cpp		\
		$(srcdir)/src/cmd_input_dialog.cpp		\
		$(srcdir)/src/config_widget.cpp			\
		$(srcdir)/src/dialogs.cpp			\
		$(srcdir)/src/tool_bar.cpp			\
		$(srcdir)/src/italc_side_bar.cpp		\
		$(srcdir)/src/main_window.cpp			\
		$(srcdir)/src/kmultitabbar.cpp			\
		$(srcdir)/src/local_system_ima.cpp		\
		$(srcdir)/src/main.cpp				\
		$(srcdir)/src/overview_widget.cpp		\
		$(srcdir)/src/remote_control_widget.cpp		\
		$(srcdir)/src/snapshot_list.cpp			\
		$(srcdir)/src/side_bar_widget.cpp		\
		$(srcdir)/src/tool_button.cpp			\
		$(srcdir)/src/client.h				\
		$(srcdir)/src/classroom_manager.h		\
		$(srcdir)/src/cmd_input_dialog.h		\
		$(srcdir)/src/config_widget.h 			\
		$(srcdir)/src/tool_bar.h 			\
		$(srcdir)/src/dialogs.h				\
		$(srcdir)/src/local_system_ima.h		\
		$(srcdir)/src/main_window.h			\
		$(srcdir)/src/italc_side_bar.h			\
		$(srcdir)/src/kmultitabbar.h			\
		$(srcdir)/src/overview_widget.h			\
		$(srcdir)/src/qplatformdefs.h			\
		$(srcdir)/src/remote_control_widget.h		\
		$(srcdir)/src/snapshot_list.h			\
		$(srcdir)/src/side_bar_widget.h			\
		$(srcdir)/src/tool_button.h			\
		$(srcdir)/italc_qrc.cpp				\
		$(srcdir)/italc.qrc				\
		$(italc_TRANSLATIONS)				\
		$(wildcard $(srcdir)/dialogs/*.ui)		\
		$(wildcard $(srcdir)/resources/*.png)


CLEANFILES = $(italc_MOC) $(italc_UIC) ./italc_qrc.cpp

man_MANS=italc.1

menudir = $(prefix)/share/menu
menu_DATA = data/italc

applicationsdir = $(prefix)/share/applications
applications_DATA = data/italc.desktop

pixmapsdir = $(prefix)/share/pixmaps
pixmaps_DATA = data/italc.xpm

iconsdir = $(prefix)/share/icons
icons_DATA = data/italc.png



EXTRA_DIST =	$(man_MANS)				\
		$(wildcard $(srcdir)/src/3rdparty/*)	\
		data/italc.desktop.in			\
		data/italc.in				\
		$(pixmaps_DATA)				\
		$(icons_DATA)				\
		data/italc.ico

if BUILD_WIN32
italc_win_resources.o: italc.rc
	$(WINDRES) -o $@ $<
WIN32_RES_LDADD = italc_win_resources.o
endif

if HAVE_LIBSSL
LIBSSL_LDADD = -lssl -lcrypto
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif


italc_LDADD = $(QT_LDADD) $(QT_LIB_GUI) -lz -ljpeg $(LIBSSL_LDADD) $(WIN32_RES_LDADD) -L../lib -litalc_core
italc_LDFLAGS = $(LDFLAGS) -mwindows -rpath $(pkglibdir)

AM_CXXFLAGS = $(QT_CXXFLAGS)

