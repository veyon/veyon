FROM centos:7.3.1611
MAINTAINER Tobias Doerffel <tobias.doerffel@gmail.com>

RUN \
	yum --enablerepo=extras install -y epel-release && \
	yum install -y centos-release-scl && \
	yum install -y git devtoolset-6 make cmake rpm-build \
		qt5-qtbase-devel qt5-qtbase qt5-linguist qt5-qttools \
		libXtst-devel libXrandr-devel libXinerama-devel \
		libjpeg-turbo-devel \
		zlib-devel \
		libpng-devel \
		openssl-devel \
		pam-devel \
		lzo-devel \
		qca-qt5-devel qca-qt5-ossl \
		openldap-devel
