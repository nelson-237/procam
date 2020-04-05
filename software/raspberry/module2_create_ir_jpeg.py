# Adafruit project link https://github.com/adafruit/Adafruit_CircuitPython_MLX90640

import time
import board
import busio
import adafruit_mlx90640
import numpy as np 
import cv2

# function to convert temperatures to pixels on image
def td_to_image(f, Tmin, Tmax):
	norm = np.uint8((f/100 - Tmin)*255/(Tmax-Tmin))
	norm.shape = (24,32)
	return norm


def print_frame(frame):
    for h in range(24):
        for w in range(32):
            t = frame[h*32 + w]
            print("%0.1f, " % t, end="")
        print()
    print()

def main(output_file, Tmin = 20, Tmax = 40):
    i2c = busio.I2C(board.SCL, board.SDA, frequency=800000)
    mlx = adafruit_mlx90640.MLX90640(i2c)
    print("MLX addr detected on I2C", [hex(i) for i in mlx.serial_number])
    try:
        frame = [0] * 768
        mlx.getFrame(frame)
        print_frame(frame)
        ta_img = td_to_image(frame, Tmin, Tmax)
        img = cv2.applyColorMap(ta_img, cv2.COLORMAP_JET)
        cv2.imwrite(img, output_file)
    except Exception as e:
        print("Error while capturing IR Image:", e)
        exit(1)


if __name__ == "__main__":
    main('test.jpeg')



