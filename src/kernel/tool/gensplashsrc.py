#!/usr/bin/python3

from PIL import Image

def convert_splash_to_c(image_path, output_path):
    try:
        img = Image.open(image_path).convert("RGBA")
    except FileNotFoundError:
        print(f"Error: Could not find {image_path}")
        return

    width, height = img.size

    # Ensure it matches the expected dimensions
    if width != 640 or height != 480:
        print(f"Notice: Resizing image from {width}x{height} to 640x480...")
        img = img.resize((640, 480))
        width, height = 640, 480

    pixels = list(img.getdata())

    print(f"Generating C array for {width}x{height} image...")

    with open(output_path, "w") as f:
        f.write('/* almond image haha funni */\n')
        f.write('#include "kernel_types.h"\n\n')
        f.write(f'const uint32_t bootsplash_img[{width} * {height}] = {{\n    ')

        for i, (r, g, b, a) in enumerate(pixels):
            # Pack RGBA into a single 32-bit integer (0xRRGGBBAA)
            val = (r << 24) | (g << 16) | (b << 8) | a

            f.write(f'0x{val:08X}, ')

            # Format nicely with line breaks every 8 pixels
            if (i + 1) % 8 == 0:
                f.write('\n    ')

        f.write('\n};\n')

    print(f"Successfully generated {output_path}!")

if __name__ == "__main__":
    convert_splash_to_c("splash.png", "bootsplash.c")
