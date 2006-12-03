bin_PROGRAMS = ica


if BUILD_LINUX

IVS_INCLUDES = -I$(srcdir)/x11 -I$(srcdir)/x11/x11vnc -I$(srcdir)/x11/libvncserver

IVS_SRCS =	$(srcdir)/x11/x11vnc.c				\
		$(srcdir)/x11/libvncserver/auth.c		\
		$(srcdir)/x11/libvncserver/cargs.c		\
		$(srcdir)/x11/libvncserver/corre.c		\
		$(srcdir)/x11/libvncserver/cursor.c		\
		$(srcdir)/x11/libvncserver/cutpaste.c		\
		$(srcdir)/x11/libvncserver/d3des.c		\
		$(srcdir)/x11/libvncserver/draw.c		\
		$(srcdir)/x11/libvncserver/font.c		\
		$(srcdir)/x11/libvncserver/hextile.c		\
		$(srcdir)/x11/libvncserver/httpd.c		\
		$(srcdir)/x11/libvncserver/main.c		\
		$(srcdir)/x11/libvncserver/rfbregion.c		\
		$(srcdir)/x11/libvncserver/rfbserver.c		\
		$(srcdir)/x11/libvncserver/rre.c		\
		$(srcdir)/x11/libvncserver/scale.c		\
		$(srcdir)/x11/libvncserver/selbox.c		\
		$(srcdir)/x11/libvncserver/sockets.c		\
		$(srcdir)/x11/libvncserver/stats.c		\
		$(srcdir)/x11/libvncserver/translate.c		\
		$(srcdir)/x11/libvncserver/ultra.c		\
		$(srcdir)/x11/libvncserver/vncauth.c		\
		$(srcdir)/x11/rfb/rfb.h				\
		$(srcdir)/x11/rfb/rfbregion.h			\
		$(srcdir)/x11/rfb/default8x16.h			\
		$(srcdir)/x11/libvncserver/d3des.h		\
		$(srcdir)/x11/libvncserver/private.h		\
		$(srcdir)/x11/libvncserver/scale.h		\
		$(srcdir)/x11/libvncserver/zrleoutstream.h	\
		$(srcdir)/x11/libvncserver/zrlepalettehelper.h	\
		$(srcdir)/x11/libvncserver/zrletypes.h		\
		$(srcdir)/x11/libvncserver/zlib.c		\
		$(srcdir)/x11/libvncserver/zrle.c		\
		$(srcdir)/x11/libvncserver/zrleoutstream.c	\
		$(srcdir)/x11/libvncserver/zrlepalettehelper.c	\
		$(srcdir)/x11/libvncserver/tight.c


if HAVE_LIBPTHREAD
PTHREADLDADD = -lpthread
endif

IVSLDADD = @X_LIBS@ $(PTHREADLDADD)


else


IVS_INCLUDES = -I$(srcdir)/win32/ -I$(srcdir)/win32/src

IVS_SRCS =	$(srcdir)/win32/win32vnc.cpp			\
		$(srcdir)/win32/src/d3des_w32.c			\
		$(srcdir)/win32/src/vncauth_w32.c		\
		$(srcdir)/win32/src/vncEncodeZlib.cpp		\
		$(srcdir)/win32/src/vncEncodeRRE.cpp		\
		$(srcdir)/win32/src/vncEncodeTight.cpp		\
		$(srcdir)/win32/src/vncEncodeCoRRE.cpp		\
		$(srcdir)/win32/src/vncEncodeHexT.cpp


IVSLDADD = -lkernel32 -lwsock32 -luser32 -lgdi32

endif


if HAVE_LIBSSL
LIBSSL_LDADD = -lssl
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif

if BUILD_WIN32
ica_win_resources.o: ica.rc
	$(WINDRES) -o $@ $<
WIN32_RES_LDADD = ica_win_resources.o
CXXFLAGS_ADD=-DHAVE_BOOLEAN
endif

ica_LDADD = $(QT_LDADD) $(QT_LIB_GUI) -lz -ljpeg $(LIBSSL_LDADD) $(IVSLDADD) $(WIN32_RES_LDADD)
ica_LDFLAGS = $(LDFLAGS)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -O2 -DBUILD_ICA -DNO_LUPUS_INTEGRATION $(CXXFLAGS_ADD)
AM_CFLAGS := $(AM_CFLAGS) -O2 -DBUILD_ICA -DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT=1

