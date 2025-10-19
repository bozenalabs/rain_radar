## notes

https://www.raspberrypi.com/documentation/pico-sdk/

I start the container with `docker compose up --detach`.
Then I launch vscode to attach to the container.
in the container I have setup vscode similar to: https://paulbupejr.com/raspberry-pi-pico-windows-development/
I left the cmake kit as unspecified.
And selected the rain radar target.
Then Cmake build command works.
Put the pico into bootloader and copy over `docker cp 2e933ce5c2cc:/home/appuser/pico/rain_radar_app/build/rain_radar.uf2 /media/my_user/RPI-RP2`
