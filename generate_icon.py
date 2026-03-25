"""Generate app.ico for Screen Rotator — a circular rotation arrow on a blue background."""

import math
from PIL import Image, ImageDraw

SIZE = 256
CENTER = SIZE // 2
BG_COLOR = (26, 115, 232)     # Google blue
ARROW_COLOR = (255, 255, 255) # White


def draw_icon(size):
    """Draw the rotation arrow icon at a given size."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    margin = size * 0.06
    radius = size * 0.18

    # Rounded rectangle background
    draw.rounded_rectangle(
        [margin, margin, size - margin, size - margin],
        radius=radius,
        fill=BG_COLOR,
    )

    # Circular arrow parameters
    cx, cy = size / 2, size / 2
    r = size * 0.30            # arc radius
    thickness = max(size * 0.09, 2)

    # Draw arc (270 degrees, leaving a gap for the arrowhead)
    arc_start = -30   # degrees
    arc_end = 250     # degrees — leaves a 80° gap
    bbox = [cx - r, cy - r, cx + r, cy + r]
    draw.arc(bbox, arc_start, arc_end, fill=ARROW_COLOR, width=max(int(thickness), 1))

    # Arrowhead at the start of the arc (pointing clockwise)
    arrow_angle = math.radians(-30)  # matches arc_start
    ax = cx + r * math.cos(arrow_angle)
    ay = cy + r * math.sin(arrow_angle)

    arrow_size = size * 0.12
    # Arrow points in the tangent direction (clockwise = perpendicular to radius, pointing "forward")
    tangent_angle = arrow_angle + math.pi / 2  # 90° ahead of radius = tangent direction

    # Three points of the arrowhead triangle
    tip_x = ax + arrow_size * math.cos(tangent_angle)
    tip_y = ay + arrow_size * math.sin(tangent_angle)

    base_angle1 = tangent_angle + math.radians(140)
    base_angle2 = tangent_angle - math.radians(140)

    b1x = ax + arrow_size * 0.8 * math.cos(base_angle1)
    b1y = ay + arrow_size * 0.8 * math.sin(base_angle1)
    b2x = ax + arrow_size * 0.8 * math.cos(base_angle2)
    b2y = ay + arrow_size * 0.8 * math.sin(base_angle2)

    draw.polygon([(tip_x, tip_y), (b1x, b1y), (b2x, b2y)], fill=ARROW_COLOR)

    # Second arrowhead at the end of the arc (pointing counter-clockwise, for "rotation" look)
    arrow_angle2 = math.radians(250)  # matches arc_end
    ax2 = cx + r * math.cos(arrow_angle2)
    ay2 = cy + r * math.sin(arrow_angle2)

    tangent_angle2 = arrow_angle2 - math.pi / 2  # opposite tangent

    tip2_x = ax2 + arrow_size * math.cos(tangent_angle2)
    tip2_y = ay2 + arrow_size * math.sin(tangent_angle2)

    base_angle2a = tangent_angle2 + math.radians(140)
    base_angle2b = tangent_angle2 - math.radians(140)

    b2ax = ax2 + arrow_size * 0.8 * math.cos(base_angle2a)
    b2ay = ay2 + arrow_size * 0.8 * math.sin(base_angle2a)
    b2bx = ax2 + arrow_size * 0.8 * math.cos(base_angle2b)
    b2by = ay2 + arrow_size * 0.8 * math.sin(base_angle2b)

    draw.polygon([(tip2_x, tip2_y), (b2ax, b2ay), (b2bx, b2by)], fill=ARROW_COLOR)

    return img


def main():
    sizes = [16, 32, 48, 256]
    images = []

    for s in sizes:
        # Draw at 4x for quality, then resize down
        render_size = max(s * 4, 256)
        img = draw_icon(render_size)
        if render_size != s:
            img = img.resize((s, s), Image.LANCZOS)
        images.append(img)

    # Save as multi-resolution .ico (first image is the "main" one)
    images[-1].save(
        "app.ico",
        format="ICO",
        sizes=[(s, s) for s in sizes],
        append_images=images[:-1],
    )
    print(f"Created app.ico with sizes: {sizes}")


if __name__ == "__main__":
    main()
