#!/usr/bin/env bash

set -e

cd $2

# show content
for i in *.deb ; do
	echo Contents of $i:
	dpkg -c $i
done

# show package information and dependencies
for i in *.deb ; do
	echo Package information for $i:
	dpkg -I $i
done

# move to Docker volume
mv -v *.deb $1
