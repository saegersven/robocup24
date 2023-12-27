import cv2
import numpy as np
import math

LINE_FRAME_WIDTH = 80
LINE_FRAME_HEIGHT = 48

def get_line_angle_gap(img):
    debug_img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

    # Find average position of black pixels
    x_avg = 0
    y_avg = 0
    num = 0
    for y in range(LINE_FRAME_HEIGHT):
        for x in range(LINE_FRAME_WIDTH):
            if(img[y][x] != 0):
                x_avg += x
                y_avg += y
                num += 1
    x_avg /= num
    y_avg /= num

    cv2.circle(debug_img, (int(x_avg), int(y_avg)), 2, (0, 255, 0), 2)

    # Now find average angle to center of pixel
    total_angle = 0
    num = 0
    for y in range(LINE_FRAME_HEIGHT):
        for x in range(LINE_FRAME_WIDTH):
            if(img[y][x] != 0):
                dx = x - x_avg
                dy = y - y_avg

                if(dy == 0):
                    continue

                angle = math.atan(-dx / dy)

                print(f"{dx}, {dy}, {angle}")

                total_angle += angle
                num += 1

    total_angle /= num

    line_dx = 20 * math.sin(total_angle)
    line_dy = -20 * math.cos(total_angle)
    cv2.line(debug_img, (int(x_avg - line_dx), int(y_avg - line_dy)), (int(x_avg + line_dx), int(y_avg + line_dy)),(255, 0, 0), 1)

    cv2.imshow("Debug", debug_img)
    cv2.waitKey(10000)

    return total_angle

test_img = (255 - cv2.imread("test1.png", cv2.IMREAD_GRAYSCALE))

angle = get_line_angle_gap(test_img)
print(f"Line angle: {angle}")