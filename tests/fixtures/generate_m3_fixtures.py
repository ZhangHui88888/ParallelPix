from pathlib import Path

from PIL import Image


FIXTURE_ROOT = Path(__file__).resolve().parent
IMAGE_DIR = FIXTURE_ROOT / "images"
WATERMARK_DIR = FIXTURE_ROOT / "watermarks"


def generate_images() -> None:
    IMAGE_DIR.mkdir(parents=True, exist_ok=True)

    color = Image.new("RGB", (3, 2))
    color.putdata(
        [
            (255, 0, 0),
            (0, 255, 0),
            (0, 0, 255),
            (255, 255, 255),
            (0, 0, 0),
            (12, 34, 56),
        ]
    )
    color.save(IMAGE_DIR / "color_3x2.png", format="PNG", compress_level=9)

    photo = Image.new("RGB", (4, 3))
    photo.putdata(
        [
            ((x * 50) % 256, (y * 80) % 256, ((x + y) * 35) % 256)
            for y in range(3)
            for x in range(4)
        ]
    )
    photo.save(
        IMAGE_DIR / "photo_4x3.jpg",
        format="JPEG",
        quality=90,
        optimize=False,
        progressive=False,
        subsampling=0,
    )

    grayscale = Image.new("L", (2, 2))
    grayscale.putdata([0, 64, 128, 255])
    grayscale.save(IMAGE_DIR / "grayscale_2x2.png", format="PNG", compress_level=9)

    unicode_image = Image.new("RGB", (1, 2))
    unicode_image.putdata([(9, 8, 7), (6, 5, 4)])
    unicode_image.save(IMAGE_DIR / "中文图片.png", format="PNG", compress_level=9)

    (IMAGE_DIR / "corrupt.png").write_bytes(b"not a png image\n")


def generate_watermarks() -> None:
    WATERMARK_DIR.mkdir(parents=True, exist_ok=True)

    rgba = Image.new("RGBA", (2, 2))
    rgba.putdata(
        [
            (255, 0, 0, 0),
            (0, 255, 0, 64),
            (0, 0, 255, 128),
            (12, 34, 56, 255),
        ]
    )
    rgba.save(WATERMARK_DIR / "rgba_2x2.png", format="PNG", compress_level=9)

    rgb = Image.new("RGB", (2, 1))
    rgb.putdata([(10, 20, 30), (40, 50, 60)])
    rgb.save(WATERMARK_DIR / "rgb_2x1.png", format="PNG", compress_level=9)

    grayscale = Image.new("L", (1, 2))
    grayscale.putdata([25, 200])
    grayscale.save(
        WATERMARK_DIR / "grayscale_1x2.png",
        format="PNG",
        compress_level=9,
    )

    (WATERMARK_DIR / "corrupt.png").write_bytes(b"not a watermark\n")


if __name__ == "__main__":
    generate_images()
    generate_watermarks()
