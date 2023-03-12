import mss
import cv2 as cv
import numpy as np
import time
import datetime

# reference https://www.delftstack.com/howto/python/opencv-average-color-of-image/

monitorW = 1920
monitorH = 1080
tileWidth = 600

baseImg = None
t0 = time.time()
n_frames = 1
mainMonitor = {"top": 0, "left": 0, "width": monitorW, "height": monitorH}

with mss.mss() as sct:
    while True:
        a = datetime.datetime.now()
        baseImg = sct.grab(monitor=mainMonitor)
        # Convert to NumPy array
        baseImg = np.array(baseImg)

        small = cv.resize(baseImg, (0, 0), fx=0.5, fy=0.5)
        #cv.imshow("Computer Vision", small)

        # grab 4 corner tiles, baseImg[y1,y2, x1,x2]
        tiles = []
        tiles.append(baseImg[0:tileWidth, 0:tileWidth])
        tiles.append(baseImg[0:tileWidth, monitorW-tileWidth:monitorW])
        tiles.append(baseImg[monitorH-tileWidth:monitorH, 0:tileWidth])
        tiles.append(baseImg[monitorH-tileWidth:monitorH, monitorW-tileWidth:monitorW])
        #cv.imshow("crop_top_left", crop_top_left)
        #cv.imshow("crop_top_right", crop_top_right)
        #cv.imshow("crop_bottom_left", crop_bottom_left)
        #cv.imshow("crop_bottom_right", crop_bottom_right)

        # compute average corner color
        avgColors = []
        for tile in tiles:
            tile.mean(axis=(0,1))
            avgColors.append(np.average(tile, axis=0))
        #avg_color_per_row = np.average(crop_top_left, axis=0)

        #d_img = np.zeros((250, 250, 3), dtype=np.uint8)
        #d_img[:,:,0] = avg_color[0]
        #d_img[:,:,1] = avg_color[1]
        #d_img[:,:,2] = avg_color[2]

        #cv.imshow('Source image',crop_top_left)
        #cv.imshow('Average Color',d_img)

        # Break loop and end test
        #key = cv.waitKey(1)
        #if key == ord('q'):
        #    break

        elapsed_time = time.time() - t0
        avg_fps = (n_frames / elapsed_time)
        print("Average FPS: " + str(avg_fps))
        print(datetime.datetime.now() - a)
        n_frames += 1
