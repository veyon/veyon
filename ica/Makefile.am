AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = ica


if BUILD_LINUX

IVS_INCLUDES = -I$(srcdir)/x11 -I$(srcdir)/x11/x11vnc -I$(srcdir)/x11/libvncserver

IVS_SRCS =	$(srcdir)/x11/x11vnc.c				\
		$(srcdir)/x11/x11vnc/appshare.c			\
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
		$(srcdir)/x11/libvncserver/tight.c		\
		$(srcdir)/x11/rfb/rfbregion.h			\
		$(srcdir)/x11/rfb/default8x16.h


if HAVE_LIBPTHREAD
PTHREADLDADD = -lpthread
endif

IVSLDADD = @X_LIBS@ $(PTHREADLDADD)


else


IVS_INCLUDES = -I$(srcdir)/win32 -I$(srcdir)/win32/src -I$(srcdir)/win32/src/omnithread -I$(srcdir)/win32/src/VNCHooks

IVS_SRCS =	$(srcdir)/win32/win32vnc.cpp			\
		$(srcdir)/win32/src/d3des_w32.c			\
		$(srcdir)/win32/src/vncauth_w32.c		\
		$(srcdir)/win32/src/vncEncodeZlib.cpp		\
		$(srcdir)/win32/src/vncEncodeRRE.cpp		\
		$(srcdir)/win32/src/vncEncodeTight.cpp		\
		$(srcdir)/win32/src/vncEncodeCoRRE.cpp		\
		$(srcdir)/win32/src/vncEncodeHexT.cpp


IVSLDADD = -lkernel32 -lwsock32 -luser32 -lgdi32 -lpsapi -lnetapi32

VNCHooks.o: $(srcdir)/win32/src/VNCHooks/VNCHooks.cpp
	$(CXX) $(INCLUDES) $(AM_CXXFLAGS) $< -c -o $@

SharedData.o: $(srcdir)/win32/src/VNCHooks/SharedData.cpp
	$(CXX) $(INCLUDES) $(AM_CXXFLAGS) $< -c -o $@

vnchooks.dll: VNCHooks.o SharedData.o
	$(CXX) VNCHooks.o SharedData.o -shared -Wl,-no-undefined -Wl,-enable-runtime-pseudo-reloc -Wl,-enable-auto-image-base -Wl,--out-implib,libvnchooks.a -lgdi32 -o $@ && $(STRIP) $@

endif


if HAVE_LIBSSL
LIBSSL_LDADD = -lssl -lcrypto
endif

if HAVE_LIBEAY32
LIBSSL_LDADD = -leay32
endif

if BUILD_WIN32

ica_win_resources.o: ica.rc vnchooks.dll
	$(WINDRES) -o $@ $<

WIN32_RES_LDADD = ica_win_resources.o -L. -lvnchooks
#CXXFLAGS_ADD=-DHAVE_BOOLEAN
endif

ica_LDADD = $(QT_LDADD) $(QT_LIB_GUI) -lz -ljpeg $(LIBSSL_LDADD) $(IVSLDADD) $(WIN32_RES_LDADD) -L../lib -litalc_core
ica_LDFLAGS = $(LDFLAGS) -mwindows -rpath $(pkglibdir)

AM_CXXFLAGS = $(QT_CXXFLAGS) -O2 -DBUILD_ICA $(CXXFLAGS_ADD)
AM_CFLAGS = -O2 -DBUILD_ICA -DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT=1 -w

INCLUDES = $(IVS_INCLUDES) -I$(srcdir)/../lib/include -I$(srcdir)/../lib/include/rfb -I$(srcdir)/src -I$(top_srcdir)



%.moc: $(srcdir)/%.h
	$(MOC) -o $@ $<


