bin_PROGRAMS = userinfo
lib_LTLIBRARIES = libtest.la
libtest_la_SOURCES = ginastub.cpp
libtest_la_LDFLAGS = -shared -no-undefined -version-info 0:0:0

userinfo_SOURCES = $(srcdir)/userinfo.cpp
userinfo_LDADD = -lnetapi32 -lpsapi

EXTRA_DIST = wake.exe

