#!/usr/bin/env python3
import os
import sys
import numpy
import cv2
import platform

# This is the path we think our binding module is in.
# This file is meant to run in automated unit tests.
zebral_test_path = '../'
zebral_format = 'YUYV'

if (platform.system() == "Windows"):
    zebral_test_path = '../Release/'
    zebral_format = 'YUY2'

# Assumes is run in build/python/test with the module in build/python
# This is what happens during normal CTest
sys.path.insert(0, zebral_test_path)

import zebralpy

retVal = 0
camMgr = zebralpy.CameraManager()
cameraList = camMgr.Enumerate()
# We can support others, esp. with system decoding,
# but it would make the reshape more complex.
easyFormats = {'YUYV','NV12','YUY2'}

for curCamera in cameraList:
    print(curCamera)
    camera = camMgr.Create(curCamera)

    format = zebralpy.FormatInfo()
    decodeType = zebralpy.Camera.DecodeType.INTERNAL
    format.width = 640
    format.height = 480
    format.fps = 30
    
    # Skip more complex formats for now in test.
    if (format.format not in easyFormats):
        continue
    
    format.format = zebral_format
    camera.SetFormat(format, decodeType)

    camera.Start(None)
    frame = camera.GetNewFrame(5000)
    if (frame):
        bgr = frame.data()
        bgr = bgr.reshape(frame.height(), frame.width(), frame.channels())
        # Let's no try popping up images in smoke tests, 
        # but this shows the image is ok
        #cv2.imshow(curCamera.name,bgr)
        #cv2.waitKey(1000)
    else:
        retVal = 1
    camera.Stop()
exit(retVal)