%.ts: 
	$(LUPDATE) $(wildcard $(srcdir)/src/*.cpp) -ts $(srcdir)/resources/$@

%.qm: %.ts
	$(LRELEASE) $(srcdir)/resources/$< -qm $(srcdir)/resources/$@


ica_MOC =	src/isd_server.moc 			\
		src/demo_server.moc			\
		src/demo_client.moc


./ica_qrc.cpp: ica.qrc
	$(RCC) $< > $@


ica_SOURCES =		$(srcdir)/src/ica_main.cpp		\
			$(srcdir)/src/ivs.cpp			\
			$(srcdir)/src/isd_server.cpp		\
			$(srcdir)/src/local_system_ica.cpp	\
			$(srcdir)/src/system_service.cpp	\
			$(srcdir)/src/demo_server.cpp		\
			$(srcdir)/src/demo_client.cpp		\
			$(srcdir)/src/ivs.h			\
			$(srcdir)/src/isd_server.h		\
			$(srcdir)/src/local_system_ica.h	\
			$(srcdir)/src/demo_server.h		\
			$(srcdir)/src/demo_client.h		\
			$(srcdir)/src/system_service.h		\
			$(srcdir)/src/ica_main.h		\
			$(srcdir)/ica.qrc			\
			$(srcdir)/ica_qrc.cpp			\
			$(wildcard $(srcdir)/resources/*)	\
			$(IVS_SRCS)


CLEANFILES = ./ica_qrc.cpp $(ica_MOC) vnchooks.dll libvnchooks.a
BUILT_SOURCES = ./ica_qrc.cpp $(ica_MOC)


man_MANS=ica.1


EXTRA_DIST =	$(man_MANS)						\
		$(srcdir)/x11/README					\
		$(srcdir)/win32/README					\
		$(srcdir)/x11/libvncserver/tableinit24.c		\
		$(srcdir)/x11/libvncserver/tableinittctemplate.c	\
		$(srcdir)/x11/libvncserver/tabletranstemplate.c		\
		$(srcdir)/x11/libvncserver/tableinitcmtemplate.c	\
		$(srcdir)/x11/libvncserver/tabletrans24template.c	\
		$(srcdir)/x11/libvncserver/zrleencodetemplate.c		\
		$(srcdir)/x11/libvncserver/zywrletemplate.c		\
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
		$(srcdir)/win32/src/VNCHooks/VNCHooks.cpp		\
		$(srcdir)/win32/src/VNCHooks/SharedData.cpp		\
		$(srcdir)/win32/src/MinMax.cpp				\
		$(srcdir)/win32/src/DynamicFn.cpp			\
		$(srcdir)/win32/src/TsSessions.cpp			\
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
		$(srcdir)/win32/src/DynamicFn.h				\
		$(srcdir)/win32/src/TsSessions.h			\
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
		$(srcdir)/win32/src/vncTimedMsgBox.h			\
		$(srcdir)/win32/src/VTypes.h				\
		$(srcdir)/win32/src/VNCHooks/SharedData.h		\
		$(srcdir)/win32/src/VNCHooks/VNCHooks.h			\
		$(srcdir)/win32/src/MinMax.h				\
		$(srcdir)/win32/src/d3des.h				\
		$(srcdir)/win32/src/vncPasswd.h				\
		$(srcdir)/win32/src/vncauth.h				\
		$(srcdir)/win32/src/ParseHost.c				\
		$(srcdir)/win32/src/ParseHost.h				\
		$(srcdir)/win32/src/omnithread/nt.cpp			\
		$(srcdir)/win32/src/omnithread/nt.h			\
		$(srcdir)/win32/src/omnithread/omnithread.h		\
		$(srcdir)/win32/zlib/zlib.h				\
		$(srcdir)/win32/libjpeg/jpeglib.h			\
		$(srcdir)/win32/crtdbg.h				\
		$(srcdir)/win32/FileTransferItemInfo.h			\
		$(srcdir)/win32/keysymdef.h				\
		$(srcdir)/win32/rectlist.h				\
		$(srcdir)/win32/vncAcceptDialog.h			\
		$(srcdir)/win32/vncCorbaConnect.h			\
		$(srcdir)/win32/vncEncodeZlibHex.h			\
		$(srcdir)/win32/VNCHelp.h				\
		$(srcdir)/win32/vncHTTPConnect.h			\
		$(srcdir)/win32/vncMenu.h				\
		$(srcdir)/win32/vncPasswd.h				\
		$(srcdir)/win32/WallpaperUtils.h


