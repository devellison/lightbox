#!/usr/bin/env python3
import os
import sys
import numpy
import cv2
import platform

zebral_test_path = '../'
zebral_format = 'YUYV'

if (platform.system() == "Windows"):
    zebral_test_path = '..\\Release\\'
    zebral_format = 'YUY2'

# Assumes is run in build/python/test with the module in build/python
# This is what happens during normal CTest
sys.path.insert(0, zebral_test_path)

import zebralpy

camMgr = zebralpy.CameraManager()
cameraList = camMgr.Enumerate()
retVal = 0
for curCamera in cameraList:
    print(curCamera)
    camera = camMgr.Create(curCamera)

    format = zebralpy.FormatInfo()
    decodeType = zebralpy.Camera.DecodeType.INTERNAL
    format.width = 640
    format.height = 480
    format.fps = 30
    format.format = zebral_format
    camera.SetFormat(format, decodeType)

    camera.Start(None)
    frame = camera.GetNewFrame(5000)
    if (frame):
        bgr = frame.data()
        bgr = bgr.reshape(frame.height(), frame.width(), frame.channels())
        # Let's not in unit tests, but this shows the image is ok
        #cv2.imshow(curCamera.name,bgr)
        #cv2.waitKey(1000)
    else:
        retVal = 1
    camera.Stop()
exit(retVal)
