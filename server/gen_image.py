import os
import requests
import requests
from pathlib import Path

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
    qr.add_data("https://weather.metoffice.gov.uk/maps-and-charts/rainfall-radar-forecast-map#?model=ukmo-ukv&layer=rainfall-rate&bbox=[[50.75904732375726,-2.4554443359375004],[52.22948173332481,2.2906494140625004]]")
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    img.save(QRCODE_FILE)
    print("Saved QR code image.")

def build_image():
    download_map_image()
    precip_ts = download_precip_image()
    info_path = COMBINED_FILE.parent / "image_info.json"
    with open(info_path, "w") as f:
        json.dump({
            "precip_ts": precip_ts,
            "text": dt.datetime.fromtimestamp(precip_ts).strftime("%Y-%m-%d %H:%M:%S") + " + "+ f"{FORECAST_SECS//60} mins forecast",
        }, f)
    qr_code_image()

    desired_width = 800
    desired_height = 480

    with Image.open(MAP_TILE_FILE).convert("RGBA") as map_img, Image.open(PRECIP_TILE_FILE).convert("RGBA") as precip_img, Image.open(QRCODE_FILE).convert("RGBA") as qr_img:
        if map_img.size != precip_img.size:
            precip_img = precip_img.resize(map_img.size, resample=Image.BILINEAR)
        combined = Image.alpha_composite(map_img, precip_img)
        combined.paste(qr_img, (1,52))
        rgb_image = combined.convert("RGB")
        current_width, current_height = rgb_image.size
        if current_width / current_height > desired_width / desired_height:
            # too wide
            cropped_width = current_height * desired_width / desired_height
            assert cropped_width <= current_width
            cropped_width_start = (current_width - cropped_width) / 2
            bounding_box = (cropped_width_start, 0, cropped_width+cropped_width_start, current_height)
        else:
            # too tall
            cropped_height = current_width * desired_height / desired_width
            assert cropped_height <= cropped_height
            cropped_height_start = (current_height - cropped_height) / 2
            bounding_box = (0, cropped_height_start, current_width, cropped_height+cropped_height_start)

        rgb_image = rgb_image.resize((desired_width,desired_height), box=bounding_box, resample=Image.BILINEAR)
        rgb_image = ImageEnhance.Color(rgb_image).enhance(1.3)
        rgb_image.save(COMBINED_FILE, progressive=False, quality=85)
        print("Combined map.png and forecast.png into one image.")

if __name__ == "__main__":
    build_image()
