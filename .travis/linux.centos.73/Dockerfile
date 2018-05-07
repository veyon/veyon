FROM centos:7.3.1611
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	yum --enablerepo=extras install -y epel-release && \
	yum install -y centos-release-scl && \
	yum install -y git devtoolset-6 make cmake3 rpm-build fakeroot \
		qt5-qtbase-devel qt5-qtbase qt5-linguist qt5-qttools \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libjpeg-turbo-devel \
		zlib-devel \
		libpng-devel \
		openssl-devel \
		pam-devel \
		procps-devel \
		lzo-devel \
		qca-qt5-devel qca-qt5-ossl \
		openldap-devel && \
	ln -s /usr/bin/cmake3 /usr/bin/cmake
