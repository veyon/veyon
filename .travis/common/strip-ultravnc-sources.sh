#!/bin/sh

cd 3rdparty/ultravnc
find -name "*.vcproj" -o -name "*.vcxproj" -o -name "*.vdproj" -o -name "*.sln" | xargs rm
rm -rf avilog translations old vncviewer zipunzip_src libjpeg-turbo-win winvnc/winvnc/res setcad setpasswd lzo* xz* zlib* JavaViewer repeater uvnc* *.iss *.zip
cd addon/ms-logon                                                             
rm -rf authadm ldap* logging MSLogonACL testauth workgrpdomnt4   
