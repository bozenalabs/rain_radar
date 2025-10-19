## notes

- I start the container with `docker compose up --detach`.
- Then I launch vscode to attach to the container.
- in the container I have setup vscode similar to: https://paulbupejr.com/raspberry-pi-pico-windows-development/
- I left the cmake kit as unspecified.
- `> CMake: Configure` And selected the rain radar CMakeLists.txt.
- Then `> Cmake: Build` command works.
- This should create the uf2 file.
- Put the pico into bootloader and copy over `docker cp 2e933ce5c2cc:/home/appuser/pico/rain_radar_app/build/rain_radar.uf2 /media/my_user/RPI-RP2`


https://www.raspberrypi.com/documentation/pico-sdk/
