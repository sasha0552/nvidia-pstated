{
  description = "A daemon that automatically manages the performance states of NVIDIA GPUs";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    nvapi-src = {
      url = "https://download.nvidia.com/XFree86/nvapi-open-source-sdk/R555-OpenSource.tar";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      nvapi-src,
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "nvidia-pstated";
        version = "0.1.0";

        src = ./.;

        nativeBuildInputs = [
          pkgs.cmake
        ];

        buildInputs = [
          pkgs.cudatoolkit
          pkgs.linuxPackages.nvidia_x11
        ];

        cmakeFlags = [
          "-DFETCHCONTENT_SOURCE_DIR_NVAPI=${nvapi-src}"
        ];

        installPhase = ''
          mkdir -p $out/bin
          cp nvidia-pstated $out/bin/
        '';
      };

      apps.${system}.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/nvidia-pstated";
      };

      nixosModules.default =
        {
          config,
          lib,
          pkgs,
          ...
        }:
        with lib;
        {
          options.services.nvidia-pstated = {
            enable = mkEnableOption "nvidia-pstated daemon";

            package = mkOption {
              type = types.package;
              default = self.packages.${system}.default;
              description = "nvidia-pstated package to use";
            };

            clockMode = mkOption {
              type = types.bool;
              default = false;
              description = ''
                Control the GPU clocks instead of the performance states.

                Required for GPUs that expose only a single performance state
                (Tesla P100, V100, etc). Needs root, so the service does not run
                as a dynamic user when this is enabled.
              '';
            };

            clockGpuLow = mkOption {
              type = types.int;
              default = 0;
              description = "Low performance graphics clock in MHz, in clock mode (0 = the lowest supported clock)";
            };

            clockGpuHigh = mkOption {
              type = types.int;
              default = 0;
              description = "High performance graphics clock in MHz, in clock mode (0 = the default clocks)";
            };

            clockMemLow = mkOption {
              type = types.int;
              default = 0;
              description = "Low performance memory clock in MHz, in clock mode (0 = unmanaged on Volta and newer)";
            };

            clockMemHigh = mkOption {
              type = types.int;
              default = 0;
              description = "High performance memory clock in MHz, in clock mode (0 = unmanaged on Volta and newer)";
            };

            ids = mkOption {
              type = types.str;
              default = "";
              description = "Comma-separated list of GPU IDs to manage (empty = all GPUs)";
            };

            temperatureThreshold = mkOption {
              type = types.int;
              default = 80;
              description = "Temperature threshold in degrees Celsius";
            };

            utilizationThreshold = mkOption {
              type = types.int;
              default = 0;
              description = "GPU utilization threshold in percentage";
            };

            performanceStateLow = mkOption {
              type = types.int;
              default = 8;
              description = "Low performance state value";
            };

            performanceStateHigh = mkOption {
              type = types.int;
              default = 16;
              description = "High performance state value";
            };

            sleepInterval = mkOption {
              type = types.int;
              default = 100;
              description = "Sleep interval in milliseconds";
            };

            iterationsBeforeSwitch = mkOption {
              type = types.int;
              default = 30;
              description = "Number of iterations before switching performance state";
            };

            iterationsBeforeIdle = mkOption {
              type = types.int;
              default = 9000;
              description = "Number of iterations before considering GPU idle";
            };

            disableFanScript = mkOption {
              type = types.str;
              default = "";
              description = "Script to run when disabling external fans";
            };

            enableFanScript = mkOption {
              type = types.str;
              default = "";
              description = "Script to run when enabling external fans";
            };

            keepaliveFanScript = mkOption {
              type = types.str;
              default = "";
              description = "Script to run periodically, with the intended state of the external fans";
            };

            iterationsBeforeKeepalive = mkOption {
              type = types.int;
              default = 10;
              description = "Set the number of iterations to wait before executing keepalive script again";
            };
          };

          config = mkIf config.services.nvidia-pstated.enable {
            systemd.services.nvidia-pstated = {
              description = "A daemon that automatically manages the performance states of NVIDIA GPUs";
              wantedBy = [ "multi-user.target" ];

              serviceConfig = {
                # Setting the clocks requires root, so a dynamic user is only used
                # when the daemon manages performance states
                DynamicUser = !config.services.nvidia-pstated.clockMode;
                ExecStart =
                  let
                    args = [
                      "--temperature-threshold"
                      (toString config.services.nvidia-pstated.temperatureThreshold)
                      "--utilization-threshold"
                      (toString config.services.nvidia-pstated.utilizationThreshold)
                      "--performance-state-low"
                      (toString config.services.nvidia-pstated.performanceStateLow)
                      "--performance-state-high"
                      (toString config.services.nvidia-pstated.performanceStateHigh)
                      "--sleep-interval"
                      (toString config.services.nvidia-pstated.sleepInterval)
                      "--iterations-before-switch"
                      (toString config.services.nvidia-pstated.iterationsBeforeSwitch)
                      "--iterations-before-idle"
                      (toString config.services.nvidia-pstated.iterationsBeforeIdle)
                      "--iterations-before-keepalive"
                      (toString config.services.nvidia-pstated.iterationsBeforeKeepalive)
                    ]
                    ++ optionals config.services.nvidia-pstated.clockMode (
                      [
                        "--clock-mode"
                      ]
                      ++ optionals (config.services.nvidia-pstated.clockGpuLow != 0) [
                        "--clock-gpu-low"
                        (toString config.services.nvidia-pstated.clockGpuLow)
                      ]
                      ++ optionals (config.services.nvidia-pstated.clockGpuHigh != 0) [
                        "--clock-gpu-high"
                        (toString config.services.nvidia-pstated.clockGpuHigh)
                      ]
                      ++ optionals (config.services.nvidia-pstated.clockMemLow != 0) [
                        "--clock-mem-low"
                        (toString config.services.nvidia-pstated.clockMemLow)
                      ]
                      ++ optionals (config.services.nvidia-pstated.clockMemHigh != 0) [
                        "--clock-mem-high"
                        (toString config.services.nvidia-pstated.clockMemHigh)
                      ]
                    )
                    ++ optionals (config.services.nvidia-pstated.ids != "") [
                      "--ids"
                      config.services.nvidia-pstated.ids
                    ]
                    ++ optionals (config.services.nvidia-pstated.disableFanScript != "") [
                      "--disable-fan-script"
                      config.services.nvidia-pstated.disableFanScript
                    ]
                    ++ optionals (config.services.nvidia-pstated.enableFanScript != "") [
                      "--enable-fan-script"
                      config.services.nvidia-pstated.enableFanScript
                    ]
                    ++ optionals (config.services.nvidia-pstated.keepaliveFanScript != "") [
                      "--keepalive-fan-script"
                      config.services.nvidia-pstated.keepaliveFanScript
                    ];
                  in
                  "${config.services.nvidia-pstated.package}/bin/nvidia-pstated ${concatStringsSep " " args}";
                Restart = "on-failure";
                RestartSec = "1s";
              };
            };
          };
        };
    };
}
