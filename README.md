# InkyFrame 7.3" Rain Radar

### notes:

- https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555
- https://github.com/pimoroni/inky-frame

To get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.
I followed this to get started https://learn.pimoroni.com/article/pico-development-using-wsl.
There is a docker file that can be used for local development and by github actions.

The `Serial Monitor` extension from microsoft works even in the container and I can see the printfs from the board.
Getting the serial monitor to work reliable is a little tricky:
Often after reprogramming, the serila monitor will hang `Waiting for reconnection`.
Instead of changing the settings, a press of the reset button should work reestablish connection.

