FROM ubuntu:bionic
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	apt-get update && \
	apt-get install --no-install-recommends -y --force-yes \
		git binutils gcc g++ make cmake rename file fakeroot \
		qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools \
		xorg-dev libxtst-dev \
		libjpeg-dev \
		zlib1g-dev \
		libssl-dev \
		libpam0g-dev \
		libprocps-dev \
		libldap2-dev \
		liblzo2-dev \
		libqca-qt5-2-dev libqca-qt5-2-plugins \
		&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
