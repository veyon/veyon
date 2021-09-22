#!/bin/sh

(cd 3rdparty/kldap && \
rm -rf \
	autotests \
	kioslave \
	src/widgets)
(cd 3rdparty/kldap-qt-compat && \
rm -rf \
	autotests \
	kioslave \
	src/widgets)
