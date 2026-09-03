{ pkgs ? import <nixpkgs> {} }:

let
  pico-sdk-full = pkgs.pico-sdk.override { withSubmodules = true; };
in
pkgs.mkShell {
  name = "rain-radar-dev-shell";

  packages = with pkgs; [
    # Pico / ARM toolchain
    gcc-arm-embedded-13
    picotool
    pico-sdk-full

    # Build tools
    cmake
    ninja
    gnumake
    pkg-config

    # Python & utilities
    python3
    git
  ];

  shellHook = ''
    export PICO_SDK_PATH="${pico-sdk-full}/lib/pico-sdk"
    export PIMORONI_PICO_PATH="$(pwd)/firmware_c/pimoroni-pico"

    echo "========================================="
    echo " Rain Radar Pico Firmware Dev Environment"
    echo "========================================="
    echo " PICO_SDK_PATH     : $PICO_SDK_PATH"
    echo " PIMORONI_PICO_PATH: $PIMORONI_PICO_PATH"
    echo " GCC Version       : $(arm-none-eabi-gcc --version | head -n 1)"
    echo " Picotool Version  : $(picotool version 2>/dev/null || echo 'available')"
    echo "========================================="
  '';
}
