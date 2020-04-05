# Adafruit project link https://github.com/adafruit/Adafruit_CircuitPython_MLX90640

import time
import board
import busio
import adafruit_mlx90640
from PIL import Image
import numpy as np
import random
from matplotlib import cm
from argparse import Action, ArgumentParser
from os import path

# From https://pymotw.com/2/argparse/ or
# https://stackoverflow.com/questions/8632354/python-argparse-custom-actions-with-additional-arguments-passed
class AbsPathAction(Action):
    """Action to convert `argparse` arguments to absolute paths.
    """
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if nargs is not None:
            raise ValueError("`nargs` not allowed")
        super(AbsPathAction, self).__init__(option_strings, dest, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest,
                path.abspath(path.expanduser(values)))

# function to convert temperatures to pixels on image
def td_to_image(f, Tmin, Tmax):
    norm = [(scale(v,Tmin,Tmax)-Tmin)/(Tmax-Tmin) for v in f]
    norm = cm.inferno(norm)*255
    norm = np.uint8(norm)
    norm.shape = (24,32,4)
    return norm

def scale(val, min, max):
    if val < min:
        return min
    elif val > max:
        return max
    else:
        return val

def print_frame(frame):
    for h in range(24):
        for w in range(32):
            t = frame[h*32 + w]
            print("%0.1f, " % t)
        print()
    print()

def make_image(frame,  output_file, Tmin , Tmax):
    ta_img = td_to_image(frame, Tmin, Tmax)
    img = Image.fromarray(ta_img)
    img = img.convert('RGB')
    img.save(output_file)

def main():
    Tmin = 0
    Tmax = 60
    # Parse the arguments.
    # All paths are converted to *absolute* with respect to the working
    # directory from which the script is invoked.
    parser = ArgumentParser(
        description="Run Module 1 (captures visible.jpeg) from a visible light camera."
    )

    parser.add_argument('file_path',
                        help="file name of captured image from visible camera",
                        action=AbsPathAction)
    
    parser.add_argument("--simulation", help="simulate the camera feed with random values",
                        action="store_true")

    args = parser.parse_args()

    frame = [0] * 768
    try:
        if args.simulation:
            frame = [ random.randint(Tmin,Tmax) for i in range(768)]
        else:
            i2c = busio.I2C(board.SCL, board.SDA, frequency=800000)
            mlx = adafruit_mlx90640.MLX90640(i2c)
            print("MLX addr detected on I2C", [hex(i) for i in mlx.serial_number])
            mlx.getFrame(frame)
    except Exception as e:
        print("Error while capturing IR Image:", e)
        exit(1)
    make_image(frame, args.file_path, Tmin, Tmax)

if __name__ == "__main__":
    main()
    
