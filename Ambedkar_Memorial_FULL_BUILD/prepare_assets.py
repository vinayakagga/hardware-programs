from pathlib import Path
from PIL import Image, ImageOps, ImageEnhance, ImageDraw, ImageFont

ROOT=Path(__file__).parent
SRC=ROOT/"source_images"
OUT=ROOT/"assets"
OUT.mkdir(exist_ok=True)

def rgb565(r,g,b):
    return ((r & 0xF8)<<8)|((g & 0xFC)<<3)|(b>>3)

def convert(src,dst,w,h,centering=(0.5,0.5),brightness=1.0):
    im=Image.open(src).convert("RGB")
    if brightness != 1.0:
        im=ImageEnhance.Brightness(im).enhance(brightness)
    im=ImageOps.fit(im,(w,h),method=Image.Resampling.LANCZOS,centering=centering)
    with open(dst,"wb") as f:
        pix=im.load()
        for y in range(h):
            for x in range(w):
                v=rgb565(*pix[x,y])
                f.write(bytes((v>>8,v&255)))

specs={
"splash.rgb565":("splash.png",480,320),
"life1.rgb565":("life1.png",160,170),
"life2.rgb565":("life2.png",160,170),
"life3.rgb565":("life3.jfif",160,170),
"life4.rgb565":("life4.jfif",160,170),
"life5.rgb565":("life5.jfif",160,170),
"timeline1.rgb565":("life1.png",190,140),
"timeline2.rgb565":("life2.png",190,140),
"timeline3.rgb565":("life3.jfif",190,140),
"timeline4.rgb565":("life4.jfif",190,140),
"timeline5.rgb565":("life5.png" if (SRC/"life5.png").exists() else "life5.jfif",190,140),
"constitution1.rgb565":("memorial_entrance.jfif",190,140),
"constitution2.rgb565":("life4.jfif",190,140),
"constitution3.rgb565":("life2.png",190,140),
"ideas1.rgb565":("life2.png",190,140),
"ideas2.rgb565":("life3.jfif",190,140),
"ideas3.rgb565":("ambedkar_statue.jfif",190,140),
"legacy1.rgb565":("life5.jfif",190,140),
"legacy2.rgb565":("ambedkar_statue.jfif",190,140),
"legacy3.rgb565":("life4.jfif",190,140),
"visit1.rgb565":("memorial_entrance.jfif",220,150),
"visit2.rgb565":("memorial_interior.jfif",220,150),
"visit3.rgb565":("ambedkar_statue.jfif",220,150)
}

for out_name,(src_name,w,h) in specs.items():
    src=SRC/src_name
    if not src.exists():
        raise FileNotFoundError(src)
    convert(src,OUT/out_name,w,h)

print("Generated",len(specs),"RGB565 assets.")
print("Total bytes:",sum(p.stat().st_size for p in OUT.glob("*.rgb565")))
