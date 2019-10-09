FROM opensuse/leap:15.1
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	zypper --gpg-auto-import-keys install -y git gcc-c++ make cmake rpm-build fakeroot \
		libqt5-qtbase-devel libqt5-linguist-devel libqt5-qttools-devel libqt5-qtquickcontrols2 libQt5QuickControls2-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libfakekey-devel \
		libjpeg8-devel \
		zlib-devel \
		libpng16-devel libpng16-compat-devel \
		libopenssl-devel \
		procps-devel \
		pam-devel lzo-devel \
		libqca-qt5-devel libqca-qt5-plugins \
		cyrus-sasl-devel \
		openldap2-devel

