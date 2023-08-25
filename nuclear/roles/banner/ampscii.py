#!/usr/bin/env python3

import math
import sys

from PIL import Image

# Make a lookup map we can use to find values
colour_map = [
    # Main available colours
    (0x00, 0x00, 0x00, 0),
    (0x80, 0x00, 0x00, 1),
    (0x00, 0x80, 0x00, 2),
    (0x80, 0x80, 0x00, 3),
    (0x00, 0x00, 0x80, 4),
    (0x80, 0x00, 0x80, 5),
    (0x00, 0x80, 0x80, 6),
    (0xC0, 0xC0, 0xC0, 7),
    # "Bright" versions of the main colours
    (0x80, 0x80, 0x80, 8),
    (0xFF, 0x00, 0x00, 9),
    (0x00, 0xFF, 0x00, 10),
    (0xFF, 0xFF, 0x00, 11),
    (0x00, 0x00, 0xFF, 12),
    (0xFF, 0x00, 0xFF, 13),
    (0x00, 0xFF, 0xFF, 14),
    (0xFF, 0xFF, 0xFF, 15),
    # Main block of 256 colours
    (0x00, 0x00, 0x00, 16),
    (0x00, 0x00, 0x5F, 17),
    (0x00, 0x00, 0x87, 18),
    (0x00, 0x00, 0xAF, 19),
    (0x00, 0x00, 0xD7, 20),
    (0x00, 0x00, 0xFF, 21),
    (0x00, 0x5F, 0x00, 22),
    (0x00, 0x5F, 0x5F, 23),
    (0x00, 0x5F, 0x87, 24),
    (0x00, 0x5F, 0xAF, 25),
    (0x00, 0x5F, 0xD7, 26),
    (0x00, 0x5F, 0xFF, 27),
    (0x00, 0x87, 0x00, 28),
    (0x00, 0x87, 0x5F, 29),
    (0x00, 0x87, 0x87, 30),
    (0x00, 0x87, 0xAF, 31),
    (0x00, 0x87, 0xD7, 32),
    (0x00, 0x87, 0xFF, 33),
    (0x00, 0xAF, 0x00, 34),
    (0x00, 0xAF, 0x5F, 35),
    (0x00, 0xAF, 0x87, 36),
    (0x00, 0xAF, 0xAF, 37),
    (0x00, 0xAF, 0xD7, 38),
    (0x00, 0xAF, 0xFF, 39),
    (0x00, 0xD7, 0x00, 40),
    (0x00, 0xD7, 0x5F, 41),
    (0x00, 0xD7, 0x87, 42),
    (0x00, 0xD7, 0xAF, 43),
    (0x00, 0xD7, 0xD7, 44),
    (0x00, 0xD7, 0xFF, 45),
    (0x00, 0xFF, 0x00, 46),
    (0x00, 0xFF, 0x5F, 47),
    (0x00, 0xFF, 0x87, 48),
    (0x00, 0xFF, 0xAF, 49),
    (0x00, 0xFF, 0xD7, 50),
    (0x00, 0xFF, 0xFF, 51),
    (0x5F, 0x00, 0x00, 52),
    (0x5F, 0x00, 0x5F, 53),
    (0x5F, 0x00, 0x87, 54),
    (0x5F, 0x00, 0xAF, 55),
    (0x5F, 0x00, 0xD7, 56),
    (0x5F, 0x00, 0xFF, 57),
    (0x5F, 0x5F, 0x00, 58),
    (0x5F, 0x5F, 0x5F, 59),
    (0x5F, 0x5F, 0x87, 60),
    (0x5F, 0x5F, 0xAF, 61),
    (0x5F, 0x5F, 0xD7, 62),
    (0x5F, 0x5F, 0xFF, 63),
    (0x5F, 0x87, 0x00, 64),
    (0x5F, 0x87, 0x5F, 65),
    (0x5F, 0x87, 0x87, 66),
    (0x5F, 0x87, 0xAF, 67),
    (0x5F, 0x87, 0xD7, 68),
    (0x5F, 0x87, 0xFF, 69),
    (0x5F, 0xAF, 0x00, 70),
    (0x5F, 0xAF, 0x5F, 71),
    (0x5F, 0xAF, 0x87, 72),
    (0x5F, 0xAF, 0xAF, 73),
    (0x5F, 0xAF, 0xD7, 74),
    (0x5F, 0xAF, 0xFF, 75),
    (0x5F, 0xD7, 0x00, 76),
    (0x5F, 0xD7, 0x5F, 77),
    (0x5F, 0xD7, 0x87, 78),
    (0x5F, 0xD7, 0xAF, 79),
    (0x5F, 0xD7, 0xD7, 80),
    (0x5F, 0xD7, 0xFF, 81),
    (0x5F, 0xFF, 0x00, 82),
    (0x5F, 0xFF, 0x5F, 83),
    (0x5F, 0xFF, 0x87, 84),
    (0x5F, 0xFF, 0xAF, 85),
    (0x5F, 0xFF, 0xD7, 86),
    (0x5F, 0xFF, 0xFF, 87),
    (0x87, 0x00, 0x00, 88),
    (0x87, 0x00, 0x5F, 89),
    (0x87, 0x00, 0x87, 90),
    (0x87, 0x00, 0xAF, 91),
    (0x87, 0x00, 0xD7, 92),
    (0x87, 0x00, 0xFF, 93),
    (0x87, 0x5F, 0x00, 94),
    (0x87, 0x5F, 0x5F, 95),
    (0x87, 0x5F, 0x87, 96),
    (0x87, 0x5F, 0xAF, 97),
    (0x87, 0x5F, 0xD7, 98),
    (0x87, 0x5F, 0xFF, 99),
    (0x87, 0x87, 0x00, 100),
    (0x87, 0x87, 0x5F, 101),
    (0x87, 0x87, 0x87, 102),
    (0x87, 0x87, 0xAF, 103),
    (0x87, 0x87, 0xD7, 104),
    (0x87, 0x87, 0xFF, 105),
    (0x87, 0xAF, 0x00, 106),
    (0x87, 0xAF, 0x5F, 107),
    (0x87, 0xAF, 0x87, 108),
    (0x87, 0xAF, 0xAF, 109),
    (0x87, 0xAF, 0xD7, 110),
    (0x87, 0xAF, 0xFF, 111),
    (0x87, 0xD7, 0x00, 112),
    (0x87, 0xD7, 0x5F, 113),
    (0x87, 0xD7, 0x87, 114),
    (0x87, 0xD7, 0xAF, 115),
    (0x87, 0xD7, 0xD7, 116),
    (0x87, 0xD7, 0xFF, 117),
    (0x87, 0xFF, 0x00, 118),
    (0x87, 0xFF, 0x5F, 119),
    (0x87, 0xFF, 0x87, 120),
    (0x87, 0xFF, 0xAF, 121),
    (0x87, 0xFF, 0xD7, 122),
    (0x87, 0xFF, 0xFF, 123),
    (0xAF, 0x00, 0x00, 124),
    (0xAF, 0x00, 0x5F, 125),
    (0xAF, 0x00, 0x87, 126),
    (0xAF, 0x00, 0xAF, 127),
    (0xAF, 0x00, 0xD7, 128),
    (0xAF, 0x00, 0xFF, 129),
    (0xAF, 0x5F, 0x00, 130),
    (0xAF, 0x5F, 0x5F, 131),
    (0xAF, 0x5F, 0x87, 132),
    (0xAF, 0x5F, 0xAF, 133),
    (0xAF, 0x5F, 0xD7, 134),
    (0xAF, 0x5F, 0xFF, 135),
    (0xAF, 0x87, 0x00, 136),
    (0xAF, 0x87, 0x5F, 137),
    (0xAF, 0x87, 0x87, 138),
    (0xAF, 0x87, 0xAF, 139),
    (0xAF, 0x87, 0xD7, 140),
    (0xAF, 0x87, 0xFF, 141),
    (0xAF, 0xAF, 0x00, 142),
    (0xAF, 0xAF, 0x5F, 143),
    (0xAF, 0xAF, 0x87, 144),
    (0xAF, 0xAF, 0xAF, 145),
    (0xAF, 0xAF, 0xD7, 146),
    (0xAF, 0xAF, 0xFF, 147),
    (0xAF, 0xD7, 0x00, 148),
    (0xAF, 0xD7, 0x5F, 149),
    (0xAF, 0xD7, 0x87, 150),
    (0xAF, 0xD7, 0xAF, 151),
    (0xAF, 0xD7, 0xD7, 152),
    (0xAF, 0xD7, 0xFF, 153),
    (0xAF, 0xFF, 0x00, 154),
    (0xAF, 0xFF, 0x5F, 155),
    (0xAF, 0xFF, 0x87, 156),
    (0xAF, 0xFF, 0xAF, 157),
    (0xAF, 0xFF, 0xD7, 158),
    (0xAF, 0xFF, 0xFF, 159),
    (0xD7, 0x00, 0x00, 160),
    (0xD7, 0x00, 0x5F, 161),
    (0xD7, 0x00, 0x87, 162),
    (0xD7, 0x00, 0xAF, 163),
    (0xD7, 0x00, 0xD7, 164),
    (0xD7, 0x00, 0xFF, 165),
    (0xD7, 0x5F, 0x00, 166),
    (0xD7, 0x5F, 0x5F, 167),
    (0xD7, 0x5F, 0x87, 168),
    (0xD7, 0x5F, 0xAF, 169),
    (0xD7, 0x5F, 0xD7, 170),
    (0xD7, 0x5F, 0xFF, 171),
    (0xD7, 0x87, 0x00, 172),
    (0xD7, 0x87, 0x5F, 173),
    (0xD7, 0x87, 0x87, 174),
    (0xD7, 0x87, 0xAF, 175),
    (0xD7, 0x87, 0xD7, 176),
    (0xD7, 0x87, 0xFF, 177),
    (0xD7, 0xAF, 0x00, 178),
    (0xD7, 0xAF, 0x5F, 179),
    (0xD7, 0xAF, 0x87, 180),
    (0xD7, 0xAF, 0xAF, 181),
    (0xD7, 0xAF, 0xD7, 182),
    (0xD7, 0xAF, 0xFF, 183),
    (0xD7, 0xD7, 0x00, 184),
    (0xD7, 0xD7, 0x5F, 185),
    (0xD7, 0xD7, 0x87, 186),
    (0xD7, 0xD7, 0xAF, 187),
    (0xD7, 0xD7, 0xD7, 188),
    (0xD7, 0xD7, 0xFF, 189),
    (0xD7, 0xFF, 0x00, 190),
    (0xD7, 0xFF, 0x5F, 191),
    (0xD7, 0xFF, 0x87, 192),
    (0xD7, 0xFF, 0xAF, 193),
    (0xD7, 0xFF, 0xD7, 194),
    (0xD7, 0xFF, 0xFF, 195),
    (0xFF, 0x00, 0x00, 196),
    (0xFF, 0x00, 0x5F, 197),
    (0xFF, 0x00, 0x87, 198),
    (0xFF, 0x00, 0xAF, 199),
    (0xFF, 0x00, 0xD7, 200),
    (0xFF, 0x00, 0xFF, 201),
    (0xFF, 0x5F, 0x00, 202),
    (0xFF, 0x5F, 0x5F, 203),
    (0xFF, 0x5F, 0x87, 204),
    (0xFF, 0x5F, 0xAF, 205),
    (0xFF, 0x5F, 0xD7, 206),
    (0xFF, 0x5F, 0xFF, 207),
    (0xFF, 0x87, 0x00, 208),
    (0xFF, 0x87, 0x5F, 209),
    (0xFF, 0x87, 0x87, 210),
    (0xFF, 0x87, 0xAF, 211),
    (0xFF, 0x87, 0xD7, 212),
    (0xFF, 0x87, 0xFF, 213),
    (0xFF, 0xAF, 0x00, 214),
    (0xFF, 0xAF, 0x5F, 215),
    (0xFF, 0xAF, 0x87, 216),
    (0xFF, 0xAF, 0xAF, 217),
    (0xFF, 0xAF, 0xD7, 218),
    (0xFF, 0xAF, 0xFF, 219),
    (0xFF, 0xD7, 0x00, 220),
    (0xFF, 0xD7, 0x5F, 221),
    (0xFF, 0xD7, 0x87, 222),
    (0xFF, 0xD7, 0xAF, 223),
    (0xFF, 0xD7, 0xD7, 224),
    (0xFF, 0xD7, 0xFF, 225),
    (0xFF, 0xFF, 0x00, 226),
    (0xFF, 0xFF, 0x5F, 227),
    (0xFF, 0xFF, 0x87, 228),
    (0xFF, 0xFF, 0xAF, 229),
    (0xFF, 0xFF, 0xD7, 230),
    (0xFF, 0xFF, 0xFF, 231),
    # Various gray values
    (0x08, 0x08, 0x08, 232),
    (0x12, 0x12, 0x12, 233),
    (0x1C, 0x1C, 0x1C, 234),
    (0x26, 0x26, 0x26, 235),
    (0x30, 0x30, 0x30, 236),
    (0x3A, 0x3A, 0x3A, 237),
    (0x44, 0x44, 0x44, 238),
    (0x4E, 0x4E, 0x4E, 239),
    (0x58, 0x58, 0x58, 240),
    (0x62, 0x62, 0x62, 241),
    (0x6C, 0x6C, 0x6C, 242),
    (0x76, 0x76, 0x76, 243),
    (0x80, 0x80, 0x80, 244),
    (0x8A, 0x8A, 0x8A, 245),
    (0x94, 0x94, 0x94, 246),
    (0x9E, 0x9E, 0x9E, 247),
    (0xA8, 0xA8, 0xA8, 248),
    (0xB2, 0xB2, 0xB2, 249),
    (0xBC, 0xBC, 0xBC, 250),
    (0xC6, 0xC6, 0xC6, 251),
    (0xD0, 0xD0, 0xD0, 252),
    (0xDA, 0xDA, 0xDA, 253),
    (0xE4, 0xE4, 0xE4, 254),
    (0xEE, 0xEE, 0xEE, 255),
]

