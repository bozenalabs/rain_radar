import os
import requests
from pathlib import Path
from zoneinfo import ZoneInfo

# if token := os.environ.get("RAINBOW_API_TOKEN"):
#     RAINBOW_API_TOKEN = token
from PIL import Image, ImageEnhance
import qrcode

import api_secrets 
import json
import datetime as dt
import ipdb
import math
import argparse
import shutil


IMAGES_DIR = Path("images")
IMAGES_DIR.mkdir(exist_ok=True)

PRECIP_TILE_FILE = IMAGES_DIR / ("forecast.png")
MAP_TILE_FILE = IMAGES_DIR / ("map.png")
QRCODE_FILE = IMAGES_DIR / ("qrcode.png")
COMBINED_FILE = IMAGES_DIR / ("combined.jpg")
QUANTIZED_FILE = IMAGES_DIR / ("quantized.bin")
IMAGE_INFO_FILE = IMAGES_DIR / ("image_info.txt")

FORECAST_SECS = 600  # 10 minutes


def get_snapshot_timestamp():
    response = requests.get(
        f"https://api.rainbow.ai/tiles/v1/snapshot?token={api_secrets.RAINBOW_API_TOKEN}"
    )
    return response.json()["snapshot"]


def get_tile_handler(snapshot_timestamp: int, zoom: int, x: int, y: int):
    # ipdb.set_trace()
    url = f"https://api.rainbow.ai/tiles/v1/precip/{snapshot_timestamp}/{FORECAST_SECS}/{zoom}/{x}/{y}?token={api_secrets.RAINBOW_API_TOKEN}&color=1"
    # ipdb.set_trace()p
    print(url)
    response = requests.get(url, stream=True, timeout=10)
    return response


# ZOOM = 10
# TILE_X = 511
# TILE_Y = 340
ZOOM = 7
TILE_X = 63
TILE_Y = 42


def download_precip_image(zoom, tile_x, tile_y, ts):
    file_path = IMAGES_DIR / f"precip_{zoom}_{tile_x}_{tile_y}.png"
    if not file_path.exists():
        print("Downloading forecast image...")

        response = get_tile_handler(ts, zoom, tile_x, tile_y)
        assert response.status_code == 200

        with open(file_path, "wb") as f:
            f.write(response.content)

    return file_path


def download_map_image(zoom, tile_x, tile_y):
    file_path = IMAGES_DIR / f"map_{zoom}_{tile_x}_{tile_y}.png"

    if not file_path.exists():
        url = f"https://api.maptiler.com/maps/0199e42b-f3ba-728f-81a6-ba4d151cc8fb/{zoom}/{tile_x}/{tile_y}.png?key={api_secrets.MAPTILER_API_KEY}"
        headers = {"User-Agent": "TileFetcher/1.0 (your.email@example.com)"}
        r = requests.get(url, headers=headers, timeout=10)

        if r.status_code != 200:
            raise RuntimeError(f"Failed to fetch tile: {r.status_code}")
        

        with open(file_path, "wb") as f:
            f.write(r.content)
    return file_path



def qr_code_image():
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=1,
        border=1,
    )
    # qr.add_data("https://weather.metoffice.gov.uk/maps-and-charts/rainfall-radar-forecast-map#?model=ukmo-ukv&layer=rainfall-rate&bbox=[[50.75904732375726,-2.4554443359375004],[52.22948173332481,2.2906494140625004]]")
    qr.add_data(
        "https://weather.metoffice.gov.uk/maps-and-charts/rainfall-radar-forecast-map#?bbox=[[50.759,-2.455],[52.229,2.290]]"
    )
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    img.save(QRCODE_FILE)
    print("Saved QR code image.")


DESIRED_WIDTH = 800
DESIRED_HEIGHT = 480

def download_range_of_tiles(zoom, tile_start_x, tile_start_y, tile_end_x, tile_end_y, ts):
    map_tiles = {}
    precip_tiles = {}
    for x in range(tile_start_x, tile_end_x + 1):
        for y in range(tile_start_y, tile_end_y + 1):
            im_path = download_map_image(zoom, x, y)
            map_tiles[(x, y)] = Image.open(im_path)
            im_path = download_precip_image(zoom, x, y, ts)
            precip_tiles[(x, y)] = Image.open(im_path)

    # assert all the values of each on the same size
    assert len(set(im.size for im in map_tiles.values())) == 1
    assert len(set(im.size for im in precip_tiles.values())) == 1

    num_tiles_x = tile_end_x - tile_start_x + 1
    num_tiles_y = tile_end_y - tile_start_y + 1

    map_tile_width, map_tile_height = next(iter(map_tiles.values())).size
    precip_tile_width, precip_tile_height = next(iter(precip_tiles.values())).size

    combined_map = Image.new("RGB", (map_tile_width * num_tiles_x, map_tile_height * num_tiles_y))
    combined_precip = Image.new("RGBA", (precip_tile_width * num_tiles_x, precip_tile_height * num_tiles_y))

    # combine the tiles into one image
    for ix, x in enumerate(range(tile_start_x, tile_end_x + 1)):
        for iy, y in enumerate(range(tile_start_y, tile_end_y + 1)):
            map_tile = map_tiles[(x, y)]
            precip_tile = precip_tiles[(x, y)]
            combined_map.paste(map_tile, (ix * map_tile_width, iy * map_tile_height))
            combined_precip.paste(precip_tile, (ix * precip_tile_width, iy * precip_tile_height))

    combined_map.save(MAP_TILE_FILE)
    combined_precip.save(PRECIP_TILE_FILE)
    print("Combined map and precipitation tiles into single images.")

