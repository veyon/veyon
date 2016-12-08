#!/usr/bin/env bash

PACKAGES="binutils gcc g++ cmake qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools xorg-dev libxtst-dev libjpeg-dev zlib1g-dev libssl-dev libpam0g-dev libldap2-dev"

sudo apt-get install -y --force-yes $PACKAGES
