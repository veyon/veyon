# Veyon - Virtual Eye On Networks

[![.github/workflows/build.yml](https://github.com/veyon/veyon/actions/workflows/build.yml/badge.svg)](https://github.com/veyon/veyon/actions/workflows/build.yml)
[![Latest stable release](https://img.shields.io/github/release/veyon/veyon.svg?maxAge=3600)](https://github.com/veyon/veyon/releases)
[![Overall downloads on Github](https://img.shields.io/github/downloads/veyon/veyon/total.svg?maxAge=3600)](https://github.com/veyon/veyon/releases)
[![Documentation Status](https://readthedocs.org/projects/veyon/badge/?version=latest)](https://docs.veyon.io/)
[![Localise on Transifex](https://img.shields.io/badge/localise-on_transifex-green.svg)](https://www.transifex.com/veyon-solutions/veyon/)
[![license](https://img.shields.io/badge/license-GPLv2-green.svg)](LICENSE)


## What is Veyon?

Veyon is a free and open source software for monitoring and controlling
computers across multiple platforms. Veyon supports you in teaching in digital
learning environments, performing virtual trainings or giving remote support.

The following features are available in Veyon:

  * Overview: monitor all computers in one or multiple locations or classrooms
  * Remote access: view or control computers to watch and support users
  * Demo: broadcast the teacher's screen in realtime (fullscreen/window)
  * Screen lock: draw attention to what matters right now
  * Communication: send text messages to students
  * Start and end lessons: log in and log out users all at once
  * Screenshots: record learning progress and document infringements
  * Programs & websites: launch programs and open website URLs remotely
  * Teaching material: distribute and open documents, images and videos easily
  * Administration: power on/off and reboot computers remotely


## License

Copyright (c) 2004-2022 Tobias Junghans / Veyon Solutions.

See the file COPYING for the GNU GENERAL PUBLIC LICENSE.


## Installation and configuration

Please refer to the official Veyon Administrator Manual at https://docs.veyon.io/en/latest/admin/index.html
for information on the installation and configuration of Veyon.


## Usage

Please refer to the official Veyon User Manual at https://docs.veyon.io/en/latest/user/index.html
for information on how to use Veyon.


## Veyon on Linux

### Downloading sources

First grab the latest sources by cloning the Git repository and fetching all submodules:

	git clone --recursive https://github.com/veyon/veyon.git && cd veyon


### Installing dependencies

Requirements for Debian-based distributions:

- Build tools: g++ libc6-dev make cmake dpkg-dev
- Qt5: qtbase5-dev qtbase5-private-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools qtdeclarative5-dev qtquickcontrols2-5-dev
- X11: xorg-dev libxtst-dev libfakekey-dev
- libjpeg: libjpeg-dev provided by libjpeg-turbo8-dev or libjpeg62-turbo-dev
- zlib: zlib1g-dev
- OpenSSL: libssl-dev
- PAM: libpam0g-dev
- procps: libprocps-dev
- LZO: liblzo2-dev
- QCA: libqca-qt5-2-dev
- LDAP: libldap2-dev
- SASL: libsasl2-dev

As root you can run

	apt install g++ libc6-dev make cmake qtbase5-dev qtbase5-private-dev \
	            qtbase5-dev-tools qttools5-dev qttools5-dev-tools \
	            qtdeclarative5-dev qtquickcontrols2-5-dev libfakekey-dev \
	            xorg-dev libxtst-dev libjpeg-dev zlib1g-dev libssl-dev libpam0g-dev \
	            libprocps-dev liblzo2-dev libqca-qt5-2-dev libldap2-dev \
	            libsasl2-dev ninja-build



Requirements for RedHat-based distributions:

- Build tools: gcc-c++ make cmake rpm-build
- Qt5: qt5-devel qt5-qtbase-private-devel
- X11: libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel libfakekey-devel
- libjpeg: libjpeg-turbo-devel
- zlib: zlib-devel
- OpenSSL: openssl-devel
- PAM: pam-devel
- procps: procps-devel
- LZO: lzo-devel
- QCA: qca-devel qca-qt5-devel
- LDAP: openldap-devel
- SASL: cyrus-sasl-devel

As root you can run

	dnf install gcc-c++ make cmake rpm-build qt5-devel libXtst-devel libXrandr-devel libXinerama-devel libXcursor-devel \
             libXrandr-devel libXdamage-devel libXcomposite-devel libXfixes-devel libfakekey-devel libjpeg-turbo-devel zlib-devel \
             openssl-devel pam-devel procps-devel lzo-devel qca-devel qca-qt5-devel openldap-devel cyrus-sasl-devel ninja-build


### Configuring and building sources

Run the following commands:

	mkdir build
	cd build
	cmake ..
	make -j4

NOTE: If you want to build a .deb or .rpm package for this software, instead of the provided cmake command, you should use:

	cmake -DCMAKE_INSTALL_PREFIX=/usr ..

to install package files in /usr instead of /usr/local.

If some requirements are not fullfilled, CMake will inform you about it and
you will have to install the missing software before continuing.

You can now generate a package (.deb or .rpm depending what system you are in).

For generating a package you can run

	fakeroot make package

Then you'll get something like veyon_x.y.z_arch.deb or veyon-x.y.z.arch.rpm

Alternatively you can install the built binaries directly (not recommended for
production systems) by running the following command as root:

	make install

### Arch linux

A PKGBUILD can be found in the [AUR](https://aur.archlinux.org/packages/veyon/).

### PPA

This PPA contains official Veyon packages for Ubuntu suitable for use both on desktop computers and ARM boards (e.g. Raspberry Pi). Even though only packages for LTS releases are available they should work for subsequent non-LTS releases as well.

	sudo add-apt-repository ppa:veyon/stable
	sudo apt-get update
	
### openSUSE

Veyon is available on official openSUSE Tumbleweed repository.

	sudo zypper in veyon
	
For openSUSE Leap 15.2, use the unofficial package from Education repository.
	 
	 https://software.opensuse.org/package/veyon?search_term=veyon

## Join development

If you are interested in Veyon, its programming, artwork, testing or something like that, you're welcome to participate in the development of Veyon!

Before starting the implementation of a new feature, please always open an issue at https://github.com/veyon/veyon/issues to start a discussion about your intended implementation. There may be different ideas, improvements, hints or maybe an already ongoing work on this feature.


## More information

* https://veyon.io/
* https://docs.veyon.io/
* https://facebook.com/veyon.io/
* https://twitter.com/veyon_io
