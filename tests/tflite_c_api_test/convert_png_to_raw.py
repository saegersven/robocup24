import cv2
import sys
import os
import struct
import numpy as np
import glob

def convert(path, out_dir):
    filename = os.path.basename(path)
    ext = os.path.splitext(filename)[1]

    if 'bin' in ext:
        with open(path, 'rb') as f:
            width, height = struct.unpack('ii', f.read(8))
        
            img = np.zeros((height, width, 3), dtype=np.uint8)
            for i in range(height):
                for j in range(width):
                    for k in range(3):
                        img[i][j][k] = struct.unpack('B', f.read(1))[0]
            
            cv2.imwrite(os.path.join(out_dir, os.path.splitext(filename)[0] + '.png'), img)
    else:
        img = cv2.imread(path)

        with open(os.path.join(out_dir, os.path.splitext(filename)[0] + '.bin'), 'wb') as f:
            f.write(struct.pack('ii', len(img[0]), len(img)))
            f.write(bytearray(img.flatten()))

path = input("Input file or directory: ")

if os.path.isdir(path):
    bin_to_png = "2" in input("PNG to BIN (1) or BIN to PNG (2): ")
    output_dir = input("Input output directory: ")

    e = "*.png"
    if bin_to_png:
        e = "*.bin"
    for file in glob.glob(os.path.join(path, e)):
        convert(file, output_dir)
else:
    convert(path, os.path.dirname(path))
