#!/usr/bin/env python3
"""mkicons.py - generate AmiAuth's classic Amiga .info icon files.

Writes original-format (OS 1.x layout, OS 2.x revision) 4-colour, 2-bitplane
DiskObject icons - the most compatible kind: rendered by every Workbench from
2.04 up, and remapped sensibly by NewIcons/GlowIcons-era systems.

The pixel art is edited HERE as ASCII grids ('.'=bg grey, '#'=black,
'w'=white, 'b'=blue - the standard WB 3.x 4-colour palette indices 0..3).
Regenerate the committed icons/ files with:  python3 tools/mkicons.py icons/

A --check mode re-parses every file it just wrote and asserts the geometry,
type, tooltypes and plane data survive a round trip; --ppm additionally dumps
host-viewable previews next to the icons.
"""

import struct
import sys
import os

# --- Amiga palette for previews (WB 3.x default 4 colours) -------------------
PALETTE = {0: (149, 149, 149), 1: (0, 0, 0), 2: (255, 255, 255), 3: (59, 103, 162)}
CHARS = {'.': 0, '#': 1, 'w': 2, 'b': 3}

WBDRAWER, WBTOOL, WBPROJECT = 1, 3, 4
NO_ICON_POSITION = -0x80000000

# --- pixel art ---------------------------------------------------------------
# 32x20. A chest/drawer with a padlock on the front: locked codes live here.
ART_DRAWER = """
................................
..#######################.......
..#wwwwwwwwwwwwwwwwwwwww##......
..#wwwwwwwwwwwwwwwwwwwww#w#.....
..#######################w#.....
..#bbbbbbbbbb#####bbbbbb#w#.....
..#bbbbbbbbb##bbb##bbbbb#w#.....
..#bbbbbbbbb#bbbbb#bbbbb#w#.....
..#bbbbbbbb#########bbbb#w#.....
..#bbbbbbbb#wwwwwww#bbbb#w#.....
..#bbbbbbbb#ww###ww#bbbb#w#.....
..#bbbbbbbb#www#www#bbbb#w#.....
..#bbbbbbbb#wwwwwww#bbbb#w#.....
..#bbbbbbbb#########bbbb#w#.....
..#bbbbbbbbbbbbbbbbbbbbb#w#.....
..#######################w#.....
...#wwwwwwwwwwwwwwwwwwwww#......
...#######################......
................................
................................
"""

# 32x20. A tool icon: a little screen showing an OTP code with countdown bar.
ART_GUI = """
................................
....######################......
....#wwwwwwwwwwwwwwwwwwww#......
....#w##################w#......
....#w#bbbbbbbbbbbbbbbb#w#......
....#w#bbbbbbbbbbbbbbbb#w#......
....#w#b##b##b##b##b##b#w#......
....#w#bw#bw#bw#bw#bw#b#w#......
....#w#b##b##b##b##b##b#w#......
....#w#bbbbbbbbbbbbbbbb#w#......
....#w#wwwwwwwwwwwwwwww#w#......
....#w#bbbbbbbbbbbwwwww#w#......
....#w##################w#......
....#wwwwwwwwwwwwwwwwwwww#......
....######################......
.........##########.............
.......################.........
......##................##......
................................
................................
"""

# 32x20. The guide: an open book with a blue bookmark.
ART_GUIDE = """
................................
.........###........###........
.......##www#######wwww##.......
......#wwwwwwww##wwwwwwww#......
.....#wwbbbbwww##wwbbbbwww#.....
.....#wwwwwwwww##wwwwwwwww#.....
.....#wwbbbwwww##wwbbbwwww#.....
.....#wwwwwwwww##wwwwwwwww#.....
.....#wwbbbbwww##wwbbbbwww#.....
.....#wwwwwwwww##wwwwwwwww#.....
.....#wwbbwwwww##wwbbbwwww#.....
.....#wwwwwwwww##wwwwwwwww#.....
.....#wwwwwwww#ww#wwwwwwww#.....
......#######w#ww#w#######......
.............#ww##..............
..............##................
................................
................................
................................
................................
"""


def parse_art(art):
    rows = [r for r in art.splitlines() if r.strip()]
    w = max(len(r) for r in rows)
    rows = [r.ljust(w, '.') for r in rows]
    grid = [[CHARS.get(c, 0) for c in row] for row in rows]
    return grid, w, len(rows)


