## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

To get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.


Im trying to write in c following this https://learn.pimoroni.com/article/pico-development-using-wsl.
Ive made a docker file for it.



The `Serial Monitor` extension from microsoft works even in the container and I can see the printfs from the board.
Getting the serial monitor to work reliable is a little tricky:
Often after reprogramming, the serila monitor will hang `Waiting for reconnection`.
Instead of changing the settings, a press of the reset button should work reestablish connection.

the pico wireless example isnt an example of using the rasp pi pico w to connect to the internt.
it is for this board which uses an esp32: https://shop.pimoroni.com/products/pico-wireless-pack
See this:
https://datasheets.raspberrypi.com/picow/connecting-to-the-internet-with-pico-w.pdf
