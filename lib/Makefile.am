AUTOMAKE_OPTIONS = foreign

%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<

%.ts: 
	$(LUPDATE)							\
		$(srcdir)/src/*.cpp				\
		$(srcdir)/../ica/src/*.cpp		\
		$(srcdir)/../ima/src/*.cpp		\
		$(srcdir)/../ima/dialogs/*.ui	\
		$(srcdir)/../setup/src/*.cpp	\
		$(srcdir)/../setup/dialogs/*.ui	\
		-ts $(srcdir)/resources/$@		\
		-locations none -no-obsolete

%.qm: %.ts
	$(LRELEASE) $(srcdir)/resources/$< -qm $(srcdir)/resources/$@



./italc_core_qrc.cpp: italc_core.qrc
	$(RCC) $< -name italc_core > $@


noinst_LIBRARIES = libitalc_core-static.a

libitalc_core_static_a_MOC =				\
		include/vncview.moc			\
		include/system_key_trapper.moc		\
		include/progress_widget.moc		\
		include/isd_connection.moc		\
		include/ivs_connection.moc

CLEANFILES =	include/rfb/rfbconfig.h			\
		include/rfb/rfbint.h			\
		$(libitalc_core_static_a_MOC)		\
		./italc_core_qrc.cpp

BUILT_SOURCES = $(libitalc_core_static_a_MOC) ./italc_core_qrc.cpp shared-lib

libitalc_core_static_a_SOURCES =			\
		$(srcdir)/src/dsa_key.cpp		\
		$(srcdir)/src/fast_qimage.cpp		\
		$(srcdir)/src/inject.cpp		\
		$(srcdir)/src/isd_base.cpp		\
		$(srcdir)/src/isd_connection.cpp	\
		$(srcdir)/src/ivs_connection.cpp	\
		$(srcdir)/src/local_system.cpp		\
		$(srcdir)/src/lock_widget.cpp		\
		$(srcdir)/src/messagebox.cpp		\
		$(srcdir)/src/minilzo.c			\
		$(srcdir)/src/progress_widget.cpp	\
		$(srcdir)/src/system_key_trapper.cpp	\
		$(srcdir)/src/vncview.cpp		\
		$(srcdir)/include/debug.h		\
		$(srcdir)/include/dsa_key.h		\
		$(srcdir)/include/fast_qimage.h		\
		$(srcdir)/include/inject.h		\
		$(srcdir)/include/isd_base.h		\
		$(srcdir)/include/isd_connection.h	\
		$(srcdir)/include/italc_rfb_ext.h	\
		$(srcdir)/include/ivs_connection.h	\
		$(srcdir)/include/local_system.h	\
		$(srcdir)/include/lock_widget.h		\
		$(srcdir)/include/lzoconf.h		\
		$(srcdir)/include/lzodefs.h		\
		$(srcdir)/include/messagebox.h		\
		$(srcdir)/include/minilzo.h		\
		$(srcdir)/include/progress_widget.h	\
		$(srcdir)/include/qt_features.h		\
		$(srcdir)/include/qt_user_events.h	\
		$(srcdir)/include/types.h		\
		$(srcdir)/include/system_key_trapper.h	\
		$(srcdir)/include/vncview.h		\
		$(srcdir)/include/rfb/keysym.h		\
		$(srcdir)/include/rfb/rfb.h		\
		$(srcdir)/include/rfb/rfbclient.h	\
		$(srcdir)/include/rfb/rfbproto.h	\
		$(srcdir)/italc_core_qrc.cpp		\
		$(srcdir)/italc_core.qrc		\
		$(wildcard $(srcdir)/resources/*.qm $(srcdir)/resources/*.ts) \
		$(wildcard $(srcdir)/resources/*.png)


inject.o: $(srcdir)/src/inject.cpp
	$(CXX) -c -o $@ $< -DHAVE_CONFIG_H $(CXXFLAGS) $(QT_CXXFLAGS) $(INCLUDES) $(CPPFLAGS) -O0

if HAVE_LIBSSL
LIBSSL = -lssl -lcrypto
endif

if HAVE_LIBEAY32
LIBSSL = -leay32
endif


if BUILD_WIN32
shared-lib: italc_core.dll

italc_core.dll: libitalc_core-static.a
	$(CXX) *.o -shared -Wl,-no-undefined -Wl,-enable-runtime-pseudo-reloc -Wl,-export-all-symbols -Wl,--out-implib,libitalc_core.a -o $@ $(QT_LDADD) $(QT_LIB_GUI) $(LIBSSL) -ljpeg -lpsapi && $(STRIP) italc_core.dll

pkglib_LTLIBRARIES = italc_core.dll
italc_core_dll_SOURCES =

endif

if BUILD_LINUX
shared-lib: libitalc_core.so

libitalc_core.so: libitalc_core-static.a
	$(CXX) *.o -shared -Wl,-no-undefined -Wl,-soname,libitalc_core.so -o $@  $(QT_LDADD) $(QT_LIB_GUI) $(LIBSSL) -lz -ljpeg

pkglib_LTLIBRARIES = libitalc_core.so
libitalc_core_so_SOURCES =

endif


AM_CXXFLAGS = $(QT_CXXFLAGS) -O2 -DBUILD_ICA -DBUILD_LIBRARY $(CXXFLAGS_ADD)

INCLUDES = -I$(srcdir)/include -I$(top_srcdir)

