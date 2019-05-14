FROM debian:stretch
MAINTAINER Tobias Junghans <tobydox@veyon.io>

RUN \
	apt-get update && \
	apt-get install -y \
		flawfinder \
		&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
