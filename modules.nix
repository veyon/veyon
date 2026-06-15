{ config, lib, pkgs, ... }:

let
  cfg = config.services.veyon;
in
{
  options.services.veyon = {
    enable = lib.mkEnableOption "Veyon - computer monitoring and control software";

    package = lib.mkOption {
      type = lib.types.package;
      default = pkgs.veyon-unwrapped;
    };

    dataDir = lib.mkOption {
      type = lib.types.path;
      default = "/var/lib/veyon";
    };

    vncServer = lib.mkOption {
      type = lib.types.enum [ "none" "builtin" "external" ];
      default = "builtin";
    };

    vncServerPort = lib.mkOption {
      type = lib.types.port;
      default = 5900;
    };

    publicKey = lib.mkOption {
      description = "Named public key stored under /etc/veyon/keys/public/<name>/key";

      type = lib.types.submodule {
        options = {
          name = lib.mkOption {
            type = lib.types.str;
            default = "test";
          };

          value = lib.mkOption {
            type = lib.types.lines;
            default = ''
              -----BEGIN PUBLIC KEY-----
              ...
              -----END PUBLIC KEY-----
            '';
          };
        };
      };
    };
  };

  config = lib.mkIf cfg.enable {
    users.groups.veyon = { };

    environment.systemPackages = [ cfg.package ];

    # writes:
    # /etc/veyon/keys/public/<name>/key
    environment.etc."veyon/keys/public/${cfg.publicKey.name}/key".text =
      cfg.publicKey.value;

    systemd.tmpfiles.rules = [
      "d ${cfg.dataDir} 0750 root veyon - -"
    ];

    systemd.services.veyon-service = {
      description = "Veyon Service (client daemon)";
      wantedBy = [ "multi-user.target" ];
      after = [ "network-online.target" "dbus.service" "systemd-logind.service" ];
      wants = [ "network-online.target" ];
      requires = [ "dbus.service" "systemd-logind.service" ];

      serviceConfig = {
        Type = "simple";
        Restart = "always";
        RestartSec = 60;

        ExecStart = "${cfg.package}/bin/veyon-service";
        Group = "veyon";
        SupplementaryGroups = [ "veyon" ];

        Environment = [
          "HOME=%h"
          "QT_QPA_PLATFORM=offscreen"
          "QT_PLUGIN_PATH=${cfg.package}/lib/qt-6/plugins"
        ];

        EnvironmentFile = "-${cfg.dataDir}/environment";

        ProtectSystem = "strict";
        ReadWritePaths = [ cfg.dataDir ];
        PrivateTmp = true;
        NoNewPrivileges = true;
      };
    };

    networking.firewall = lib.mkIf (cfg.vncServer != "none") {
      allowedTCPPorts = [ cfg.vncServerPort ];
      allowedUDPPorts = [ cfg.vncServerPort ];
    };

    security.wrappers.veyon-auth-helper = {
      source = "${cfg.package}/bin/veyon-auth-helper";
      owner = "root";
      group = "root";
      setuid = true;
    };
  };
}
