import os
import requests
import requests
from pathlib import Path

# if token := os.environ.get("RAINBOW_API_TOKEN"):
#     RAINBOW_API_TOKEN = token
from PIL import Image

from api_secrets import RAINBOW_API_TOKEN


PRECIP_TILE_FILE = Path("forecast.png")
MAP_TILE_FILE = Path("map.png")
COMBINED_FILE = Path("combined.jpg")


def get_snapshot_timestamp():
    response = requests.get(f"https://api.rainbow.ai/tiles/v1/snapshot?token={RAINBOW_API_TOKEN}")
    return response.json()["snapshot"]


def get_tile_handler(snapshot_timestamp: int, forecast_time: int, zoom: int, x: int, y: int):
    url = f"https://api.rainbow.ai/tiles/v1/precip/{snapshot_timestamp}/{forecast_time}/{zoom}/{x}/{y}?token={RAINBOW_API_TOKEN}&color=8"
    response = requests.get(url, stream=True, timeout=10)
    return response

ZOOM = 5
TILE_X = 15
TILE_Y = 9

def download_precip_image():
    print("Downloading forecast image...")
    ts = get_snapshot_timestamp()
    print(f"Snapshot timestamp: {ts}")
    response = get_tile_handler(ts, 600,ZOOM,TILE_X,TILE_Y)
    if response.status_code == 200:
        with open(PRECIP_TILE_FILE, "wb") as f:
            f.write(response.content)
        print("Image updated.")
    else:
        print(f"Failed to fetch: {response.status_code}")

def download_map_image(save_path: Path):
    url = f"https://tile.openstreetmap.org/{ZOOM}/{TILE_X}/{TILE_Y}.png"
    headers = {"User-Agent": "TileFetcher/1.0 (your.email@example.com)"}
    r = requests.get(url, headers=headers, timeout=10)

    if r.status_code != 200:
        raise RuntimeError(f"Failed to fetch tile: {r.status_code}")

    if save_path:
        with open(save_path, "wb") as f:
            f.write(r.content)

    return r.content

def build_image():
    download_map_image(MAP_TILE_FILE)
    download_precip_image()


    desired_width = 800
    desired_height = 480

    with Image.open(MAP_TILE_FILE).convert("RGBA") as map_img, Image.open(PRECIP_TILE_FILE).convert("RGBA") as precip_img:
        if map_img.size != precip_img.size:
            precip_img = precip_img.resize(map_img.size, resample=Image.BILINEAR)
        combined = Image.alpha_composite(map_img, precip_img)
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
        rgb_image.save(COMBINED_FILE)
        print("Combined map.png and forecast.png into one image.")

if __name__ == "__main__":
    build_image()
