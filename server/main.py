import os
import requests
from pathlib import Path
from zoneinfo import ZoneInfo

from PIL import Image, ImageEnhance
import qrcode

import api_secrets 
import datetime as dt
import ipdb
import argparse
import shutil
from PIL import ImageDraw, ImageFont
import io
import numpy as np

IMAGES_DIR = Path("images")
IMAGES_DIR.mkdir(exist_ok=True)

PRECIP_NOW_TILE_FILE = IMAGES_DIR / ("precip_now.png")
PRECIP_FORECAST_TILE_FILE = IMAGES_DIR / ("precip_forecast.png")
MAP_TILE_FILE = IMAGES_DIR / ("map.png")
QRCODE_FILE = IMAGES_DIR / ("qrcode.png")
COMBINED_FILE = IMAGES_DIR / ("combined.jpg")
QUANTIZED_FILE = IMAGES_DIR / ("quantized.bin")
IMAGE_INFO_FILE = IMAGES_DIR / ("image_info.txt")

INTENSITY_MIN = 9
INTENSITY_MAX = 127

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)
RED = (255, 0, 0)
YELLOW = (255, 255, 0)
ORANGE = (255, 140, 0)

INKY_FRAME_PALETTE = (
    *BLACK,
    *WHITE,
    *GREEN,
    *BLUE,
    *RED,
    *YELLOW,
    *ORANGE,
)

def get_snapshot_timestamp():
    response = requests.get(
        f"https://api.rainbow.ai/tiles/v1/snapshot?token={api_secrets.RAINBOW_API_TOKEN}"
    )
    return response.json()["snapshot"]


def get_tile_handler(zoom: int, x: int, y: int, snapshot_timestamp: int, forecast_secs: int):
    url = f"https://api.rainbow.ai/tiles/v1/precip/{snapshot_timestamp}/{forecast_secs}/{zoom}/{x}/{y}?token={api_secrets.RAINBOW_API_TOKEN}&color=dbz_u8"
    # print(url)
    response = requests.get(url, stream=True, timeout=10)
    return response


# ZOOM = 10
# TILE_X = 511
# TILE_Y = 340
ZOOM = 7
TILE_X = 63
TILE_Y = 42



def lerp_color(color1: tuple, color2: tuple, t: float) -> tuple:
    """Linear interpolation between two colors"""
    return tuple(int(c1 + (c2 - c1) * t) for c1, c2 in zip(color1, color2))


import functools as ft

@ft.lru_cache(maxsize=None)
def intensity_to_color(intensity: int) -> tuple:
    """Convert DBZ intensity (0-127) to color with linear interpolation between palette colors"""
    if intensity <= INTENSITY_MIN:
        return (0, 0, 0, 0)  # Black for no precipitation
    
    # Define color stops with intensity values (0-127 range)
    color_stops = [
        (0, BLACK + (255,)),      # No precipitation
        (10, GREEN + (255,)),     # Light precipitation
        (30, BLUE + (255,)),      # Moderate precipitation
        (50, YELLOW + (255,)),    # Heavy precipitation
        (70, ORANGE + (255,)),    # Very heavy precipitation
        (100, RED + (255,)),      # Extreme precipitation
        (127, WHITE + (255,)),    # Maximum intensity
    ]
    
    # Clamp intensity to valid range
    intensity = max(INTENSITY_MIN, min(INTENSITY_MAX, intensity))
    
    # Find the two color stops to interpolate between
    for i in range(len(color_stops) - 1):
        intensity1, color1 = color_stops[i]
        intensity2, color2 = color_stops[i + 1]
        
        if intensity1 <= intensity <= intensity2:
            # Calculate interpolation factor (0.0 to 1.0)
            if intensity2 == intensity1:
                t = 0
            else:
                t = (intensity - intensity1) / (intensity2 - intensity1)
            
            # Interpolate between the two colors
            return lerp_color(color1, color2, t)
    
    assert False, "Should not reach here"


def process_dbz_u8(img: Image) -> Image:
    """Process dbz_u8 image: set pixel to pure white if red component & 128 == 128"""
    # Convert to RGBA to ensure we can work with individual color channels
    img = img.convert("RGBA")
    
    # Get pixel data as a list
    pixels = list(img.getdata())

    # import ipdb; ipdb.set_trace()
    
    # Process each pixel
    processed_pixels = []
    for r, g, b, a in pixels:
        assert r == g == b, "Expected grayscale image where R=G=B"
        if a == 0:
            # not rain data
            processed_pixels.append((0, 0, 0, 0))  # Keep fully transparent pixels as is
            continue
        if r & 128 == 128:  # Check if bit 7 (128) is set in red component, it is snow
            processed_pixels.append((255, 255, 255, a))
        else:
            # this is the interesting part, we can transform dbz to our colour palette
            processed_pixels.append(intensity_to_color(r))  # Use palette color with original alpha
    
    # Create new image with processed pixels
    processed_img = Image.new("RGBA", img.size)
    processed_img.putdata(processed_pixels)
    
    return processed_img

