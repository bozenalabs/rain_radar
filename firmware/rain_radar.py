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

IMAGE_FILE_NAME = "rain_radar.bin"
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
QUANTIZED_URL = "https://muse-hub.taile8f45.ts.net/quantized.bin"
JSON_URL = "https://muse-hub.taile8f45.ts.net/image_info.json"

def update():
    global error_string
    error_string = ""

    try:
        # Grab the image
        socket = urequest.urlopen(QUANTIZED_URL)
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


INKY_7_WIDTH_PX = 800 
INKY_7_HEIGHT_PX = 480
NB_BYTES_TOTAL = (INKY_7_WIDTH_PX * INKY_7_HEIGHT_PX // 8) * 3
NB_PIX_PER_PLANE = NB_BYTES_TOTAL // 3

def iter_color_spans_from_buffer(buf):
    current_col = None
    span_nb = 1
    current_x = 0
    current_y = 0
    for i in range(NB_PIX_PER_PLANE):
      for j in range(7, -1, -1):
        col = 0
        mask = 1 << j
        col |= ((buf[i] & mask) >> j) << 2
        col |= ((buf[i + NB_PIX_PER_PLANE] & mask) >> j ) << 1
        col |= ((buf[i + NB_PIX_PER_PLANE + NB_PIX_PER_PLANE] & mask) >> j)
        if current_col == col:
          span_nb += 1
        elif current_col is None:
          current_col = col
        else:
          new_index = current_x + span_nb
          if new_index >= INKY_7_WIDTH_PX:
              first_line_span = INKY_7_WIDTH_PX - current_x
              span_nb -= first_line_span
              # emit remaining on line
              yield current_col, first_line_span, current_x, current_y
              current_x = 0
              current_y += 1
              # if span multiple line, emit them
              while span_nb > INKY_7_WIDTH_PX:
                yield current_col, INKY_7_WIDTH_PX, current_x, current_y
                current_y += 1
                span_nb -= INKY_7_WIDTH_PX
          if span_nb:
            yield current_col, span_nb, current_x, current_y
            current_x += span_nb
          current_col = col
          span_nb = 1
    yield current_col, span_nb, current_x, current_y

def draw():
    global error_string
    # TODO: https://github.com/pimoroni/inky-frame/blob/main/examples/display_png.py
    # jpeg = jpegdec.JPEG(graphics)
    gc.collect()  # For good measure...

    graphics.set_pen(WHITE)
    graphics.clear()

    try:
        # jpeg.open_file(IMAGE_FILE_NAME)
        # jpeg.decode()
        print("starting to display image")
        print(gc.mem_free())
        with open(IMAGE_FILE_NAME, "rb") as f:
            for col, span, x, y in iter_color_spans_from_buffer(f.read()):
                graphics.set_pen(col)
                graphics.pixel_span(x, y, span)
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
            error_string = f"Time diff too high: {current_time - current_precip_ts}s"
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
