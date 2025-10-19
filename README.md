## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

To get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.


Im trying to write in c following this https://learn.pimoroni.com/article/pico-development-using-wsl.
Ive made a docker file for it.


I start the container with docker compose.
then i launch vscode to attach to the container.
in the container I have setup vscode similar to: https://paulbupejr.com/raspberry-pi-pico-windows-development/
I left the cmake kit as unspecified.
And selected the rain radar target.
Then Cmake build command works.
put the pico into bootloader and copy over `docker cp 2e933ce5c2cc:/home/appuser/pico/rain_radar_app/build/rain_radar.uf2 /media/my_user/RPI-RP2`

The `Serial Monitor` extension from microsoft works even in the container and I can see the printfs from the board.
Getting the serial monitor to work reliable is a little tricky:
Often after reprogramming, the serila monitor will hang `Waiting for reconnection`.
Instead of changing the settings, a press of the reset button should work reestablish connection.

the pico wireless example isnt an example of using the rasp pi pico w to connect to the internt.
it is for this board which uses an esp32: https://shop.pimoroni.com/products/pico-wireless-pack
See this:
https://datasheets.raspberrypi.com/picow/connecting-to-the-internet-with-pico-w.pdf