def download_precip_image(zoom, tile_x, tile_y, ts, forecast_secs):
    file_path = IMAGES_DIR / f"precip_{zoom}_{tile_x}_{tile_y}_{ts}_{forecast_secs}_dbz_u8.png"
    if not file_path.exists(): # or True:
        print("Downloading forecast image...")

        response = get_tile_handler(zoom, tile_x, tile_y, ts, forecast_secs)
        assert response.status_code == 200

        img = Image.open(io.BytesIO(response.content))
        if "dbz_u8" in file_path.name:
            img = process_dbz_u8(img)
        img.save(file_path)

    return file_path


def download_map_image(zoom, tile_x, tile_y):
    file_path = IMAGES_DIR / f"map_{zoom}_{tile_x}_{tile_y}.png"

    if not file_path.exists():
        url = f"https://api.maptiler.com/maps/0199e42b-f3ba-728f-81a6-ba4d151cc8fb/{zoom}/{tile_x}/{tile_y}.png?key={api_secrets.MAPTILER_API_KEY}"
        headers = {"User-Agent": "TileFetcher/1.0 (your.email@example.com)"}
        print(f"Downloading map image from {url}...")
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

def download_range_of_tiles(zoom, tile_start_x, tile_start_y, tile_end_x, tile_end_y, ts, forecast_secs):
    map_tiles = {}
    precip_tiles_now = {}
    precip_tiles_forecast = {}
    for x in range(tile_start_x, tile_end_x + 1):
        for y in range(tile_start_y, tile_end_y + 1):
            im_path = download_map_image(zoom, x, y)
            map_tiles[(x, y)] = Image.open(im_path)
            im_path = download_precip_image(zoom, x, y, ts, 0)
            precip_tiles_now[(x, y)] = Image.open(im_path)
            im_path = download_precip_image(zoom, x, y, ts, forecast_secs)
            precip_tiles_forecast[(x, y)] = Image.open(im_path)

    # assert all the values of each on the same size
    assert len(set(im.size for im in map_tiles.values())) == 1
    assert len(set(im.size for im in precip_tiles_now.values())) == 1
    assert len(set(im.size for im in precip_tiles_forecast.values())) == 1

    num_tiles_x = tile_end_x - tile_start_x + 1
    num_tiles_y = tile_end_y - tile_start_y + 1

    map_tile_width, map_tile_height = next(iter(map_tiles.values())).size
    precip_tile_width, precip_tile_height = next(iter(precip_tiles_forecast.values())).size


    combined_map = Image.new("RGB", (map_tile_width * num_tiles_x, map_tile_height * num_tiles_y))
    combined_precip_now = Image.new("RGBA", (precip_tile_width * num_tiles_x, precip_tile_height * num_tiles_y))
    combined_precip_forecast = Image.new("RGBA", (precip_tile_width * num_tiles_x, precip_tile_height * num_tiles_y))

    # combine the tiles into one image
    for ix, x in enumerate(range(tile_start_x, tile_end_x + 1)):
        for iy, y in enumerate(range(tile_start_y, tile_end_y + 1)):
            map_tile = map_tiles[(x, y)]
            precip_tile_now = precip_tiles_now[(x, y)]
            precip_tile_forecast = precip_tiles_forecast[(x, y)]
            combined_map.paste(map_tile, (ix * map_tile_width, iy * map_tile_height))
            combined_precip_now.paste(precip_tile_now, (ix * precip_tile_width, iy * precip_tile_height))
            combined_precip_forecast.paste(precip_tile_forecast, (ix * precip_tile_width, iy * precip_tile_height))

    combined_map.save(MAP_TILE_FILE)
    combined_precip_now.save(PRECIP_NOW_TILE_FILE)
    combined_precip_forecast.save(PRECIP_FORECAST_TILE_FILE)
    print("Combined map and precipitation tiles into single images.")

