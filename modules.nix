{
  config,
  lib,
  pkgs,
  ...
}:

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

    environment.etc."veyon/keys/public/${cfg.publicKey.name}/key".text = cfg.publicKey.value;

    systemd.tmpfiles.rules = [ "d ${cfg.dataDir} 0750 root veyon - -" ];

    systemd.services.veyon-service = {
      description = "Veyon Service";

      documentation = [ "man:veyon-service(1)" ];

      after = [
        "network-online.target"
        "dbus.service"
        "systemd-logind.service"
      ];

      wants = [
        "network-online.target"
      ];

      requires = [
        "dbus.service"
        "systemd-logind.service"
      ];

      wantedBy = [ "multi-user.target" ];

      serviceConfig = {
        ExecStart = "${cfg.package}/bin/veyon-service";
        Type = "simple";
        Restart = "always";
        RestartSec = 0;
      };

      startLimitIntervalSec = 60;
      startLimitBurst = 10;
    };

    networking.firewall = {
      allowedTCPPorts = [
        11100
        11200
        11300
      ];
      allowedUDPPorts = [
        11100
        11200
        11300
      ];
    };

    security.wrappers.veyon-auth-helper = {
      source = "${cfg.package}/bin/veyon-auth-helper";
      owner = "root";
      group = "root";
      setuid = true;
    };
  };
}
