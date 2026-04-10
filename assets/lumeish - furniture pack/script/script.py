"""
Spritesheet Packer
------------------
Place this script in a directory with your images and run it.
Each image is packed into a 16x16-cell grid, occupying as many
cells as needed (ceiling of width/16 × ceiling of height/16),
centered within that cell block.

Output: spritesheet.png  (and optionally spritesheet.json for slice data)
"""

import os
import math
import json
from PIL import Image

# ── Config ────────────────────────────────────────────────────────────────────
CELL = 16                          # cell size in pixels
SHEET_COLS = 16                    # how many cells wide the output sheet is
OUTPUT_IMAGE = "spritesheet.png"
OUTPUT_JSON  = "spritesheet.json"  # set to None to skip JSON output
EXTENSIONS   = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}
# ─────────────────────────────────────────────────────────────────────────────


def cells_needed(px: int) -> int:
    """How many 16px cells does a dimension of `px` pixels require?"""
    return max(1, math.ceil(px / CELL))


def pack(images: list[tuple[str, Image.Image]]) -> tuple[Image.Image, list[dict]]:
    """
    Greedy row-based bin packer.
    Returns (sheet_image, metadata_list).
    metadata entry keys: name, x, y, w, h, cell_x, cell_y, cell_w, cell_h
    """
    # Sort largest-footprint first for better packing
    images.sort(key=lambda t: -(cells_needed(t[1].width) * cells_needed(t[1].height)))

    # Grid: track which cells are occupied
    grid: set[tuple[int, int]] = set()
    placements: list[dict] = []

    def fits(cx: int, cy: int, cw: int, ch: int) -> bool:
        for dy in range(ch):
            for dx in range(cw):
                if (cx + dx, cy + dy) in grid:
                    return False
        return True

    def place(cx: int, cy: int, cw: int, ch: int):
        for dy in range(ch):
            for dx in range(cw):
                grid.add((cx + dx, cy + dy))

    max_cy = 0

    for name, img in images:
        cw = cells_needed(img.width)
        ch = cells_needed(img.height)

        # Find the first cell (left→right, top→bottom) where this sprite fits
        placed = False
        cy = 0
        while not placed:
            for cx in range(SHEET_COLS - cw + 1):
                if fits(cx, cy, cw, ch):
                    place(cx, cy, cw, ch)

                    # Pixel position of the cell block's top-left corner
                    block_px = cx * CELL
                    block_py = cy * CELL
                    block_pw = cw * CELL
                    block_ph = ch * CELL

                    # Center the image within the cell block
                    off_x = (block_pw - img.width)  // 2
                    off_y = (block_ph - img.height) // 2

                    placements.append({
                        "name":   name,
                        # pixel coords of the image inside the sheet
                        "x":      block_px + off_x,
                        "y":      block_py + off_y,
                        "w":      img.width,
                        "h":      img.height,
                        # cell-grid coords (useful for game engines)
                        "cell_x": cx,
                        "cell_y": cy,
                        "cell_w": cw,
                        "cell_h": ch,
                    })
                    max_cy = max(max_cy, cy + ch)
                    placed = True
                    break
            if not placed:
                cy += 1   # try next row

    # Build output sheet (height = however many rows we needed)
    sheet_h = max_cy * CELL
    sheet_w = SHEET_COLS * CELL
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    img_map = {name: img for name, img in images}
    for p in placements:
        sheet.paste(img_map[p["name"]].convert("RGBA"), (p["x"], p["y"]))

    return sheet, placements


def main():
    script_name = os.path.basename(__file__)
    script_dir  = os.path.dirname(os.path.abspath(__file__))

    # Gather images from the same directory as this script
    image_files = [
        f for f in os.listdir(script_dir)
        if os.path.splitext(f)[1].lower() in EXTENSIONS
        and f != OUTPUT_IMAGE
        and f != script_name
    ]

    if not image_files:
        print("No images found. Place image files alongside this script.")
        return

    print(f"Found {len(image_files)} image(s). Packing…")

    images: list[tuple[str, Image.Image]] = []
    for fname in image_files:
        path = os.path.join(script_dir, fname)
        try:
            img = Image.open(path)
            img.load()
            images.append((os.path.splitext(fname)[0], img))
        except Exception as e:
            print(f"  ⚠  Skipping {fname}: {e}")

    if not images:
        print("No valid images could be loaded.")
        return

    sheet, placements = pack(images)

    out_img_path = os.path.join(script_dir, OUTPUT_IMAGE)
    sheet.save(out_img_path)
    print(f"✓ Spritesheet saved → {OUTPUT_IMAGE}  ({sheet.width}×{sheet.height} px)")

    if OUTPUT_JSON:
        out_json_path = os.path.join(script_dir, OUTPUT_JSON)
        with open(out_json_path, "w") as f:
            json.dump({
                "cell_size": CELL,
                "sheet_width":  sheet.width,
                "sheet_height": sheet.height,
                "sprites": placements,
            }, f, indent=2)
        print(f"✓ Metadata saved    → {OUTPUT_JSON}")

    print("\nSprite layout:")
    print(f"  {'Name':<30} {'Cells':>12}  {'Pixel origin'}")
    print("  " + "─" * 58)
    for p in sorted(placements, key=lambda p: (p["cell_y"], p["cell_x"])):
        cell_str  = f"({p['cell_x']},{p['cell_y']}) {p['cell_w']}×{p['cell_h']}"
        pixel_str = f"@ ({p['x']},{p['y']})  {p['w']}×{p['h']}px"
        print(f"  {p['name']:<30} {cell_str:>12}  {pixel_str}")


if __name__ == "__main__":
    main()