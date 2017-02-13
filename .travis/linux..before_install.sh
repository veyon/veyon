#!/usr/bin/env bash

sudo add-apt-repository -y "deb http://archive.ubuntu.com/ubuntu yakkety main universe multiverse"
sudo apt-get update -qq
sudo apt-get install -y --force-yes dpkg