# Tiling options based on which pixels in a 2x2 grid are filled
# First two elements are the filled elements
# The second two are the shape and its inverse
tile_map = [
    ((1, 1), (1, 1), "\u2588", "\u2588"),  # |█| Full block
    ((0, 0), (1, 1), "\u2584", "\u2580"),  # |▄| Lower half block
    ((0, 1), (0, 1), "\u2590", "\u258C"),  # |▐| Right half block
    ((0, 1), (1, 0), "\u259E", "\u259A"),  # |▞| Quadrant upper right and lower left
    ((0, 1), (1, 1), "\u259F", "\u2598"),  # |▟| Quadrant upper right and lower left and lower right
    ((1, 0), (0, 1), "\u259A", "\u259E"),  # |▚| Quadrant upper left and lower right
    ((1, 0), (1, 0), "\u258C", "\u2590"),  # |▌| Left half block
    ((1, 0), (1, 1), "\u2599", "\u259D"),  # |▙| Quadrant upper left and lower left and lower right
    ((1, 1), (0, 0), "\u2580", "\u2584"),  # |▀| Upper half block
    ((1, 1), (0, 1), "\u259C", "\u2596"),  # |▜| Quadrant upper left and upper right and lower right
    ((1, 1), (1, 0), "\u259B", "\u2597"),  # |▛| Quadrant upper left and upper right and lower left
]


