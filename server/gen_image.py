import os
import requests
import requests
from pathlib import Path
from zoneinfo import ZoneInfo

# if token := os.environ.get("RAINBOW_API_TOKEN"):
#     RAINBOW_API_TOKEN = token
from PIL import Image, ImageEnhance
import qrcode

from api_secrets import RAINBOW_API_TOKEN
import json
import datetime as dt


PRECIP_TILE_FILE = Path("forecast.png")
MAP_TILE_FILE = Path("map.png")
QRCODE_FILE = Path("qrcode.png")
COMBINED_FILE = Path("publicly_available/combined.jpg")
QUANTIZED_FILE = Path("publicly_available/quantized.bin")

FORECAST_SECS = 600  # 10 minutes


def get_snapshot_timestamp():
    response = requests.get(f"https://api.rainbow.ai/tiles/v1/snapshot?token={RAINBOW_API_TOKEN}")
    return response.json()["snapshot"]


def get_tile_handler(snapshot_timestamp: int, zoom: int, x: int, y: int):
    url = f"https://api.rainbow.ai/tiles/v1/precip/{snapshot_timestamp}/FORECAST_SECS/{zoom}/{x}/{y}?token={RAINBOW_API_TOKEN}&color=8"
    response = requests.get(url, stream=True, timeout=10)
    return response

ZOOM = 5
TILE_X = 15
TILE_Y = 9

def download_precip_image():
    print("Downloading forecast image...")
    ts = get_snapshot_timestamp()
    print(f"Snapshot timestamp: {ts}")
    response = get_tile_handler(ts,ZOOM,TILE_X,TILE_Y)
    if response.status_code == 200:
        with open(PRECIP_TILE_FILE, "wb") as f:
            f.write(response.content)
        print("Image updated.")
    else:
        print(f"Failed to fetch: {response.status_code}")
    
    return ts

def download_map_image():
    url = f"https://tile.openstreetmap.org/{ZOOM}/{TILE_X}/{TILE_Y}.png"
    headers = {"User-Agent": "TileFetcher/1.0 (your.email@example.com)"}
    r = requests.get(url, headers=headers, timeout=10)

    if r.status_code != 200:
        raise RuntimeError(f"Failed to fetch tile: {r.status_code}")

    with open(MAP_TILE_FILE, "wb") as f:
        f.write(r.content)


def qr_code_image():
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=1,
        border=1,
    )
    # qr.add_data("https://weather.metoffice.gov.uk/maps-and-charts/rainfall-radar-forecast-map#?model=ukmo-ukv&layer=rainfall-rate&bbox=[[50.75904732375726,-2.4554443359375004],[52.22948173332481,2.2906494140625004]]")
    qr.add_data("https://weather.metoffice.gov.uk/maps-and-charts/rainfall-radar-forecast-map#?bbox=[[50.759,-2.455],[52.229,2.290]]")
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    img.save(QRCODE_FILE)
    print("Saved QR code image.")

DESIRED_WIDTH = 800
DESIRED_HEIGHT = 480

def build_image():
    download_map_image()
    precip_ts = download_precip_image()
    info_path = COMBINED_FILE.parent / "image_info.txt"
    with open(info_path, "w") as f:
        f.write(f"precip_ts={precip_ts}\n")
        text = dt.datetime.fromtimestamp(precip_ts, tz=ZoneInfo("Europe/London")).strftime("%Y-%m-%d %H:%M:%S") + " + "+ f"{FORECAST_SECS//60} mins forecast"
        f.write(f"text={text}\n")

    qr_code_image()



    with Image.open(MAP_TILE_FILE).convert("RGBA") as map_img, Image.open(PRECIP_TILE_FILE).convert("RGBA") as precip_img, Image.open(QRCODE_FILE).convert("RGBA") as qr_img:
        if map_img.size != precip_img.size:
            precip_img = precip_img.resize(map_img.size, resample=Image.BILINEAR)
        combined = Image.alpha_composite(map_img, precip_img)
        combined.paste(qr_img, (1,52))
        rgb_image = combined.convert("RGB")
        current_width, current_height = rgb_image.size
        if current_width / current_height > DESIRED_WIDTH / DESIRED_HEIGHT:
            # too wide
            cropped_width = current_height * DESIRED_WIDTH / DESIRED_HEIGHT
            assert cropped_width <= current_width
            cropped_width_start = (current_width - cropped_width) / 2
            bounding_box = (cropped_width_start, 0, cropped_width+cropped_width_start, current_height)
        else:
            # too tall
            cropped_height = current_width * DESIRED_HEIGHT / DESIRED_WIDTH
            assert cropped_height <= cropped_height
            cropped_height_start = (current_height - cropped_height) / 2
            bounding_box = (0, cropped_height_start, current_width, cropped_height+cropped_height_start)

        rgb_image = rgb_image.resize((DESIRED_WIDTH,DESIRED_HEIGHT), box=bounding_box, resample=Image.BILINEAR)
        convert_to_bitmap(rgb_image)
        rgb_image = ImageEnhance.Color(rgb_image).enhance(1.3)
        rgb_image.save(COMBINED_FILE, progressive=False, quality=85)
        print("Combined map.png and forecast.png into one image.")

PALETTE = (
    0, 0, 0,        # 1 Black
    255, 255, 255,  # 2 White
    0, 255, 0,      # 3 Green
    0, 0, 255,      # 4 Blue
    255, 0, 0,      # 5 Red
    255, 255, 0,    # 6 Yellow
    255, 140, 0,    # 7 Orange
    255, 255, 255   # ^3
)
def convert_to_bitmap(img):

    # Image to hold the quantize palette
    pal_img = Image.new("P", (1, 1))

    pal_img.putpalette(PALETTE + (0, 0, 0) * 8 * 31, rawmode='RGB')

    # Open the source image and quantize it to our palette
    quantized_img = img.convert("RGB").quantize(palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG)


    # draw color swatches across the top edge using palette indices
    num_colors = len(PALETTE) // 3
    square_size = min(DESIRED_WIDTH // num_colors, max(1, DESIRED_HEIGHT // 10))

    for i in range(num_colors):
        x0 = 200 + i * square_size
        x1 = min(x0 + square_size, DESIRED_WIDTH)
        for x in range(x0, x1):
            for y in range(0, square_size):
                quantized_img.putpixel((x, y), i)

    # so we can see it
    quantized_img.convert("RGB").save(COMBINED_FILE.with_name("quantized.jpg"))

    def put_pixel(buf:bytearray, x:int, y:int, c):
        offset      = (DESIRED_WIDTH * DESIRED_HEIGHT) // 8
        pixel       = (x // 8) + (y * DESIRED_WIDTH // 8)
        bit_offset  = 7 - (x & 0b111)

        for i in range(3):
            b = (c >> (2 - i)) & 1
            buf[pixel + i * offset] &= ~(1 << bit_offset)
            buf[pixel + i * offset] |= (b << bit_offset)
    
    # 3 single bit planes.
    # plane_0[x,y] = bit 0 of color
    # plane_1[x,y] = bit 1 of color
    # plane_2[x,y] = bit 2 of color
    
    framebuffer = bytearray(DESIRED_WIDTH * DESIRED_HEIGHT // 8 * 3)
    for x in range(DESIRED_WIDTH):
        for y in range(DESIRED_HEIGHT):
            c = quantized_img.getpixel((x,y))
            put_pixel(framebuffer, x, y, c)
    with open(QUANTIZED_FILE, "wb") as f:
        f.write(framebuffer)
    print("Wrote quantized framebuffer.")

if __name__ == "__main__":
    build_image()
