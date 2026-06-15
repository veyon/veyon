{
  description = "Veyon - Virtual Eye On Networks - computer monitoring and control software";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    let
      cmakeFile = builtins.readFile ./CMakeLists.txt;
      cmakeFlat = builtins.replaceStrings [ "\n" ] [ " " ] cmakeFile;

      extractVersion =
        attr: default:
        let
          regex = ".*set\\(" + attr + " ([0-9]+)\\).*";
          m = builtins.match regex cmakeFlat;
        in
        if m == null then default else builtins.head m;

      versionMajor = extractVersion "VERSION_MAJOR" "4";
      versionMinor = extractVersion "VERSION_MINOR" "10";
      versionPatch = extractVersion "VERSION_PATCH" "2";
      defaultVersion = "${versionMajor}.${versionMinor}.${versionPatch}";

      overlay = final: prev: {
        makeVeyon =
          {
            version ? defaultVersion,
            qtVersion ? "qt6",
            withTests ? false,
            withTranslations ? true,
            withLTO ? false,
            buildType ? "Release",
            extraCMakeFlags ? [ ],
          }:
          let
            isQt6 = qtVersion == "qt6";
            qtPkgs = if isQt6 then final.qt6 else final.qt5;
          in
          final.stdenv.mkDerivation {
            pname = "veyon";
            inherit version;

            CI_COMMIT_TAG = "v${version}";
            GITHUB_REF_NAME = "v${version}";

            src = final.lib.cleanSource self;

            postPatch = ''
              substituteInPlace service/CMakeLists.txt \
                --replace-fail 'set(SYSTEMD_SERVICE_INSTALL_DIR /lib/systemd/system)' \
                               'set(SYSTEMD_SERVICE_INSTALL_DIR ''${CMAKE_INSTALL_PREFIX}/lib/systemd/system)'

              substituteInPlace plugins/platform/linux/auth-helper/CMakeLists.txt \
                --replace-fail \
                  'PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE' \
                  'PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE'

              substituteInPlace core/src/veyonconfig.h.in \
                --replace-fail '@VEYON_PLUGIN_DIR@' "$out/lib/veyon"
            '';

            postFixup = ''
              for bin in $out/bin/.*-wrapped; do
                patchelf --set-rpath "$out/lib/veyon:$(patchelf --print-rpath $bin)" $bin
              done
            '';

            nativeBuildInputs = with final; [
              cmake
              ninja
              pkg-config
              gettext
              qtPkgs.wrapQtAppsHook
              patchelf
            ];

            buildInputs =
              with final;
              [
                qtPkgs.qtbase
                qtPkgs.qttools
                qtPkgs.qttranslations
                openssl
                openldap
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
                libxcb-util
                libxcb-wm
                libxcb-image
                libxcb-cursor
                libxcb-keysyms
                libxcb-render-util
                pam
                systemd
                libglvnd
                libnl
                libpulseaudio
                libxslt
                cyrus_sasl
                procps
                libdbusmenu-qt5
              ]
              ++ final.lib.optionals isQt6 (
                with final;
                [
                  qt6.qt5compat
                  qt6.qthttpserver
                  qt6Packages.qca
                ]
              )
              ++ final.lib.optionals (!isQt6) (
                with final;
                [
                  qt5.qtx11extras
                  qt5Packages.qca
                ]
              );

            cmakeFlags = [
              "-DWITH_QT6=${if isQt6 then "ON" else "OFF"}"
              "-DWITH_TRANSLATIONS=${if withTranslations then "ON" else "OFF"}"
              "-DWITH_TESTS=${if withTests then "ON" else "OFF"}"
              "-DWITH_LTO=${if withLTO then "ON" else "OFF"}"
              "-DCMAKE_BUILD_TYPE=${buildType}"
              "-DCMAKE_C_FLAGS=-Wno-error"
              "-DCMAKE_INSTALL_RPATH=${placeholder "out"}/lib/veyon"
              "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON"
              "-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON"
            ]
            ++ extraCMakeFlags;

            env = {
              NIX_CFLAGS_COMPILE = "-Wno-error";
            };

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
              mainProgram = "veyon";
            };
          };

        veyon-unwrapped = final.makeVeyon { };
        veyon-unwrapped-qt5 = final.makeVeyon { qtVersion = "qt5"; };
        veyon-unwrapped-debug = final.makeVeyon { buildType = "Debug"; };

        veyon = final.veyon-unwrapped;
      };
    in
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ overlay ];
        };
      in
      {
        packages = {
          veyon = pkgs.veyon;
          veyon-qt5 = pkgs.veyon-unwrapped-qt5;
          veyon-debug = pkgs.veyon-unwrapped-debug;
          default = pkgs.veyon;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ pkgs.veyon-unwrapped ];
          packages = [ pkgs.gnumake ];
        };
      }
    )
    // {
      overlays.default = overlay;

      nixosModules.default = {
        imports = [ ./modules.nix ];
        nixpkgs.overlays = [ overlay ];
      };
    };
}