def distance(a, b):
    return math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2)


def best_match(px):
    # Search through our list and choose the one with the smallest distance
    best = colour_map[0]
    best_distance = distance((0, 0, 0), (255, 255, 255))

    for c in colour_map:
        d = distance(px, c)

        if d < best_distance:
            best = c
            best_distance = d

    return best


def best_grouping(tl, tr, bl, br):

    fg = (0, 0, 0)
    bg = (0, 0, 0)
    best = None
    best_value = sys.float_info.max

    # Go through each of our possible tile mappings
    for g in tile_map:
        group_a = []
        group_b = []

        # Sort each element into the appropriate group
        (group_a if g[0][0] == 1 else group_b).append(tl)
        (group_a if g[0][1] == 1 else group_b).append(tr)
        (group_a if g[1][0] == 1 else group_b).append(bl)
        (group_a if g[1][1] == 1 else group_b).append(br)

        # Calculate the average of our groups
        avg_a = tuple(map(lambda y: sum(y) / float(len(y)), zip(*group_a)))
        avg_b = tuple(map(lambda y: sum(y) / float(len(y)), zip(*group_b)))

        # Subtract our mean from the values
        group_a = [(v[0] - avg_a[0], v[1] - avg_a[1], v[2] - avg_a[2]) for v in group_a]
        group_b = [(v[0] - avg_b[0], v[1] - avg_b[1], v[2] - avg_b[2]) for v in group_b]

        # Take the sum of squares of these values
        total = 0
        for v in group_a + group_b:
            total += v[0] ** 2 + v[1] ** 2 + v[2] ** 2

        if total < best_value:
            best_value = total
            best = g
            fg = avg_a
            bg = avg_b

    # If one group wasn't filled (the 1,1,1,1 group) set fg and bg the same
    fg = fg if fg else bg
    bg = bg if bg else fg

    # Always favour the lighter color as the foreground since it helps with some terminals
    if sum(fg) > sum(bg):
        return (fg, bg, best[2], best_value)
    else:
        return (bg, fg, best[3], best_value)


