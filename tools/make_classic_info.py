#!/usr/bin/env python3
"""Generate conservative old-style Amiga Workbench 2.x/3.x tool icons."""
import struct
from pathlib import Path

WB_DISKMAGIC = 0xE310
WB_DISKVERSION = 1
WBTOOL = 3
NO_ICON_POSITION = 0x80000000
GFLG_GADGIMAGE = 0x0004
GADGHIMAGE = 0x0002
STACK = 131072

FONT = {
    'A':[14,17,17,31,17,17,17], 'B':[30,17,17,30,17,17,30],
    'C':[15,16,16,16,16,16,15], 'D':[30,17,17,17,17,17,30],
    'E':[31,16,16,30,16,16,31], 'F':[31,16,16,30,16,16,16],
    'G':[15,16,16,23,17,17,15], 'I':[31,4,4,4,4,4,31],
    'P':[30,17,17,30,16,16,16], 'T':[31,4,4,4,4,4,4],
    'U':[17,17,17,17,17,17,14],
    '0':[14,17,19,21,25,17,14], '1':[4,12,4,4,4,4,14],
    '2':[14,17,1,2,4,8,31], '3':[30,1,1,14,1,1,30],
    '4':[2,6,10,18,31,2,2], '5':[31,16,16,30,1,1,30],
    '6':[14,16,16,30,17,17,14], '7':[31,1,2,4,8,8,8],
    '8':[14,17,17,14,17,17,14], '9':[14,17,17,15,1,1,14],
    ' ':[0,0,0,0,0,0,0],
}

def draw_text(img, x, y, text, color=1):
    for ch in text.upper():
        glyph = FONT.get(ch, FONT[' '])
        for yy, row in enumerate(glyph):
            for xx in range(5):
                if row & (1 << (4-xx)):
                    px, py = x+xx, y+yy
                    if 0 <= py < len(img) and 0 <= px < len(img[0]):
                        img[py][px] = color
        x += 6

def pixels(title, badge):
    w, h = 52, 22
    p = [[0]*w for _ in range(h)]
    for x in range(w): p[0][x] = p[h-1][x] = 1
    for y in range(h): p[y][0] = p[y][w-1] = 1
    for y in range(3,18):
        for x in range(4,36): p[y][x] = 2
    for x in range(4,36): p[3][x] = p[17][x] = 1
    for y in range(3,18): p[y][4] = p[y][35] = 1
    for x in range(8,31): p[14][x] = 1 if x % 2 else 3
    for y in range(5,16):
        for x in range(38,49): p[y][x] = 2
    for x in range(38,49): p[5][x] = p[15][x] = 1
    for y in range(5,16): p[y][38] = p[y][48] = 1
    draw_text(p, 8, 5, title, 1)
    draw_text(p, 39, 7, badge, 1)
    return p

def selected(p):
    return [[0 if v == 0 else (3 if v == 1 else 1) for v in row] for row in p]

def planar(p, depth=2):
    h, w = len(p), len(p[0])
    words = (w+15)//16
    out = bytearray()
    for y in range(h):
        for plane in range(depth):
            for wi in range(words):
                word = 0
                for bit in range(16):
                    x = wi*16+bit
                    if x < w and ((p[y][x] >> plane) & 1):
                        word |= 1 << (15-bit)
                out += struct.pack('>H', word)
    return bytes(out)

def image_block(p):
    h, w, depth = len(p), len(p[0]), 2
    return struct.pack('>hhhhhLBBL',0,0,w,h,depth,1,0x03,0,0) + planar(p, depth)

def tt_string(s):
    b = s.encode('ascii') + b'\0'
    return struct.pack('>L',len(b)) + b

def tooltypes(entries):
    out = struct.pack('>L',(len(entries)+1)*4)
    for e in entries: out += tt_string(e)
    return out + tt_string('')

def make_icon(title, badge, build, kind):
    p1 = pixels(title,badge)
    p2 = selected(p1)
    w,h = len(p1[0]),len(p1)
    header = bytearray()
    header += struct.pack('>HH',WB_DISKMAGIC,WB_DISKVERSION)
    header += struct.pack('>L',0)
    header += struct.pack('>hh',0,0)
    header += struct.pack('>hh',w,h)
    header += struct.pack('>HHH',GFLG_GADGIMAGE|GADGHIMAGE,0,0)
    header += struct.pack('>L',1)
    header += struct.pack('>L',1)
    header += struct.pack('>L',0)
    header += struct.pack('>l',0)
    header += struct.pack('>L',0)
    header += struct.pack('>H',0)
    header += struct.pack('>L',1)
    assert len(header) == 0x30
    header += struct.pack('>BB',WBTOOL,0)
    header += struct.pack('>L',0)
    header += struct.pack('>L',1)
    header += struct.pack('>LL',NO_ICON_POSITION,NO_ICON_POSITION)
    header += struct.pack('>L',0)
    header += struct.pack('>L',0)
    header += struct.pack('>l',STACK)
    assert len(header) == 78
    return bytes(header) + image_block(p1) + image_block(p2) + tooltypes([f'TYPE={kind}',f'BUILD={build}',f'STACK={STACK}'])

def main():
    outdir = Path(__file__).resolve().parents[1] / 'icons'
    outdir.mkdir(parents=True, exist_ok=True)
    specs = [
        ('AmiGPT020.info','GPT','020','020','CLI'),
        ('AmiGPT030FPU.info','GPT','030','030FPU','CLI'),
        ('AmiGPT040.info','GPT','040','040','CLI'),
        ('AmiGPTGUI020.info','GUI','020','020','GUI'),
        ('AmiGPTGUI030FPU.info','GUI','030','030FPU','GUI'),
        ('AmiGPTGUI040.info','GUI','040','040','GUI'),
    ]
    for filename,title,badge,build,kind in specs:
        path = outdir / filename
        data = make_icon(title,badge,build,kind)
        path.write_bytes(data)
        print(f'wrote {path} ({len(data)} bytes)')

if __name__ == '__main__':
    main()
