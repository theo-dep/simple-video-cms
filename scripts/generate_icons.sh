#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
ICONS_DIR="$ROOT/front/assets/icons"
SVG="$ROOT/front/assets/img/icon.svg"

# Site background color (cf. --color-bg-dark in front/assets/components/_base.css)
BG_COLOR="#000000"

# Maskable icon safe zone: the logo takes up 55% of the canvas
SAFE_ZONE=55

# Apple icon safe zone: iOS masks the icon with a rounded rectangle, so keep the
# logo inside 80% of the canvas to survive the corner rounding
APPLE_SAFE_ZONE=80

# Vertical shift of the maskable logo toward the top, as a % of the canvas
# (0 = centered)
VERTICAL_OFFSET=2

MASKABLE_SIZES=(48 72 96 128 192 384 512 1024)
APPLE_SIZES=(60 76 120 152 167 180)
APPLE_ICON_SIZE=192
STARTUP_SIZE=1024

if [[ ! -f "$SVG" ]]; then
    echo "error: source SVG not found: $SVG" >&2
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "==> Cleaning $ICONS_DIR"
rm -f "$ICONS_DIR"/*.png

echo "==> Rendering SVG (transparent background)"
inkscape "$SVG" \
    --export-type=png \
    --export-filename="$WORK/logo.png" \
    --export-width=1024

# Logo centered on a transparent square: source for non-maskable icons
convert "$WORK/logo.png" \
    -gravity center -background transparent \
    -extent 1024x1024 \
    "$WORK/src.png"

# Logo shrunk into the Apple safe zone, centered over the baked site
# background: source for apple-touch icons
convert "$WORK/src.png" -resize $((1024 * APPLE_SAFE_ZONE / 100))x$((1024 * APPLE_SAFE_ZONE / 100)) \
    "$WORK/logo_apple.png"
convert -size 1024x1024 "xc:$BG_COLOR" \
    \( "$WORK/logo_apple.png" \) \
    -gravity center -composite \
    "$WORK/apple.png"

# Logo shrunk into the safe zone, shifted upward over the site background:
# source for maskable icons
convert "$WORK/src.png" -resize $((1024 * SAFE_ZONE / 100))x$((1024 * SAFE_ZONE / 100)) \
    "$WORK/logo_safe.png"
convert -size 1024x1024 "xc:$BG_COLOR" \
    \( "$WORK/logo_safe.png" \) \
    -gravity center -geometry +0-$((1024 * VERTICAL_OFFSET / 100)) \
    -composite \
    "$WORK/maskable.png"

echo "==> Maskable icons (background $BG_COLOR, ${SAFE_ZONE}% safe zone)"
for size in "${MASKABLE_SIZES[@]}"; do
    if [[ "$size" -eq 1024 ]]; then
        out="$ICONS_DIR/maskable_icon.png"
    else
        out="$ICONS_DIR/maskable_icon_x${size}.png"
    fi
    convert "$WORK/maskable.png" -resize ${size}x${size} "$out"
done

echo "==> Apple icons (background $BG_COLOR, ${APPLE_SAFE_ZONE}% safe zone)"
for size in "${APPLE_SIZES[@]}"; do
    convert "$WORK/apple.png" -resize ${size}x${size} "$ICONS_DIR/apple-touch-icon-x${size}.png"
done
convert "$WORK/apple.png" -resize ${APPLE_ICON_SIZE}x${APPLE_ICON_SIZE} "$ICONS_DIR/apple-touch-icon.png"

echo "==> Startup image (transparent background)"
convert "$WORK/src.png" -resize ${STARTUP_SIZE}x${STARTUP_SIZE} "$ICONS_DIR/apple-touch-startup-image.png"

echo "==> icon.png (transparent background)"
cp "$WORK/src.png" "$ICONS_DIR/icon.png"

echo "==> preview.png (social preview image, background $BG_COLOR)"
convert -size 1024x1024 "xc:$BG_COLOR" \
    \( "$WORK/src.png" \) \
    -gravity center -composite \
    "$ICONS_DIR/preview.png"

echo ""
echo "OK: icons regenerated in $ICONS_DIR"

