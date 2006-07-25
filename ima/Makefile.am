INCLUDES = -I$(srcdir)/../common/include
bin_PROGRAMS = italc

%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<

%.uic: %.ui
	$(UIC) -o $@ $<

%.ts: 
	$(LUPDATE) $(srcdir)/src/*.cpp $(srcdir)/dialogs/*.ui -ts $(srcdir)/resources/$@
	
%.qm: %.ts
	$(LRELEASE) $<

italc_MOC =	./src/client.moc			\
		./src/client_manager.moc		\
		./src/client_settings_dialog.moc	\
		./src/cmd_input_dialog.moc		\
		./src/config_widget.moc			\
		./src/support_widget.moc		\
		./src/italc_side_bar.moc		\
		./src/kmultitabbar.moc			\
		./src/main_window.moc			\
		./src/msg_input_dialog.moc		\
		./src/overview_widget.moc		\
		./src/progress_information.moc		\
		./src/snapshot_list.moc			\
		./src/tool_button.moc			\
		./src/user_list.moc			\
		../common/include/isd_connection.moc	\
		../common/include/ivs_connection.moc

italc_UIC =	./dialogs/client_settings.uic		\
		./dialogs/support.uic			\
		./dialogs/about.uic

BUILT_SOURCES = $(italc_MOC) $(italc_UIC) ./italc_qrc.cpp


italc_TRANSLATIONS = $(wildcard $(srcdir)/resources/*.qm $(srcdir)/resources/*.ts)


./italc_qrc.cpp: italc.qrc
	rcc $< > $@


italc_SOURCES = $(srcdir)/src/about_dialog.cpp			\
		$(srcdir)/src/client.cpp			\
		$(srcdir)/src/client_manager.cpp		\
		$(srcdir)/src/client_settings_dialog.cpp	\
		$(srcdir)/src/cmd_input_dialog.cpp		\
		$(srcdir)/src/config_widget.cpp			\
		$(srcdir)/src/support_widget.cpp		\
		$(srcdir)/src/italc_side_bar.cpp		\
		$(srcdir)/src/main_window.cpp			\
		$(srcdir)/src/kmultitabbar.cpp			\
		$(srcdir)/src/main.cpp				\
		$(srcdir)/src/msg_input_dialog.cpp		\
		$(srcdir)/src/overview_widget.cpp		\
		$(srcdir)/src/progress_information.cpp		\
		$(srcdir)/src/snapshot_list.cpp			\
		$(srcdir)/src/side_bar_widget.cpp		\
		$(srcdir)/src/tool_button.cpp			\
		$(srcdir)/src/user_list.cpp			\
		$(srcdir)/src/about_dialog.h			\
		$(srcdir)/src/client.h				\
		$(srcdir)/src/client_manager.h			\
		$(srcdir)/src/client_settings_dialog.h		\
		$(srcdir)/src/cmd_input_dialog.h		\
		$(srcdir)/src/config_widget.h 			\
		$(srcdir)/src/support_widget.h			\
		$(srcdir)/src/main_window.h			\
		$(srcdir)/src/italc_side_bar.h			\
		$(srcdir)/src/kmultitabbar.h			\
		$(srcdir)/src/msg_input_dialog.h		\
		$(srcdir)/src/overview_widget.h			\
		$(srcdir)/src/progress_information.h		\
		$(srcdir)/src/snapshot_list.h			\
		$(srcdir)/src/side_bar_widget.h			\
		$(srcdir)/src/tool_button.h			\
		$(srcdir)/src/user_list.h			\
		$(srcdir)/italc_qrc.cpp				\
		$(srcdir)/italc.qrc				\
		$(srcdir)/../common/src/fast_qimage.cpp		\
		$(srcdir)/../common/src/isd_base.cpp		\
		$(srcdir)/../common/src/isd_connection.cpp	\
		$(srcdir)/../common/src/ivs_connection.cpp	\
		$(srcdir)/../common/src/local_system.cpp	\
		$(srcdir)/../common/src/messagebox.cpp		\
		$(srcdir)/../common/src/dsa_key.cpp		\
		$(italc_TRANSLATIONS)				\
		$(wildcard $(srcdir)/dialogs/*.ui)		\
		$(wildcard $(srcdir)/resources/*.png)


CLEANFILES = $(italc_MOC) $(italc_UIC) ./italc_qrc.cpp


if HAVE_LIBSSL
LIBSSL_LDADD = -lssl
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif


italc_LDADD = $(QT_LDADD) -lz -ljpeg $(LIBSSL_LDADD)
italc_LDFLAGS = $(LDFLAGS)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS)

