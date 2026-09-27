"""Draws the labelled station pictures used in the README.

    pip install pillow
    python tools/readme_images/annotate.py

Reads the CAD renders in Assembling/img/ and writes docs/station-front.jpg and
docs/station-back.jpg. To move or rename a label, edit FRONT or BACK below and
run the script again.

Each label is (side, label_y, x, y, title, note):
  side      'L' or 'R', which margin the text goes in
  label_y   height of the title line in that margin
  x, y      the point on the part, in pixels of the original render
  title     bold name of the part
  note      grey text under it; '\n' starts a new line

Labels are numbered in the order they are listed.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]

ACCENT = (214, 92, 40)
INK = (34, 38, 44)
GREY = (96, 104, 112)
LEADER = (60, 66, 74)

TOP = 60     # room for the "Front" / "Back" heading
BOTTOM = 40

FRONT = [
    ('L', 190, 205, 300, 'Reagent syringe', '10 ml, holds the HCl'),
    ('L', 420, 75, 430, 'Stepper motor', 'one per pump'),
    ('L', 640, 170, 760, 'Reagent pump', 'lead screw pushes\nthe plunger'),
    ('L', 860, 380, 775, 'Electrode holder', 'pH probe + tubes\ninto the reactor'),
    ('L', 1060, 360, 1040, 'Reactor', 'sample cup, magnetic\nstirrer underneath'),
    ('R', 120, 640, 200, 'Water syringe', 'draws the tank sample'),
    ('R', 330, 410, 432, 'Tube ports', 'Acid · W out · W in'),
    ('R', 560, 470, 600, 'Valves', 'two servo-driven\nrotary taps'),
    ('R', 800, 640, 820, 'Water pump', 'same design as the\nreagent pump'),
    ('R', 1040, 560, 1050, 'Drain pump', 'empties the reactor\nbetween runs'),
]

BACK = [
    ('L', 160, 150, 170, 'Syringe clamps', 'hold the syringe barrels'),
    ('L', 420, 200, 420, 'Optical endstop', 'tells the pump it is home.\nKeep out of direct sun'),
    ('R', 420, 720, 345, 'Optical endstop', 'second pump'),
    ('R', 640, 530, 560, 'Electronics box', 'main board (ESP32),\nvented cover'),
]


def font(name, size):
    # Pillow looks in the system font folders by file name. DejaVu ships with
    # most Linux systems; on Windows or macOS install it or change the name.
    try:
        return ImageFont.truetype(name, size)
    except OSError:
        raise SystemExit(f'Font {name} not found. Install DejaVu fonts or edit font() in {__file__}')


TITLE = font('DejaVuSans-Bold.ttf', 26)
NOTE = font('DejaVuSans.ttf', 20)
NUMBER = font('DejaVuSans-Bold.ttf', 20)


def build(src, out, heading, labels, left, right):
    render = Image.open(ROOT / src).convert('RGB')
    width = render.width + left + right
    canvas = Image.new('RGB', (width, render.height + TOP + BOTTOM), 'white')
    canvas.paste(render, (left, TOP))
    d = ImageDraw.Draw(canvas)
    d.text((width // 2, 24), heading, font=TITLE, fill=INK, anchor='mt')

    for n, (side, label_y, x, y, title, note) in enumerate(labels, 1):
        x += left
        y += TOP
        label_y += TOP
        if side == 'L':
            text_x = 24
            leader_x = left - 14
        else:
            text_x = left + render.width + 24
            leader_x = text_x - 10

        d.text((text_x + 36, label_y), title, font=TITLE, fill=INK, anchor='ls')
        for k, line in enumerate(note.split('\n')):
            d.text((text_x + 36, label_y + 26 + k * 24), line, font=NOTE, fill=GREY, anchor='ls')

        # numbered badge; two digits need a slightly bigger circle
        cx, cy = text_x + 14, label_y - 9
        r = 14 if n < 10 else 18
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=ACCENT)
        d.text((cx, cy), str(n), font=NUMBER, fill='white', anchor='mm')

        # leader line with a white halo so it stays visible over dark parts
        d.line([(leader_x, cy), (x, y)], fill='white', width=6)
        d.line([(leader_x, cy), (x, y)], fill=LEADER, width=2)
        d.ellipse((x - 8, y - 8, x + 8, y + 8), fill=ACCENT, outline='white', width=3)

    canvas.save(ROOT / out, quality=90, optimize=True)
    print('wrote', out)


if __name__ == '__main__':
    build('Assembling/img/front1.jpg', 'docs/station-front.jpg', 'Front', FRONT, 330, 330)
    build('Assembling/img/back1.jpg', 'docs/station-back.jpg', 'Back', BACK, 340, 330)