# Make our colour using our colour map
def colour(tl, tr, bl, br):
    # We have to pick two colours that best represent this set of pixels
    # and we can arrange them into any 2x2 grid. so we will try them all
    # and see which one results in the lowest error

    # Groupings
    value = best_grouping(tl, tr, bl, br)

    # Foreground colour
    fg = best_match(value[0])[3]
    # Background colour
    bg = best_match(value[1])[3]
    # Group character
    ch = value[2]

    return (fg, bg, ch)


# Convert our image into an ansi coded string
def ampscii(src, unicode=True):

    # Load the image
    im = Image.open(src)
    pix = im.load()

    # Loop through our image pixels in 2x2 blocks and get the best match for colours
    # Since our resolution is double in the x as y, we jump 2x as fast in that direction
    rows = []
    for y in range(1, im.height - 1, 4):
        row = []
        for x in range(1, im.width - 1, 2):
            tl = pix[x + 0, y + 0]
            tr = pix[x + 1, y + 0]
            bl = pix[x + 0, y + 1]
            br = pix[x + 1, y + 1]

            row.append(colour(tl, tr, bl, br))
        rows.append(row)

    # Now convert those rows into a string
    output = ""

    for l in rows:
        fg = None
        bg = None
        for v in l:

            # Change foreground colour
            if v[0] != fg:
                fg = v[0]
                output += "\x1b[38;5;{}m".format(fg)

            # Change background colour
            if v[1] != bg:
                bg = v[1]
                output += "\x1b[48;5;{}m".format(bg)

            # TODO check if we can flip the FG and BG colours to not change

            # Append our character
            if unicode:
                output += v[2]
            else:
                output += "#"

        # Reset and make a new line
        output += "\x1b[0m\n"

    return output


if __name__ == "__main__":
    print(ampscii(sys.argv[1]))
