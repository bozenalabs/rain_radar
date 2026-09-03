# Firmware C/C++

## Building with Nix Shell (Recommended)

Start a shell with the complete ARM toolchain, Pico SDK (with submodules), picotool, and CMake:

```bash
nix-shell
```

From inside the nix-shell:

### Build for Pico 2 W:
```bash
mkdir -p firmware_c/rain_radar_app/build_pico2_w
cd firmware_c/rain_radar_app/build_pico2_w
cmake ..
make -j$(nproc)
```

### Build for Pico W:
```bash
mkdir -p firmware_c/rain_radar_app/build_pico_w
cd firmware_c/rain_radar_app/build_pico_w
cmake ..
make -j$(nproc)
```

Single-command build outside the shell:
```bash
nix-shell --run "cd firmware_c/rain_radar_app && mkdir -p build_pico2_w && cd build_pico2_w && cmake .. && make -j\$(nproc)"
```

## Building with Docker (Alternative)
- Start container with `docker compose up --detach` in `firmware_c/`.

### misc
https://www.raspberrypi.com/documentation/pico-sdk/

Note that the pico wireless example isnt an example of using the rasp pi pico w to connect to the internt.
It's for this board which uses an esp32: https://shop.pimoroni.com/products/pico-wireless-pack
See this:
https://datasheets.raspberrypi.com/picow/connecting-to-the-internet-with-pico-w.pdf


