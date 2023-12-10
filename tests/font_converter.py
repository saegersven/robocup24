from lxml import etree
import struct

parsed = etree.parse("runtime_data/font.fnt")

chars = parsed.getroot()[3]

with open("runtime_data/font.bin", "wb") as f:
    f.write(struct.pack('B', len(chars)))
    for char in chars:
        f.write(struct.pack('BBBBBB',
            int(char.attrib["id"]),
            int(char.attrib["x"]),
            int(char.attrib["y"]),
            int(char.attrib["width"]),
            int(char.attrib["height"]),
            int(char.attrib["xadvance"])
            ))