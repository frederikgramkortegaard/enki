#!/bin/bash
set -e

# Paths
INPUT="sample-bmp-files-sample_640x426.bmp"
OUTDIR="tests/builtins"
MORPHIR="./morphir"
MORPHEVAL="./morpheval"

mkdir -p "$OUTDIR"

run_flow() {
  local name="$1"
  local morph_code="$2"
  echo "Testing $name..."
  echo "$morph_code" > "$OUTDIR/$name.morph"
  $MORPHIR -i "$OUTDIR/$name.morph" -o "$OUTDIR/$name.json"
  $MORPHEVAL "$OUTDIR/$name.json"
}

# Single builtins
run_flow "greyscale" "let image = open(\"$INPUT\")
let out = greyscale(image)
save(out, \"$OUTDIR/greyscale.bmp\")"
run_flow "invert" "let image = open(\"$INPUT\")
let out = invert(image)
save(out, \"$OUTDIR/invert.bmp\")"
run_flow "threshold" "let image = open(\"$INPUT\")
let out = threshold(image)
save(out, \"$OUTDIR/threshold.bmp\")"
run_flow "brightness" "let image = open(\"$INPUT\")
let out = brightness(image)
save(out, \"$OUTDIR/brightness.bmp\")"
run_flow "contrast" "let image = open(\"$INPUT\")
let out = contrast(image)
save(out, \"$OUTDIR/contrast.bmp\")"
run_flow "saturate" "let image = open(\"$INPUT\")
let out = saturate(image)
save(out, \"$OUTDIR/saturate.bmp\")"
run_flow "exposure" "let image = open(\"$INPUT\")
let out = exposure(image)
save(out, \"$OUTDIR/exposure.bmp\")"
run_flow "blur" "let image = open(\"$INPUT\")
let out = blur(image)
save(out, \"$OUTDIR/blur.bmp\")"
run_flow "sharpen" "let image = open(\"$INPUT\")
let out = sharpen(image)
save(out, \"$OUTDIR/sharpen.bmp\")"
run_flow "emboss" "let image = open(\"$INPUT\")
let out = emboss(image)
save(out, \"$OUTDIR/emboss.bmp\")"
run_flow "edge_detect" "let image = open(\"$INPUT\")
let out = edge_detect(image)
save(out, \"$OUTDIR/edge_detect.bmp\")"
run_flow "resize" "let image = open(\"$INPUT\")
let out = resize(image, 320)
save(out, \"$OUTDIR/resize.bmp\")"
run_flow "rotate" "let image = open(\"$INPUT\")
let out = rotate(image, 90)
save(out, \"$OUTDIR/rotate.bmp\")"
run_flow "flip" "let image = open(\"$INPUT\")
let out = flip(image)
save(out, \"$OUTDIR/flip.bmp\")"
run_flow "crop" "let image = open(\"$INPUT\")
let out = crop(image, 0, 0, 100, 100)
save(out, \"$OUTDIR/crop.bmp\")"

# Combined flows
run_flow "greyscale_invert" "let image = open(\"$INPUT\")
let grey = greyscale(image)
let out = invert(grey)
save(out, \"$OUTDIR/greyscale_invert.bmp\")"
run_flow "emboss_invert" "let image = open(\"$INPUT\")
let emb = emboss(image)
let out = invert(emb)
save(out, \"$OUTDIR/emboss_invert.bmp\")"
run_flow "blur_sharpen" "let image = open(\"$INPUT\")
let blur_img = blur(image)
let out = sharpen(blur_img)
save(out, \"$OUTDIR/blur_sharpen.bmp\")"
run_flow "greyscale_invert_contrast" "let image = open(\"$INPUT\")
let grey = greyscale(image)
let inv = invert(grey)
let out = contrast(inv)
save(out, \"$OUTDIR/greyscale_invert_contrast.bmp\")"
run_flow "blur_brightness" "let image = open(\"$INPUT\")
let blur_img = blur(image)
let out = brightness(blur_img)
save(out, \"$OUTDIR/blur_brightness.bmp\")"
run_flow "sharpen_saturate" "let image = open(\"$INPUT\")
let sharp = sharpen(image)
let out = saturate(sharp)
save(out, \"$OUTDIR/sharpen_saturate.bmp\")"
run_flow "greyscale_blur_edge" "let image = open(\"$INPUT\")
let grey = greyscale(image)
let blur_img = blur(grey)
let out = edge_detect(blur_img)
save(out, \"$OUTDIR/greyscale_blur_edge.bmp\")"
run_flow "invert_exposure_emboss" "let image = open(\"$INPUT\")
let inv = invert(image)
let exp = exposure(inv)
let out = emboss(exp)
save(out, \"$OUTDIR/invert_exposure_emboss.bmp\")"
run_flow "resize_rotate_flip" "let image = open(\"$INPUT\")
let rsz = resize(image, 320)
let rot = rotate(rsz, 90)
let out = flip(rot)
save(out, \"$OUTDIR/resize_rotate_flip.bmp\")"
run_flow "crop_blur_sharpen" "let image = open(\"$INPUT\")
let crp = crop(image, 0, 0, 100, 100)
let blur_img = blur(crp)
let out = sharpen(blur_img)
save(out, \"$OUTDIR/crop_blur_sharpen.bmp\")"
run_flow "greyscale_emboss_edge" "let image = open(\"$INPUT\")
let grey = greyscale(image)
let emb = emboss(grey)
let out = edge_detect(emb)
save(out, \"$OUTDIR/greyscale_emboss_edge.bmp\")"
run_flow "flip_brightness_contrast" "let image = open(\"$INPUT\")
let flp = flip(image)
let bright = brightness(flp)
let out = contrast(bright)
save(out, \"$OUTDIR/flip_brightness_contrast.bmp\")"

# Add more combinations as desired

echo "All builtins and combinations tested. Output images are in $OUTDIR"
