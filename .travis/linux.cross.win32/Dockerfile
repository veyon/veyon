FROM ubuntu:xenial
MAINTAINER Tobias Doerffel <tobias.doerffel@gmail.com>

RUN \
	apt-get update && \
	apt-get install -y software-properties-common && \
	add-apt-repository -y "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu xenial main" && \
	apt-get update && \
	apt-get install --no-install-recommends -y --force-yes \
		git \
		cmake \
		make \
		nsis \
		tofrodos \
		unzip \
		g++-mingw-w64 \
		qt5base-mingw-w64 qt5tools-mingw-w64 qt5translations-mingw-w64 \
		openssl-mingw-w64 \
		libz-mingw-w64-dev \
		libpng-mingw-w64 \
		libjpeg-turbo-mingw-w64 \
		interception-mingw-w64 \
		openldap-mingw-w64 \
		lzo2-mingw-w64 \
		qca-mingw-w64 \
	&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
