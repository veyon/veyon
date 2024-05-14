FROM ubuntu:noble
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	apt-get update && \
	apt-get install --no-install-recommends -y \
		dpkg-dev \
		ca-certificates git binutils gcc g++ ninja-build cmake file fakeroot bzip2 \
		qt6-base-dev qt6-5compat-dev qt6-l10n-tools qt6-declarative-dev qt6-httpserver-dev qt6-webengine-dev \
		xorg-dev \
		libfakekey-dev \
		libvncserver-dev \
		libssl-dev \
		libpam0g-dev \
		libproc2-dev \
		libldap2-dev \
		libsasl2-dev \
		libqca-qt6-dev libqca-qt6-plugins \
		libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
		&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
