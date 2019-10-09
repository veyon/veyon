FROM debian:stretch
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	apt-get update && \
	apt-get install --no-install-recommends -y \
		git binutils gcc g++ make cmake rename file fakeroot bzip2 \
		qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools qtdeclarative5-dev qtquickcontrols2-5-dev \
		xorg-dev \
		libfakekey-dev \
		libjpeg-dev \
		zlib1g-dev \
		libssl-dev \
		libpam0g-dev \
		libprocps-dev \
		libldap2-dev \
		libsasl2-dev \
		libpng-dev \
		liblzo2-dev \
		libqca-qt5-2-dev libqca-qt5-2-plugins \
		&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
