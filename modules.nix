{ config, lib, pkgs, ... }:

let
  cfg = config.services.veyon;
in
{
  options = {
    services.veyon = {
      enable = lib.mkEnableOption "Veyon - computer monitoring and control software";

      package = lib.mkOption {
        type = lib.types.package;
        default = pkgs.veyon-unwrapped;
        description = "Veyon package to use.";
      };

      enableMaster = lib.mkOption {
        type = lib.types.bool;
        default = false;
        description = "Whether to enable the Veyon Master (teacher) GUI application.";
      };

      enableService = lib.mkOption {
        type = lib.types.bool;
        default = false;
        description = "Whether to enable the Veyon Service (runs on client computers).";
      };

      enableConfigurator = lib.mkOption {
        type = lib.types.bool;
        default = false;
        description = "Whether to enable the Veyon Configurator (configuration tool).";
      };

      enableCLI = lib.mkOption {
        type = lib.types.bool;
        default = false;
        description = "Whether to enable the Veyon CLI tools.";
      };

      dataDir = lib.mkOption {
        type = lib.types.path;
        default = "/var/lib/veyon";
        description = "Directory for Veyon data files.";
      };

      configFile = lib.mkOption {
        type = lib.types.path;
        default = null;
        description = "Path to Veyon configuration file.";
        example = "/etc/veyon/veyon.ini";
      };

      vncServer = lib.mkOption {
        type = lib.types.enum [ "none" "builtin" "external" ];
        default = "builtin";
        description = "VNC server implementation to use.";
      };

      roomName = lib.mkOption {
        type = lib.types.str;
        default = "Default";
        description = "Default computer room name.";
      };

      masterPassword = lib.mkOption {
        type = lib.types.str;
        default = "";
        description = "Master password for teacher authentication (empty = generated at runtime).";
      };

      userAccessMode = lib.mkOption {
        type = lib.types.enum [ "none" "full" "view-only" "full-read-only" ];
        default = "none";
        description = "User access mode for remote access.";
      };

      vncServerPort = lib.mkOption {
        type = lib.types.port;
        default = 5900;
        description = "Default VNC server port.";
      };

      vncServerPassword = lib.mkOption {
        type = lib.types.str;
        default = "";
        description = "VNC server password (empty = auto-generated).";
      };

      extraOptions = lib.mkOption {
        type = lib.types.attrsOf lib.types.anything;
        default = { };
        description = "Additional Veyon configuration options.";
      };
    };
  };

  config = lib.mkIf cfg.enable {
    assertions = [
      {
        assertion = cfg.enableMaster || cfg.enableService || cfg.enableConfigurator || cfg.enableCLI;
        message = "At least one of enableMaster, enableService, enableConfigurator, or enableCLI must be enabled.";
      }
    ];

    users.groups.veyon = lib.mkIf cfg.enableService { };

    environment.systemPackages = [ ]
      ++ (lib.mkIf cfg.enableMaster [ cfg.package ])
      ++ (lib.mkIf cfg.enableService [ cfg.package ])
      ++ (lib.mkIf cfg.enableConfigurator [ cfg.package ])
      ++ (lib.mkIf cfg.enableCLI [ cfg.package ]);

    systemd.services.veyon-service = lib.mkIf cfg.enableService {
      wantedBy = [ "multi-user.target" ];
      partOf = [ "graphical.target" ];
      after = [ "network-online.target" "dbus.service" "systemd-logind.service" ];
      requires = [ "dbus.service" "systemd-logind.service" ];
      serviceConfig = {
        Type = "simple";
        Restart = "always";
        RestartSec = 60;
        ExecStart = "${cfg.package}/bin/veyon-service";
        Environment = "HOME=%h";
        EnvironmentFile = lib.mkIf (cfg.dataDir != null) "${cfg.dataDir}/environment";
        SupplementaryGroups = "veyon";
        Group = "veyon";
      };
    };

    boot.extraModprobeConfig = lib.mkIf (cfg.enableService && cfg.vncServer != "none") [
      "nf_conntrack_netlink"
      "nf_conntrack"
    ];

    services.udev.rules = lib.mkIf (cfg.enableService && cfg.vncServer != "none") [
      ''
        ACTION=="add", SUBSYSTEM=="usb", RUN+="${pkgs.systemd}/bin/systemd-run --no-block --pty -M _guest veyon-service --register"
      ''
    ];

    systemd.tmpfiles.rules = [
      (lib.mkIf (cfg.dataDir != null) "d ${cfg.dataDir} 0755 root veyon - -")
    ];

    networking.firewall.allowedTCPPorts = lib.mkIf (cfg.enableService && cfg.vncServer != "none") [ cfg.vncServerPort ];
    networking.firewall.allowedUDPPorts = lib.mkIf (cfg.enableService && cfg.vncServer != "none") [ cfg.vncServerPort ];
  };
}