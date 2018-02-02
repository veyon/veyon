FROM opensuse:42.3
MAINTAINER Tobias Junghans <tobydox@users.sf.net>

RUN \
	zypper addrepo -f http://download.opensuse.org/repositories/devel:tools/openSUSE_Leap_42.3/devel:tools.repo && \
	zypper --gpg-auto-import-keys install -y git gcc6-c++ make cmake rpm-build fakeroot \
		libqt5-qtbase-devel libqt5-linguist-devel libqt5-qttools-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libjpeg8-devel \
		zlib-devel \
		libpng16-devel libpng16-compat-devel \
		libopenssl-devel \
		procps-devel \
		pam-devel lzo-devel \
		libqca-qt5-devel libqca-qt5-plugins \
		openldap2-devel cyrus-sasl-devel

