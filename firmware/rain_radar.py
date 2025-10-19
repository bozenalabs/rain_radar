import gc
import jpegdec
from urllib import urequest
from ujson import load
import inky_helper as ih
import time

gc.collect()

graphics = None
WIDTH = None
HEIGHT = None

IMAGE_FILE_NAME = "rain_radar.jpg"
IMAGE_INFO_FILE_NAME = "image_info.json"

# Length of time between updates in minutes.
# Frequent updates will reduce battery life!
UPDATE_INTERVAL = 240

BLACK = 0
WHITE = 1
GREEN = 2
BLUE = 3
RED = 4
YELLOW = 5
ORANGE = 6
TAUPE = 7

error_string = ""
IMG_URL = "https://muse-hub.taile8f45.ts.net/combined.jpg"
JSON_URL = "https://muse-hub.taile8f45.ts.net/image_info.json"

def update():
    global error_string
    error_string = ""

    try:
        # Grab the image
        socket = urequest.urlopen(IMG_URL)
        gc.collect()
        data = bytearray(1024)
        with open(IMAGE_FILE_NAME, "wb") as f:
            while True:
                if socket.readinto(data) == 0:
                    break
                f.write(data)
        socket.close()
        del data
        gc.collect()
    except OSError as e:
        print(e)
        error_string = "Unable to download image"
    else:
        try:
            # Grab the image info JSON
            socket = urequest.urlopen(JSON_URL)
            gc.collect()
            data = bytearray(1024)
            with open(IMAGE_INFO_FILE_NAME, "w") as f:
                while True:
                    if socket.readinto(data) == 0:
                        break
                    f.write(data)
            socket.close()
            del data
            gc.collect()
            print("Downloaded image info")
        except OSError as e:
            print(e)
            error_string = "Unable to download image info"

def open_image_info():
    with open(IMAGE_INFO_FILE_NAME, "r") as f:
        info = load(f)
        return info


def draw():
    global error_string
    # TODO: https://github.com/pimoroni/inky-frame/blob/main/examples/display_png.py
    jpeg = jpegdec.JPEG(graphics)
    gc.collect()  # For good measure...

    graphics.set_pen(WHITE)
    graphics.clear()

    try:
        jpeg.open_file(IMAGE_FILE_NAME)
        jpeg.decode()
    except OSError:
        graphics.set_pen(RED)
        graphics.rectangle(0, (HEIGHT // 2) - 20, WIDTH, 40)
        graphics.set_pen(WHITE)
        graphics.text("Unable to display image!", 5, (HEIGHT // 2) - 15, WIDTH, 2)
        graphics.text("Check your network settings in secrets.py", 5, (HEIGHT // 2) + 2, WIDTH, 2)

    print("Displayed image")
    if ih.file_exists(IMAGE_INFO_FILE_NAME):
        image_info = open_image_info()
        image_info_text = image_info["text"]
        current_precip_ts = image_info["precip_ts"]
        current_time = time.time()
        if current_time - current_precip_ts > 30*60 and error_string == "":
            error_string = f"Time diff too high: {current_time - current_precip_ts} secs (current_time: {current_time}, precip_ts: {current_precip_ts})"
    else:
        image_info_text = "No image info found"
        if error_string == "":
            error_string = image_info_text


    graphics.set_pen(BLUE)
    graphics.rectangle(0, HEIGHT - 25, WIDTH, 25)
    graphics.set_pen(WHITE)
    graphics.text(image_info_text, 5, HEIGHT - 20, WIDTH, 2)
   
    if error_string != "":
        graphics.set_pen(RED)
        graphics.rectangle(0, 10, WIDTH, 35)
        graphics.set_pen(WHITE)
        graphics.text(error_string, 5, 16, WIDTH - 5, 2)

    gc.collect()
    graphics.update()
