{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    let
        # https://gist.github.com/adisbladis/2a44cded73e048458a815b5822eea195
        # NOTE: we pass in packages so that we can use pkgsCross as base (so that CC is set)
        mergeEnvs = pkgs: envs:
          let
            mergeVar = name:
              builtins.concatStringsSep " " (pkgs.lib.unique (builtins.filter (x: x != "") (map (v:
                if builtins.hasAttr name v
                then v.${name}
                else "") envs)));
            merged = builtins.foldl' (a: v: {
              buildInputs = a.buildInputs ++ v.buildInputs;
              nativeBuildInputs = a.nativeBuildInputs ++ v.nativeBuildInputs;
              propagatedBuildInputs = a.propagatedBuildInputs ++ v.propagatedBuildInputs;
              propagatedNativeBuildInputs = a.propagatedNativeBuildInputs ++ v.propagatedNativeBuildInputs;
              shellHook = a.shellHook + "\n" + v.shellHook;
            }) (pkgs.mkShellNoCC {}) envs;
          in
          pkgs.mkShell (merged // {
            CFLAGS = mergeVar "CFLAGS";
            LDFLAGS = mergeVar "LDFLAGS";
          });

        getPkgsStaticAarch64     = pkgs: pkgs.pkgsCross.aarch64-multiplatform.pkgsStatic;
        getPkgsStaticRiscv64     = pkgs: pkgs.pkgsCross.riscv64.pkgsStatic;
        getPkgsStaticLoongarch64 = pkgs: pkgs.pkgsCross.loongarch64-linux.pkgsStatic;
    in
    (flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pkgsStaticAarch64     = getPkgsStaticAarch64     pkgs;
        pkgsStaticRiscv64     = getPkgsStaticRiscv64     pkgs;
        pkgsStaticLoongarch64 = getPkgsStaticLoongarch64 pkgs;

        tools = (pkgs.mkShellNoCC {
          # NOTE: You can add language servers or other tools here.
          nativeBuildInputs = [
            pkgs.clang-tools
            pkgs.git
            pkgs.texlive.combined.scheme-full
          ];
        });

        pythonEnv = pkgs.mkShellNoCC {
          nativeBuildInputs = [
            (pkgs.python3.withPackages (ps: with ps; [
              pandas matplotlib scikitlearn
            ]))
          ];
        };

        set-path = (pkgs.mkShellNoCC {
          shellHook = ''
            source "$(git rev-parse --show-toplevel)/bin/set-path"
          '';
        });

        basePkgs = pkgsStaticAarch64;
        envVars = {
          CFLAGS = "-static";
          LDFLAGS = "-static";
        };

        # NOTE: we do it like this because loongarch64 env lost compiler etc.
        # when merging
        cc-aarch64       = pkgsStaticAarch64.buildPackages.gcc;
        binutils-aarch64 = pkgsStaticAarch64.buildPackages.binutils;
        aarch64Env = (basePkgs.mkShellNoCC ({
          nativeBuildInputs = [
            cc-aarch64 binutils-aarch64
          ];
        } // envVars));

        cc-riscv64       = pkgsStaticRiscv64.buildPackages.gcc;
        binutils-riscv64 = pkgsStaticRiscv64.buildPackages.binutils;
        riscv64Env = (basePkgs.mkShellNoCC ({
          nativeBuildInputs = [
            cc-riscv64 binutils-riscv64
          ];
        } // envVars));

        cc-loongarch64       = pkgsStaticLoongarch64.buildPackages.gcc;
        binutils-loongarch64 = pkgsStaticLoongarch64.buildPackages.binutils;
        loongarch64Env = (basePkgs.mkShellNoCC ({
          nativeBuildInputs = [
            cc-loongarch64 binutils-loongarch64
          ];
        } // envVars));

        envs = [tools set-path pythonEnv];
      in
      rec {
        devShells.tools = tools;

        devShells.aarch64     = mergeEnvs pkgsStaticAarch64     envs;
        devShells.riscv64     = mergeEnvs pkgsStaticRiscv64     envs;
        devShells.loongarch64 = mergeEnvs pkgsStaticLoongarch64 envs;

        devShells.merged  = mergeEnvs basePkgs (with devShells; envs++[loongarch64Env riscv64Env aarch64Env]);

        devShells.default = devShells.merged;
      })) // {
        inherit mergeEnvs nixpkgs getPkgsStaticAarch64 getPkgsStaticRiscv64 getPkgsStaticLoongarch64;
      };
}