def build_image():

    precip_ts = get_snapshot_timestamp()
    print(f"Snapshot timestamp: {precip_ts}")
    download_range_of_tiles(ZOOM, TILE_X, TILE_Y, TILE_X+1, TILE_Y+1, precip_ts)

    with open(IMAGE_INFO_FILE, "w") as f:
        f.write(f"precip_ts={precip_ts}\n")
        text = (
            dt.datetime.fromtimestamp(precip_ts, tz=ZoneInfo("Europe/London")).strftime(
                "%Y-%m-%d %H:%M:%S"
            )
            + " + "
            + f"{FORECAST_SECS//60} mins forecast"
        )
        f.write(f"text={text}\n")

    qr_code_image()

    map_img = Image.open(MAP_TILE_FILE).convert("RGBA")
    precip_img = Image.open(PRECIP_TILE_FILE).convert("RGBA")
    qr_img = Image.open(QRCODE_FILE).convert("RGBA")

    assert map_img.size[0] / map_img.size[1] == precip_img.size[0] / precip_img.size[1]

    precip_img = precip_img.resize(map_img.size, resample=Image.BILINEAR)

    combined = Image.alpha_composite(map_img, precip_img)

    
    # combined.paste(qr_img, (1, 52)) # near the top left corner

    combined = combined.convert("RGB")
    current_width, current_height = combined.size
    if current_width / current_height > DESIRED_WIDTH / DESIRED_HEIGHT:
        # too wide
        cropped_width = current_height * DESIRED_WIDTH / DESIRED_HEIGHT
        assert cropped_width <= current_width
        cropped_width_start = (current_width - cropped_width) / 2
        bounding_box = (
            cropped_width_start,
            0,
            cropped_width + cropped_width_start,
            current_height,
        )
    else:
        # too tall
        cropped_height = current_width * DESIRED_HEIGHT / DESIRED_WIDTH
        assert cropped_height <= current_height
        # cropped_height_start = (current_height - cropped_height) / 2
        cropped_height_start = 0
        bounding_box = (
            0,
            cropped_height_start,
            current_width,
            cropped_height + cropped_height_start,
        )

    combined = combined.crop(bounding_box)

    
    # zoom into the center quarter of the image
    width, height = combined.size
    scale = 0.7
    centre_point = (width*0.38, height*0.35)
    new_width = int(width * scale)
    new_height = int(height * scale)
    left = centre_point[0] - new_width // 2
    upper = centre_point[1] - new_height // 2
    right = centre_point[0] + new_width // 2
    lower = centre_point[1] + new_height // 2
    combined = combined.crop((left, upper, right, lower))

    combined = combined.resize(
        (DESIRED_WIDTH, DESIRED_HEIGHT), resample=Image.BILINEAR
    )



    convert_to_bitmap(combined)
    combined = ImageEnhance.Color(combined).enhance(1.3)
    combined.save(COMBINED_FILE, progressive=False, quality=85)
    print("Combined map.png and forecast.png into one image.")


PALETTE = (
    0,
    0,
    0,  # 1 Black
    255,
    255,
    255,  # 2 White
    0,
    255,
    0,  # 3 Green
    0,
    0,
    255,  # 4 Blue
    255,
    0,
    0,  # 5 Red
    255,
    255,
    0,  # 6 Yellow
    255,
    140,
    0,  # 7 Orange
    255,
    255,
    255,  # ^3
)


def convert_to_bitmap(img):

    # Image to hold the quantize palette
    pal_img = Image.new("P", (1, 1))

    pal_img.putpalette(PALETTE + (0, 0, 0) * 8 * 31, rawmode="RGB")

    # Open the source image and quantize it to our palette
    quantized_img = img.convert("RGB").quantize(
        palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG
    )

    # draw color swatches across the top edge using palette indices

    # num_colors = len(PALETTE) // 3
    # for i in range(num_colors):
    #     swatch_width = 70
    #     swatch_height = 30
    #     x0 = 100 + i * swatch_width
    #     x1 = min(x0 + swatch_width, DESIRED_WIDTH)
    #     for x in range(x0, x1):
    #         for y in range(0, swatch_height):
    #             quantized_img.putpixel((x, y), i)

    # so we can see it
    quantized_img.convert("RGB").save(COMBINED_FILE.with_name("quantized.jpg"))

    # for other picos, the frame buffer on the pico is logically 3 single bit planes one after anther.
    # plane_0[x,y] = bit 0 of color
    # plane_1[x,y] = bit 1 of color
    # plane_2[x,y] = bit 2 of color
    # BUT for the 7.3, the buffer is stored on psram and for some reason
    # it is just an array of bytes, one byte per pixel, each byte is the color index.

    framebuffer = bytearray(DESIRED_WIDTH * DESIRED_HEIGHT)
    counter = 0
    for y in range(DESIRED_HEIGHT):
        for x in range(DESIRED_WIDTH):
            c = quantized_img.getpixel((x, y))
            framebuffer[counter] = c
            counter += 1

    with open(QUANTIZED_FILE, "wb") as f:
        f.write(framebuffer)
    print("Wrote quantized framebuffer.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--deploy", action="store_true", help="Copy the generated combined image to the deployment directory")
    args = parser.parse_args()

    build_image()
    if args.deploy:
        deploy_dir = Path("publicly_available/")
        deploy_dir.mkdir(exist_ok=True)
        shutil.copy(COMBINED_FILE, deploy_dir / COMBINED_FILE.name)
        shutil.copy(QUANTIZED_FILE, deploy_dir / QUANTIZED_FILE.name)
        shutil.copy(IMAGE_INFO_FILE, deploy_dir / IMAGE_INFO_FILE.name)
        print(f"Copied images to {deploy_dir}")


