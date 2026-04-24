{ lib
, stdenv
, src ? lib.cleanSource ./..
, cmake
, ninja
, pkg-config
, qt5
, libsForQt5
, openssl
, pam
, libvncserver
, libpng
, libjpeg
, zlib
, lzo
, xorg
, libfakekey
, procps
, openldap
, cyrus_sasl
, ffmpeg
}:

stdenv.mkDerivation {
  pname = "veyon";
  version = "4.10.2";

  inherit src;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    qt5.wrapQtAppsHook
    qt5.qttools
  ];

  buildInputs = [
    qt5.qtbase
    qt5.qtdeclarative
    libsForQt5.qca
    openssl
    pam
    libvncserver
    libpng
    libjpeg
    zlib
    lzo
    xorg.libX11
    xorg.libXtst
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXdamage
    xorg.libXcomposite
    xorg.libXfixes
    xorg.libXext
    libfakekey
    procps
    openldap
    cyrus_sasl
    ffmpeg
  ];

  cmakeFlags = [
    "-DWITH_QT6=OFF"
    "-DWITH_TRANSLATIONS=OFF"
  ];

  meta = with lib; {
    description = "Cross-platform computer control and classroom management";
    longDescription = ''
      Veyon is a free and open source software for monitoring and controlling
      computers across multiple platforms. Veyon supports you in teaching in
      digital learning environments, performing virtual trainings or giving
      remote support.
    '';
    homepage = "https://veyon.io";
    license = licenses.gpl2Only;
    platforms = [ "x86_64-linux" "aarch64-linux" ];
    maintainers = [ ];
  };
}
