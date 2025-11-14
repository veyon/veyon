FROM gcr.io/oss-fuzz-base/base-builder
RUN apt-get update && apt-get install -y ca-certificates git ninja-build cmake \
                qtbase5-dev qtbase5-private-dev qtbase5-dev-tools qtdeclarative5-dev qtquickcontrols2-5-dev \
                libkf5itemmodels-dev \
                xorg-dev \
                libfakekey-dev \
                libpng-dev libjpeg-dev zlib1g-dev liblzo2-dev \
                libvncserver-dev \
                libssl-dev \
                libpam0g-dev \
                libprocps-dev \
                libldap2-dev \
                libsasl2-dev \
                libqca-qt5-2-dev libqca-qt5-2-plugins
RUN git clone --recurse-submodules https://github.com/veyon/veyon.git veyon
WORKDIR veyon
COPY build.sh $SRC/   
