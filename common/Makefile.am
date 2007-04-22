
%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<


%.ts: 
	$(LUPDATE) $(wildcard $(srcdir)/src/*.cpp) -ts $(srcdir)/resources/$@

%.qm: %.ts
	$(LRELEASE) $< -qm $@ 




./italc_core_qrc.cpp: italc_core.qrc
	rcc $< > $@


lib_LTLIBRARIES = libitalc_core.la
libitalc_core_la_SOURCES =			\
		src/dsa_key.cpp			\
		src/fast_qimage.cpp		\
		src/isd_base.cpp		\
		src/isd_connection.cpp		\
		src/ivs_connection.cpp		\
		src/local_system.cpp		\
		src/lock_widget.cpp		\
		src/messagebox.cpp		\
		src/minilzo.c			\
		src/progress_widget.cpp		\
		src/rect_list.cpp		\
		src/system_key_trapper.cpp	\
		src/system_service.cpp		\
		src/vncview.cpp			\
		include/debug.h			\
		include/dsa_key.h		\
		include/fast_qimage.h		\
		include/isd_base.h		\
		include/isd_connection.h	\
		include/italc_rfb_ext.h		\
		include/ivs_connection.h	\
		include/local_system.h		\
		include/lock_widget.h		\
		include/lzoconf.h		\
		include/messagebox.h		\
		include/minilzo.h		\
		include/progress_widget.h	\
		include/qt_features.h		\
		include/qt_user_events.h	\
		include/rect_list.h		\
		include/types.h			\
		include/system_key_trapper.h	\
		include/system_service.h	\
		include/vncview.h		\
		include/rfb/keysym.h		\
		include/rfb/rfb.h		\
		include/rfb/rfbproto.h

libitalc_core_la_LDFLAGS = -version-info 1:0:0 -no-undefined -shared

libitalc_core_la_MOC =
		include/vncview.moc		\
		include/system_key_trapper.moc	\
		include/progress_widget.moc	\
		include/isd_connection.moc	\
		include/ivs_connection.moc

CLEANFILES =	include/rfb/rfbconfig.h		\
		include/rfb/rfbint.h		\
		$(libitalc_core_la_MOC)

BUILT_SOURCES = $(libitalc_core_la_MOC)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -O2 -DBUILD_ICA -DNO_LUPUS_INTEGRATION $(CXXFLAGS_ADD)

INCLUDES = -I$(srcdir)/include -I$(top_srcdir)

