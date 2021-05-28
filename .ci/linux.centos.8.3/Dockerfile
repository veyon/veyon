FROM centos:8.3.2011
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	yum --enablerepo=extras install -y epel-release dnf-plugins-core && \
	yum config-manager --set-enabled powertools && \
	dnf install -y https://pkgs.dyn.su/el8/base/x86_64/raven-release-1.0-1.el8.noarch.rpm && \
	yum install -y git gcc-c++ make ninja-build cmake rpm-build fakeroot \
		qt5-qtbase-devel qt5-qtbase-private-devel qt5-qtbase qt5-linguist qt5-qttools qt5-qtquickcontrols2-devel qt5-qtwebengine-devel \
		kf5-kitemmodels-devel \
		libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel \
		libfakekey-devel \
		libjpeg-turbo-devel \
		zlib-devel \
		libpng-devel \
		openssl-devel \
		pam-devel \
		procps-ng-devel \
		lzo-devel \
		qca-qt5-devel qca-qt5-ossl \
		cyrus-sasl-devel \
		openldap-devel
