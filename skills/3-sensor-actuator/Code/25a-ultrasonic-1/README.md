#  Ultrasonic range - 1

Author: Ellen Lo, 2019-10-01

## Summary
In this skill assignment, I was able to get distance measurement with ultrasonic sensor. I chose to read analog outputs instead of using UART and PWM in this example. A distance value in millimeters is updated and displayed on console every 2 seconds.

Although specs sheet says minimum distance is 30cm away from bottom of chip, tests show that the minimum distance is not so accurate. It could be about 40cm.

## Sketches and Photos
### Console
<!-- <center><img src="./img/console.png" width="50%" /></center> -->

### Wiring
<!-- <center><img src="./img/IMG_2393.jpeg" width="50%" /></center> -->
<!-- See console image for commands. -->
Pin 6 of range finder is wired to 3v power, pin 7 to ground, and pin 3 to analog input A5 of ESP32 (GPIO #4).

## Modules, Tools, Source Used in Solution
-[esp-idf adc example](https://github.com/espressif/esp-idf/tree/affe75a10250564353d088f6b9a74dbb6f1ea0df/examples/peripherals/adc)

-[Maxbotics Range Finder Specs](https://www.maxbotix.com/documents/HRLV-MaxSonar-EZ_Datasheet.pdf)


## Supporting Artifacts
-[Video Demo](https://youtu.be/Jy_FZ1tpyeg)
