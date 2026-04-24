{
  description = "Veyon - Computer Monitoring and Classroom Management";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = system: nixpkgs.legacyPackages.${system};
    in {
      packages = forAllSystems (system:
        let pkgs = nixpkgsFor system; in {
          default = pkgs.callPackage ./nix/package.nix { src = self; };
          veyon  = pkgs.callPackage ./nix/package.nix { src = self; };
        }
      );

      devShells = forAllSystems (system:
        let pkgs = nixpkgsFor system; in {
          default = pkgs.mkShell {
            inputsFrom = [ (pkgs.callPackage ./nix/package.nix { }) ];
            # Extra tools useful during development
            packages = [ pkgs.ninja pkgs.cmake ];
          };
        }
      );
    };
}
