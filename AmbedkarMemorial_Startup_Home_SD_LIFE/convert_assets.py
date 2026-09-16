import argparse
from PIL import Image, ImageOps

def rgb565(r,g,b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("src")
    p.add_argument("dst")
    p.add_argument("--width",type=int,default=160)
    p.add_argument("--height",type=int,default=170)
    a=p.parse_args()

    im=ImageOps.fit(
        Image.open(a.src).convert("RGB"),
        (a.width,a.height),
        method=Image.Resampling.LANCZOS,
        centering=(0.5,0.5)
    )

    with open(a.dst,"wb") as f:
        for r,g,b in im.getdata():
            v=rgb565(r,g,b)
            f.write(bytes([(v>>8)&255,v&255]))

    print(f"{a.src} -> {a.dst}")
    print(f"{a.width}x{a.height}, {a.width*a.height*2} bytes")

if __name__=="__main__":
    main()
