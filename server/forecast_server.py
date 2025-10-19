import os
import requests
import requests
import time
import threading
from flask import Flask, send_file
from pathlib import Path

if token := os.environ.get("RAINBOW_API_TOKEN"):
    RAINBOW_API_TOKEN = token
from PIL import Image


PRECIP_TILE_FILE = Path("forecast.png")
MAP_TILE_FILE = Path("map.png")
COMBINED_FILE = Path("combined.png")


app = Flask(__name__)

def get_snapshot_timestamp():
    response = requests.get(f"https://api.rainbow.ai/tiles/v1/snapshot?token={RAINBOW_API_TOKEN}")
    return response.json()["snapshot"]


def get_tile_handler(snapshot_timestamp: int, forecast_time: int, zoom: int, x: int, y: int):
    url = f"https://api.rainbow.ai/tiles/v1/precip/{snapshot_timestamp}/{forecast_time}/{zoom}/{x}/{y}?token={RAINBOW_API_TOKEN}"
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

def download_images_continually():
    download_map_image(MAP_TILE_FILE)
    while True:
        try:
            download_precip_image()
        except Exception as e:
            print("Error downloading:", e)

        with Image.open(MAP_TILE_FILE).convert("RGBA") as map_img, Image.open(PRECIP_TILE_FILE).convert("RGBA") as precip_img:
            if map_img.size != precip_img.size:
                precip_img = precip_img.resize(map_img.size, resample=Image.BILINEAR)
            combined = Image.alpha_composite(map_img, precip_img)
            combined.save(COMBINED_FILE)
            print("Combined map.png and forecast.png into one image.")


        UPDATE_INTERVAL = 3600  # every hour
        time.sleep(UPDATE_INTERVAL)

@app.route(f"/{COMBINED_FILE}")
def serve_forecast():
    return send_file(COMBINED_FILE, mimetype="image/png")

c

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8080))

    threading.Thread(target=download_images_continually, daemon=True).start()
    app.run(host="0.0.0.0", port=port)
