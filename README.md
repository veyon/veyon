# Veyon - Open Source computer monitoring and classroom management

[![Build status](https://img.shields.io/travis/veyon/veyon.svg)](https://travis-ci.org/veyon/veyon)
[![Latest stable release](https://img.shields.io/github/release/veyon/veyon.svg?maxAge=3600)](https://github.com/veyon/veyon/releases)
[![Overall downloads on Github](https://img.shields.io/github/downloads/veyon/veyon/total.svg?maxAge=3600)](https://github.com/veyon/veyon/releases)
[![Documentation Status](https://readthedocs.org/projects/veyon/badge/?version=latest)](http://veyon.readthedocs.io/en/latest/?badge=latest)
[![Localise on Transifex](https://img.shields.io/badge/localise-on_transifex-green.svg)](https://www.transifex.com/veyon-solutions/veyon/)


## What is Veyon?

Veyon is an Open Source computer monitoring and classroom management software.
It enables teachers to view and control computer labs and interact with students.
Veyon is available in different languages and provides lots of useful features:

  * see what's going on in computer labs in overview mode and take screenshots
  * remote control computers to support and help users
  * broadcast teacher's screen to students in realtime by using demo mode
    (either in fullscreen or in a window)
  * lock workstations for attracting attention to teacher
  * send text messages to students
  * powering on/off and rebooting computers remote
  * remote logoff and remote execution of arbitrary commands/scripts
  * home schooling - Veyon's network technology is not restricted to a subnet
    and therefore students at home can join lessons via VPN connections just by
    installing the Veyon service


## License

Copyright (c) 2004-2017 Tobias Doerffel / Veyon Solutions.

See the file COPYING for the GNU GENERAL PUBLIC LICENSE.


## Installation and configuration

Please refer to the official Veyon Administrator Manual at http://docs.veyon.io/en/latest/admin/index.html
for information on the installation and configuration of Veyon.


## Usage

Please refer to the official Veyon User Manual at http://docs.veyon.io/en/latest/user/index.html
for information on how to use Veyon.


## Veyon on Linux

### Downloading sources

First grab the latest sources by cloning the Git repository and fetching all submodules:

	git clone https://github.com/veyon/veyon.git
	cd veyon
	git submodule update --init


### Installing dependencies

Requirements for Debian-based distributions:

- Build tools: g++ make cmake
- Qt5: qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools
- X11: xorg-dev libxtst-dev
- libjpeg: libjpeg-turbo8-dev
- zlib: zlib1g-dev
- OpenSSL: libssl-dev
- PAM: libpam0g-dev
- LZO: liblzo2-dev 
- QCA: libqca2-dev libqca-qt5-2-dev 
- LDAP: libldap2-dev libsasl2-dev

As root you can run

	apt install g++ make cmake qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools \
	            xorg-dev libxtst-dev libjpeg-turbo8-dev zlib1g-dev libssl-dev libpam0g-dev \ 
	            liblzo2-dev libqca2-dev libqca-qt5-2-dev libldap2-dev libsasl2-dev
    

Requirements for RedHat-based distributions:

- Build tools: gcc-c++ make cmake
- Qt5: qt5-devel
- X11: libXtst-devel
- libjpeg: libjpeg-turbo-devel
- zlib: zlib-devel
- OpenSSL: openssl-devel
- PAM: pam-devel
- LZO: lzo-devel
- QCA: qca-devel qca-qt5-devel
- LDAP: openldap-devel libgsasl

As root you can run

	dnf install gcc-c++ make cmake qt5-devel libXtst-devel libjpeg-turbo-devel zlib-devel  \
             openssl-devel pam-devel lzo-devel qca-devel qca-qt5-devel openldap-devel libgsasl


### Configuring and building sources

Run the following commands:

	mkdir build
	cd build
	cmake ..
	make -j4

If some requirements are not fullfilled, CMake will inform you about it and
you will have to install the missing software before continuing.

You can now generate a package (.deb or .rpm depending what system you are in).

On Fedora, this requires an additional dependency (rpm-build) which can be installed by running

	dnf install rpm-build

For generating a package you can run
    
	make package

Then you'll get something like veyon_x.y.z_arch.deb or veyon-x.y.z.arch.rpm

Alternatively you can install the built binaries directly (not recommended for
production systems) by running the following command as root:

	make install


## Join development

If you are interested in Veyon, its programming, artwork, testing or something like that, you're welcome to participate in the development of Veyon!

Before starting the implementation of a new feature, please always open an issue at https://github.com/veyon/veyon/issues to start a discussion about your intended implementation. There may be different ideas, improvements, hints or maybe an already ongoing work on this feature.


## More Information

* http://veyon.io/
* http://docs.veyon.io/
* http://facebook.com/veyon.io/
