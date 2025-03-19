FROM rockylinux:9
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	dnf -y --enablerepo=extras install epel-release && \
	dnf -y install https://download1.rpmfusion.org/free/el/rpmfusion-free-release-9.noarch.rpm && \
	dnf -y --enablerepo=devel install \
		git ninja-build cmake rpm-build fakeroot \
		qt6-qtbase-devel qt6-qtbase qt6-qt5compat-devel qt6-linguist qt6-qttools-devel qt6-qtdeclarative-devel qt6-qthttpserver-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libfakekey-devel \
		libjpeg-turbo-devel zlib-devel libpng-devel lzo-devel \
		libvncserver-devel \
		openssl-devel \
		pam-devel \
		procps-devel \
		lzo-devel \
		qca-qt6-devel qca-qt6-ossl \
		ffmpeg-devel \
		cyrus-sasl-devel \
		openldap-devel
