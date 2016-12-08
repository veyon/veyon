#!/usr/bin/env bash

sudo add-apt-repository -y "deb http://archive.ubuntu.com/ubuntu xenial main universe multiverse"
sudo add-apt-repository -y "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu xenial main"
sudo apt-get update -qq
sudo apt-get install -y --force-yes dpkg

