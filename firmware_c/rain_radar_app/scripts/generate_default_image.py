#!/usr/bin/env python3
"""
Convert an image into a C++ header containing pre-quantized raw framebuffer bytes
for the Pimoroni Inky Frame 7.3" (Pico W and Pico 2 W) and export preview PNG images
to the build directory.
"""

import sys
from pathlib import Path
from PIL import Image

TARGET_WIDTH = 800
TARGET_HEIGHT = 480
TARGET_SIZE = TARGET_WIDTH * TARGET_HEIGHT

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)
RED = (255, 0, 0)
YELLOW = (255, 255, 0)
ORANGE = (255, 140, 0)

# ACeP 7-color palette for Pico W (RP2040)
PALETTE_PICO_W = (*BLACK, *WHITE, *GREEN, *BLUE, *RED, *YELLOW, *ORANGE)

# Spectra 6-color palette for Pico 2 W (RP2350)
PALETTE_PICO2_W = (*BLACK, *BLACK, *YELLOW, *RED, *WHITE, *BLUE, *GREEN)


def process_image(input_path: Path, output_header_path: Path):
    print(f"Loading image: {input_path}")
    img = Image.open(input_path)

    # Resize & center crop to 800x480
    img_ratio = img.width / img.height
    target_ratio = TARGET_WIDTH / TARGET_HEIGHT

    if img_ratio > target_ratio:
        new_h = TARGET_HEIGHT
        new_w = int(TARGET_HEIGHT * img_ratio)
    else:
        new_w = TARGET_WIDTH
        new_h = int(TARGET_WIDTH / img_ratio)

    img_resized = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    left = (new_w - TARGET_WIDTH) // 2
    top = (new_h - TARGET_HEIGHT) // 2
    img_cropped = img_resized.crop((left, top, left + TARGET_WIDTH, top + TARGET_HEIGHT)).convert("RGB")

    def quantize_image(palette_tuple):
        pal_img = Image.new("P", (1, 1))
        pal_img.putpalette(palette_tuple, rawmode="RGB")
        quant = img_cropped.quantize(palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG)
        return quant

    print("Quantizing for Pico W (ACeP 7-color)...")
    quant_pico_w = quantize_image(PALETTE_PICO_W)
    bytes_pico_w = quant_pico_w.tobytes()

    print("Quantizing for Pico 2 W (Spectra 6-color)...")
    quant_pico2_w = quantize_image(PALETTE_PICO2_W)
    bytes_pico2_w = quant_pico2_w.tobytes()

    assert len(bytes_pico_w) == TARGET_SIZE
    assert len(bytes_pico2_w) == TARGET_SIZE

    # Write C++ header
    print(f"\nWriting header: {output_header_path}")
    with open(output_header_path, "w") as f:
        f.write("// Auto-generated default background image\n")
        f.write("#pragma once\n\n")
        f.write("#include <cstdint>\n")
        f.write("#include <cstddef>\n")
        f.write('#include "pico/platform.h"\n\n')
        f.write(f"constexpr size_t DEFAULT_IMAGE_SIZE = {TARGET_SIZE};\n\n")

        f.write("#if PICO_RP2350\n")
        f.write("// Pre-quantized for Pico 2 W (Spectra palette)\n")
        f.write(f"const uint8_t DEFAULT_IMAGE_DATA[{TARGET_SIZE}] = {{\n")
        for i in range(0, len(bytes_pico2_w), 32):
            chunk = bytes_pico2_w[i : i + 32]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
        f.write("};\n")

        f.write("#elif PICO_RP2040\n")
        f.write("// Pre-quantized for Pico W (ACeP palette)\n")
        f.write(f"const uint8_t DEFAULT_IMAGE_DATA[{TARGET_SIZE}] = {{\n")
        for i in range(0, len(bytes_pico_w), 32):
            chunk = bytes_pico_w[i : i + 32]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
        f.write("};\n")
        f.write("#endif\n")

    # Save preview PNG images to build directories
    app_dir = output_header_path.parent
    build_dir = app_dir / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    preview_w = build_dir / "default_image_pico_w.png"
    preview_2w = build_dir / "default_image_pico2_w.png"

    quant_pico_w.convert("RGB").save(preview_w)
    quant_pico2_w.convert("RGB").save(preview_2w)

    # Also save inside build_pico_w / build_pico2_w if they exist
    build_pico_w_dir = app_dir / "build_pico_w"
    if build_pico_w_dir.is_dir():
        quant_pico_w.convert("RGB").save(build_pico_w_dir / "default_image_preview.png")

    build_pico2_w_dir = app_dir / "build_pico2_w"
    if build_pico2_w_dir.is_dir():
        quant_pico2_w.convert("RGB").save(build_pico2_w_dir / "default_image_preview.png")

    print("\nSaved preview images:")
    print(f"  Pico W   (ACeP 7-color):    {preview_w.resolve()}")
    print(f"  Pico 2 W (Spectra 6-color): {preview_2w.resolve()}")
    if build_pico_w_dir.is_dir():
        print(f"  Pico W Build Dir:           {(build_pico_w_dir / 'default_image_preview.png').resolve()}")
    if build_pico2_w_dir.is_dir():
        print(f"  Pico 2 W Build Dir:         {(build_pico2_w_dir / 'default_image_preview.png').resolve()}")
    print("\nDone!")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        inp = Path(sys.argv[1]).expanduser()
    else:
        inp = Path.home() / "Pictures" / "webb-cats-paw-nebula.webp"

    outp = Path(__file__).resolve().parent.parent / "default_image.hpp"
    process_image(inp, outp)
