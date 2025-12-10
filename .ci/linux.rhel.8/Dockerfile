FROM rockylinux:8
LABEL maintainer="Tobias Junghans <tobydox@veyon.io>"

RUN \
	dnf -y --enablerepo=extras install epel-release && \
	dnf -y install https://download1.rpmfusion.org/free/el/rpmfusion-free-release-8.noarch.rpm && \
	dnf -y --enablerepo=devel install git ninja-build cmake rpm-build fakeroot \
		qt5-qtbase-devel qt5-qtbase qt5-linguist qt5-qttools qt5-qtquickcontrols2-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libjpeg-turbo-devel \
		zlib-devel \
		libpng-devel \
		openssl-devel \
		pam-devel \
		procps-devel \
		lzo-devel \
		qca-qt5-devel qca-qt5-ossl \
		ffmpeg-devel \
		cyrus-sasl-devel \
		openldap-devel