def build_image():

    precip_ts = get_snapshot_timestamp()
    print(f"Snapshot timestamp: {precip_ts}")
    FORECAST_SECS = 1200  # 20 minutes
    download_range_of_tiles(ZOOM, TILE_X, TILE_Y, TILE_X+1, TILE_Y+1, precip_ts, FORECAST_SECS)

    with open(IMAGE_INFO_FILE, "w") as f:
        image_text = dt.datetime.fromtimestamp(precip_ts, tz=ZoneInfo("Europe/London")).strftime(
                "%Y-%m-%d %H:%M:%S") + " + " + f"{FORECAST_SECS//60} mins forecast"
        f.write(f"precip_ts={precip_ts}\n")
        f.write(f"text={image_text}\n")

    qr_code_image()

    map_img = Image.open(MAP_TILE_FILE).convert("RGBA")
    precip_now_img = Image.open(PRECIP_NOW_TILE_FILE).convert("RGBA")
    precip_forecast_img = Image.open(PRECIP_FORECAST_TILE_FILE).convert("RGBA")
    qr_img = Image.open(QRCODE_FILE).convert("RGBA")

    # turn the old precip data into the lightest intensity
    # and draw the forecast over it
    # precip_now_img = precip_now_img.
    precip_now_img = np.array(precip_now_img)
    precip_now_img[precip_now_img[:,:,3] != 0] = intensity_to_color(INTENSITY_MIN)
    precip_now_img = Image.fromarray(precip_now_img)
    assert precip_now_img.mode == "RGBA"
    precip_combined_img = Image.new("RGBA", precip_now_img.size)
    precip_combined_img = Image.alpha_composite(precip_combined_img, precip_now_img)
    precip_combined_img = Image.alpha_composite(precip_combined_img, precip_forecast_img)

    assert map_img.size[0] / map_img.size[1] == precip_combined_img.size[0] / precip_combined_img.size[1]

    precip_combined_img = precip_combined_img.resize(map_img.size, resample=Image.BILINEAR)

    combined = map_img
    # combined = Image.alpha_composite(map_img, precip_combined_img)

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




def convert_to_bitmap(img):

    # Image to hold the quantize palette
    pal_img = Image.new("P", (1, 1))

    pal_img.putpalette(INKY_FRAME_PALETTE, rawmode="RGB")

    # draw a bar in the bottom right showing the colour intesity legend using intensity_to_color
    if add_legend := True:
        legend_width = 400
        legend_start_x = DESIRED_WIDTH - legend_width - 3
        legend_height = 16
        legend_start_y = DESIRED_HEIGHT - legend_height - 3
        for i in range(int(legend_width)):
            intensity = int((i / legend_width) * (INTENSITY_MAX - INTENSITY_MIN) + INTENSITY_MIN)
            color = intensity_to_color(intensity)
            if color[3] != 0:
                for y in range(int(legend_height)):
                    img.putpixel((int(legend_start_x + i), int(legend_start_y + y)), color)

    # Open the source image and quantize it to our palette
    quantized_img = img.convert("RGB").quantize(
        palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG
    )

    if add_qr_code := False:
        combined.paste(qr_img, (1, 52)) # near the top left corner

    if add_text := True:
        TEXT_HEIGHT = 16
        with open(IMAGE_INFO_FILE, "r") as f:
            lines = f.readlines()
            image_text = lines[1].strip().split("=")[1]


        # draw image_text in the lower-left corner with a semi-transparent background
        draw = ImageDraw.Draw(quantized_img)
        
        # https://www.dafont.com/minecraftia.font
        font = ImageFont.truetype("Minecraftia-Regular.ttf", TEXT_HEIGHT)

        padding = 8
        x = padding
        y = quantized_img.size[1] - TEXT_HEIGHT - padding

        print(f"Adding text to image: {image_text}")


        draw.text((x, y), image_text, font=font, fill=(0, 0, 0), stroke_width=3, stroke_fill=(0,0,0))
        draw.text((x, y), image_text, font=font, fill=(255, 255, 255))


    # so we can see it
    quantized_img.convert("RGB").save(COMBINED_FILE.with_name("quantized.png"))

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
        for i in range(10):
            if i == 10:
                deploy_dir = Path(f"publicly_available")
            else:
                deploy_dir = Path(f"publicly_available/{i}")
            deploy_dir.mkdir(exist_ok=True)
            shutil.copy(COMBINED_FILE, deploy_dir / COMBINED_FILE.name)
            shutil.copy(QUANTIZED_FILE, deploy_dir / QUANTIZED_FILE.name)
            shutil.copy(IMAGE_INFO_FILE, deploy_dir / IMAGE_INFO_FILE.name)
            print(f"Copied images to {deploy_dir}")