INCLUDES = $(IVS_INCLUDES) -I$(srcdir)/../common/include -I$(srcdir)/../common/include/rfb -I$(srcdir)/src -I$(top_srcdir)



%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<


%.ts: 
	$(LUPDATE) $(wildcard $(srcdir)/src/*.cpp $(srcdir)/../common/src/*.cpp) -ts $(srcdir)/resources/$@

%.qm: %.ts
	$(LRELEASE) $< -qm $@ 


ica_MOC =	src/isd_server.moc 			\
		src/remote_control_widget.moc		\
		src/vncview.moc				\
		src/system_key_trapper.moc		\
		src/demo_server.moc			\
		src/progress_widget.moc			\
		src/demo_client.moc			\
		../common/include/isd_connection.moc	\
		../common/include/ivs_connection.moc


./ica_qrc.cpp: ica.qrc
	rcc $< > $@


ica_SOURCES =		$(srcdir)/src/ica_main.cpp			\
			$(srcdir)/src/system_service.cpp		\
			$(srcdir)/src/ivs.cpp				\
			$(srcdir)/src/isd_server.cpp			\
			$(srcdir)/src/lock_widget.cpp			\
			$(srcdir)/src/progress_widget.cpp		\
			$(srcdir)/src/remote_control_widget.cpp		\
			$(srcdir)/src/vncview.cpp			\
			$(srcdir)/src/demo_server.cpp			\
			$(srcdir)/src/demo_client.cpp			\
			$(srcdir)/src/system_key_trapper.cpp		\
			$(srcdir)/src/inject.cpp			\
			$(srcdir)/src/minilzo.c				\
			$(srcdir)/../common/src/messagebox.cpp		\
			$(srcdir)/../common/src/fast_qimage.cpp		\
			$(srcdir)/../common/src/isd_base.cpp		\
			$(srcdir)/../common/src/isd_connection.cpp	\
			$(srcdir)/../common/src/ivs_connection.cpp	\
			$(srcdir)/../common/src/local_system.cpp	\
			$(srcdir)/../common/src/rect_list.cpp		\
			$(srcdir)/../common/src/dsa_key.cpp		\
			$(srcdir)/src/inject.h				\
			$(srcdir)/src/ivs.h				\
			$(srcdir)/src/isd_server.h			\
			$(srcdir)/src/progress_widget.h			\
			$(srcdir)/src/lock_widget.h			\
			$(srcdir)/src/remote_control_widget.h		\
			$(srcdir)/src/vncview.h				\
			$(srcdir)/src/demo_server.h			\
			$(srcdir)/src/demo_client.h			\
			$(srcdir)/src/minilzo.h				\
			$(srcdir)/src/lzoconf.h				\
			$(srcdir)/src/ica_main.h			\
			$(srcdir)/src/system_key_trapper.h		\
			$(srcdir)/src/system_service.h			\
			$(srcdir)/ica.qrc				\
			$(srcdir)/ica_qrc.cpp				\
			$(wildcard $(srcdir)/resources/*)		\
			$(IVS_SRCS)


inject.o: $(srcdir)/src/inject.cpp
	$(CXX) -c -o $@ $< -DHAVE_CONFIG_H $(CXXFLAGS) $(QT_CXXFLAGS) $(INCLUDES) -O0

CLEANFILES = ./ica_qrc.cpp $(ica_MOC)
BUILT_SOURCES = ./ica_qrc.cpp $(ica_MOC)




EXTRA_DIST =	$(srcdir)/x11/libvncserver/tableinit24.c		\
		$(srcdir)/x11/libvncserver/tableinittctemplate.c	\
		$(srcdir)/x11/libvncserver/tabletranstemplate.c		\
		$(srcdir)/x11/libvncserver/tableinitcmtemplate.c	\
		$(srcdir)/x11/libvncserver/tabletrans24template.c	\
		$(srcdir)/x11/libvncserver/zrleencodetemplate.c		\
		$(srcdir)/x11/libvncserver/zlib.c			\
		$(srcdir)/x11/libvncserver/zrle.c			\
		$(srcdir)/x11/libvncserver/zrleoutstream.c		\
		$(srcdir)/x11/libvncserver/zrlepalettehelper.c		\
		$(srcdir)/x11/libvncserver/tight.c			\
		$(wildcard $(srcdir)/x11/x11vnc/*)			\
		$(srcdir)/win32/src/tableinitcmtemplate.cpp		\
		$(srcdir)/win32/src/tabletranstemplate.cpp		\
		$(srcdir)/win32/src/tableinittctemplate.cpp		\
		$(srcdir)/win32/src/vncKeymap.cpp			\
		$(srcdir)/win32/src/VSocket.cpp				\
		$(srcdir)/win32/src/WinVNC.cpp				\
		$(srcdir)/win32/src/VideoDriver.cpp			\
		$(srcdir)/win32/src/vncService.cpp			\
		$(srcdir)/win32/src/vncInstHandler.cpp			\
		$(srcdir)/win32/src/vncServer.cpp			\
		$(srcdir)/win32/src/vncClient.cpp			\
		$(srcdir)/win32/src/stdhdrs.cpp				\
		$(srcdir)/win32/src/RectList.cpp			\
		$(srcdir)/win32/src/translate.cpp			\
		$(srcdir)/win32/src/Log.cpp				\
		$(srcdir)/win32/src/vncEncoder.cpp			\
		$(srcdir)/win32/src/vncBuffer.cpp			\
		$(srcdir)/win32/src/vncRegion.cpp			\
		$(srcdir)/win32/src/vncSockConnect.cpp			\
		$(srcdir)/win32/src/vncDesktop.cpp			\
		$(srcdir)/win32/src/VNCHooks.cpp			\
		$(srcdir)/win32/src/SharedData.cpp			\
		$(srcdir)/win32/src/MinMax.cpp				\
		$(srcdir)/win32/src/nt.cpp				\
		$(srcdir)/win32/src/VSocket.h				\
		$(srcdir)/win32/src/stdhdrs.h				\
		$(srcdir)/win32/src/vncBuffer.h				\
		$(srcdir)/win32/src/Log.h				\
		$(srcdir)/win32/src/resource.h				\
		$(srcdir)/win32/src/rfb.h				\
		$(srcdir)/win32/src/vncServer.h				\
		$(srcdir)/win32/src/vncDesktop.h			\
		$(srcdir)/win32/src/vncEncodeTight.h			\
		$(srcdir)/win32/src/vncEncodeHexT.h			\
		$(srcdir)/win32/src/WinVNC.h				\
		$(srcdir)/win32/src/VideoDriver.h			\
		$(srcdir)/win32/src/vncKeymap.h				\
		$(srcdir)/win32/src/vncClient.h				\
		$(srcdir)/win32/src/vncRegion.h				\
		$(srcdir)/win32/src/vncInstHandler.h			\
		$(srcdir)/win32/src/RectList.h				\
		$(srcdir)/win32/src/vncEncodeZlib.h			\
		$(srcdir)/win32/src/vncSockConnect.h			\
		$(srcdir)/win32/src/vncEncodeRRE.h			\
		$(srcdir)/win32/src/vncService.h			\
		$(srcdir)/win32/src/translate.h				\
		$(srcdir)/win32/src/vncEncoder.h			\
		$(srcdir)/win32/src/vncEncodeCoRRE.h			\
		$(srcdir)/win32/src/VTypes.h				\
		$(srcdir)/win32/src/SharedData.h			\
		$(srcdir)/win32/src/VNCHooks.h				\
		$(srcdir)/win32/src/omnithread.h			\
		$(srcdir)/win32/src/MinMax.h				\
		$(srcdir)/win32/src/nt.h				\
		$(srcdir)/win32/src/d3des.h				\
		$(srcdir)/win32/src/vncPasswd.h				\
		$(srcdir)/win32/src/vncauth.h				\
		$(srcdir)/win32/zlib/zlib.h				\
		$(srcdir)/win32/libjpeg/jpeglib.h			\
		$(srcdir)/win32/crtdbg.h				\
		$(srcdir)/win32/FileTransferItemInfo.h			\
		$(srcdir)/win32/keysymdef.h				\
		$(srcdir)/win32/rectlist.h				\
		$(srcdir)/win32/vncAcceptDialog.h			\
		$(srcdir)/win32/vncCorbaConnect.h			\
		$(srcdir)/win32/vncCORBAConnect.h			\
		$(srcdir)/win32/vncEncodeZlibHex.h			\
		$(srcdir)/win32/VNCHelp.h				\
		$(srcdir)/win32/vncHTTPConnect.h			\
		$(srcdir)/win32/vncMenu.h				\
		$(srcdir)/win32/vncPasswd.h				\
		$(srcdir)/win32/WallpaperUtils.h			\
		$(srcdir)/win32/Windows.h

