{
  description = "Veyon - Virtual Eye On Networks - computer monitoring and control software";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      overlay = final: prev: rec {
        veyon-unwrapped = makeVeyon { };
        veyon-unwrapped-qt5 = makeVeyon { qtVersion = "qt5"; };
        veyon-unwrapped-debug = makeVeyon { buildType = "Debug"; };

        makeVeyon = {
          qtVersion ? "qt6",
          withTests ? false,
          withTranslations ? true,
          withLTO ? false,
          buildType ? "Release",
          extraCMakeFlags ? [],
        }:
          final.stdenv.mkDerivation {
            pname = "veyon";
            version = "4.10.2";

            src = final.lib.cleanSource self;

            nativeBuildInputs = with final; [
              cmake
              ninja
              pkg-config
              gettext
              final.qt6.wrapQtAppsHook
            ];

            buildInputs = with final; [
              qt6.qtbase
              qt6.qttools
              qt6.qttranslations
              libdbusmenu-qt5
              openssl
              libvncserver
              libx11
              libxext
              libxi
              libxinerama
              libxrandr
              libxcursor
              libxdamage
              libxcomposite
              libxfixes
              libxtst
              libxxf86vm
              libxkbcommon
              libxcb
              pam
              systemd
              libglvnd
              libnl
              libpulseaudio
              libxslt
            ];

            cmakeFlags = [
              "-DWITH_QT6=${if qtVersion == "qt6" then "ON" else "OFF"}"
              "-DWITH_TRANSLATIONS=${if withTranslations then "ON" else "OFF"}"
              "-DWITH_TESTS=${if withTests then "ON" else "OFF"}"
              "-DWITH_LTO=${if withLTO then "ON" else "OFF"}"
              "-DCMAKE_INSTALL_PREFIX=$out"
              "-DCMAKE_BUILD_TYPE=${buildType}"
            ] ++ extraCMakeFlags;

            installTargets = "install";

            meta = with final.lib; {
              description = "Veyon - Virtual Eye On Networks - computer monitoring and control software";
              longDescription = ''
                Veyon is free and open source software for monitoring and controlling
                computers across multiple platforms. It is primarily used in educational
                settings for monitoring classrooms, remote access and control,
                screen broadcasting, screen locking, text messaging, and more.
              '';
              homepage = "https://veyon.io/";
              license = licenses.gpl2;
              platforms = platforms.linux;
            };
          };

        veyon = final.buildEnv {
          name = "veyon";
          paths = [ veyon-unwrapped ];
        };

        linux-modules = import ./modules.nix { pkgs = final; };
      };
    in
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ overlay ];
        };
      in
      {
        packages = {
          veyon = pkgs.veyon;
          default = pkgs.veyon;
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            gettext
            qt6.wrapQtAppsHook
            gcc
            gnumake
          ];
        };
      }
    ) // { overlay = overlay; };
}