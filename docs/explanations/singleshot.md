# Singleshot Control

The following sets out the correct way for higher level control to run singleshot
acquisitions. The following uses an example of 1 second exposures for the real data and
0.1 seconds exposures for the fast optimisation scan.

1. Start the file writer with N frames
2. Set singleshot mode
3. Start singleshot
  - Transition to WAITING
4. Set the timeout to 1 second + necessary overhead
5. Disable file writing on the detector
6. Acquire 10 frames on the detector
  - The filters will be adjusted until
    - The attenuation is optimal - i.e. a frame is received that does not change the attenuation
    - The attenuation reaches 0
  - Transition to SINGLESHOT_COMPLETE
7. The system will maintain the optimised attenuation until the timeout expires, this is the time allowed to capture a single image and start the next singleshot run
  a. Enable file writing on the detector
  b. Acquire 1 frame on the detector
8. Start singleshot, which will return the attenuation to maximum and wait (forever) for more data
  - Transition to WAITING
9. Reset the frame counter, so that frames starting from 0 are processed again
10. Repeat