def planes(grid, w, h, depth=2):
    """Pack the pixel grid into `depth` word-aligned bitplanes."""
    bpr = ((w + 15) // 16) * 2
    out = b''
    for p in range(depth):
        for y in range(h):
            rowbytes = bytearray(bpr)
            for x in range(w):
                if (grid[y][x] >> p) & 1:
                    rowbytes[x // 8] |= 0x80 >> (x % 8)
            out += bytes(rowbytes)
    return out


def image(grid, w, h, depth=2):
    """struct Image + plane data (ImageData is a placeholder pointer)."""
    hdr = struct.pack('>hhhhh I BB I', 0, 0, w, h, depth,
                      1,                      # ImageData (placeholder != 0)
                      (1 << depth) - 1, 0,    # PlanePick, PlaneOnOff
                      0)                      # NextImage
    return hdr + planes(grid, w, h, depth)


def cstr(s):
    b = s.encode('ascii') + b'\0'
    return struct.pack('>I', len(b)) + b


def icon(art, do_type, default_tool=None, tooltypes=(), stack=8192):
    grid, w, h = parse_art(art)
    is_drawer = do_type == WBDRAWER

    gadget = struct.pack('>IhhhhHHHIIIIIHI',
                         0,                # NextGadget
                         0, 0, w, h,       # LeftEdge/TopEdge/Width/Height
                         0x0004,           # Flags: GADGIMAGE|GADGHCOMP
                         0x0003,           # Activation: RELVERIFY|IMMEDIATE
                         0x0001,           # GadgetType: BOOLGADGET
                         1,                # GadgetRender (placeholder != 0)
                         0, 0,             # SelectRender, GadgetText
                         0, 0,             # MutualExclude, SpecialInfo
                         0,                # GadgetID
                         1)                # UserData: OS 2.x icon revision
    assert len(gadget) == 44, len(gadget)

    do = struct.pack('>HH', 0xE310, 1) + gadget
    do += struct.pack('>Bx', do_type)
    do += struct.pack('>IIiiIII',
                      1 if default_tool else 0,     # do_DefaultTool
                      1 if tooltypes else 0,        # do_ToolTypes
                      NO_ICON_POSITION, NO_ICON_POSITION,
                      1 if is_drawer else 0,        # do_DrawerData
                      0,                            # do_ToolWindow
                      stack)
    assert len(do) == 78, len(do)

    body = b''
    if is_drawer:
        # struct DrawerData: NewWindow (48) + dd_CurrentX/Y (8)
        newwin = struct.pack('>hhhh', 30, 30, 400, 120)          # box
        newwin += struct.pack('>BB', 255, 255)                   # pens
        newwin += struct.pack('>I', 0)                           # IDCMP
        newwin += struct.pack('>I', 0)                           # Flags
        newwin += struct.pack('>IIII', 0, 0, 0, 0)               # FirstGadget,
                                                                 # CheckMark,
                                                                 # Title, Screen
        newwin += struct.pack('>I', 0)                           # BitMap
        newwin += struct.pack('>hhhh', 90, 40, 0x7fff, 0x7fff)   # Min/Max size
        newwin += struct.pack('>H', 1)                           # WBENCHSCREEN
        assert len(newwin) == 48, len(newwin)
        body += newwin + struct.pack('>ii', 0, 0)                # dd_CurrentX/Y

    body += image(grid, w, h)
    if default_tool:
        body += cstr(default_tool)
    if tooltypes:
        body += struct.pack('>I', (len(tooltypes) + 1) * 4)
        for t in tooltypes:
            body += cstr(t)
    if is_drawer:
        # OS 2.x DrawerData extension (revision 1): dd_Flags + dd_ViewModes.
        body += struct.pack('>IH', 1, 0)   # view icons, default mode
    return do + body


ICONS = {
    'AmiAuth.info': dict(art=ART_DRAWER, do_type=WBDRAWER),
    'AmiAuthGUI.info': dict(art=ART_GUI, do_type=WBTOOL, stack=16384,
                            tooltypes=('CX_POPKEY=ctrl alt a', 'CX_POPUP=yes',
                                       '(TIMESERVER=pool.ntp.org)',
                                       'DONOTWAIT')),
    'AmiAuth.guide.info': dict(art=ART_GUIDE, do_type=WBPROJECT,
                               default_tool='SYS:Utilities/MultiView',
                               stack=65536),
}


def check(path, spec):
    """Round-trip: re-parse the written file and verify what matters."""
    data = open(path, 'rb').read()
    magic, version = struct.unpack('>HH', data[:4])
    assert magic == 0xE310 and version == 1, 'bad DiskObject header'
    w, h = struct.unpack('>hh', data[12:16])   # Gadget.Width/Height
    do_type = data[48]
    assert do_type == spec['do_type'], f'type {do_type} != {spec["do_type"]}'
    grid, aw, ah = parse_art(spec['art'])
    assert (w, h) == (aw, ah), f'geometry {(w, h)} != {(aw, ah)}'
    # image starts after DiskObject (78) + optional DrawerData (56)
    off = 78 + (56 if do_type == WBDRAWER else 0)
    iw, ih, depth = struct.unpack('>hhh', data[off + 4:off + 10])
    assert (iw, ih, depth) == (w, h, 2), 'image header mismatch'
    pdata = data[off + 20:off + 20 + ((w + 15) // 16) * 2 * h * 2]
    assert pdata == planes(grid, w, h), 'plane data mismatch'
    # strings land in order: default tool, then each tooltype
    for t in spec.get('tooltypes', ()):
        assert t.encode() + b'\0' in data, f'tooltype missing: {t}'
    dt = spec.get('default_tool')
    if dt:
        assert dt.encode() + b'\0' in data, 'default tool missing'
    return True


def ppm(path, spec):
    grid, w, h = parse_art(spec['art'])
    scale = 8
    out = [f'P3 {w * scale} {h * scale} 255']
    for y in range(h):
        row = ' '.join(' '.join(map(str, PALETTE[grid[y][x]]))
                       for x in range(w) for _ in range(scale))
        out.extend([row] * scale)
    open(path, 'w').write('\n'.join(out) + '\n')


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else 'icons'
    os.makedirs(outdir, exist_ok=True)
    for name, spec in ICONS.items():
        path = os.path.join(outdir, name)
        with open(path, 'wb') as f:
            f.write(icon(**{k: v for k, v in spec.items()}))
        check(path, spec)
        if '--ppm' in sys.argv:
            ppm(path + '.ppm', spec)
        print(f'{path}: OK ({os.path.getsize(path)} bytes)')


if __name__ == '__main__':
    main()
