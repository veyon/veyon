FROM fedora:40
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-40.noarch.rpm && \
	dnf install -y --setopt=install_weak_deps=False \
		git gcc-c++ ninja-build cmake rpm-build fakeroot fakeroot-libs \
		qt6-qtbase-devel qt6-qtbase qt6-qt5compat-devel qt6-linguist qt6-qttools-devel qt6-qtdeclarative-devel qt6-qthttpserver-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libfakekey-devel \
		libvncserver-devel \
		openssl-devel \
		pam-devel \
		procps-devel \
		qca-qt6-devel qca-qt6-ossl \
		ffmpeg-devel \
		cyrus-sasl-devel \
		openldap-devel
