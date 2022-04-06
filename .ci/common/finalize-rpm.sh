#!/usr/bin/env bash

set -e

cd $2

# add distribution name to file name
rename ".x86_64" ".${3}.x86_64" *.rpm

# show files
for i in *.rpm ; do
	echo Contents of $i:
	rpm -qlp $i
done

# show dependencies
for i in *.rpm ; do
	echo Package information for $i:
	rpm -qpR $i
done

# move to Docker volume
mv -v *.rpm $1
