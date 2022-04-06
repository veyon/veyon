#!/usr/bin/env bash

set -e

cd $2

# add distribution name to file name
rename "s/_amd64/-${3}_amd64/g" *.deb

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
