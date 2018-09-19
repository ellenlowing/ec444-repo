# Pseudo-mechanical clock
Author: Ellen Lo, 2019-09-18

## Summary
I designed an alarm clock which has the following hardware functionalities:
- pushbutton to restart alarm
- led to alert alarm going off
- numerical display to show current time (hour and minute)
- servos as second hand and minute hand

## Implementation
To restart alarm, user presses pushbutton and hence setting the reset flag. Once reset flag is set, two timers (*TIMER_0* goes off at preset alarming period and *TIMER_1* goes off every second) start counting. The program keeps track of current time with 2 variables (*minute_count* and *second_count*) and controls minute hand and second hand via 2 servos respectively. As servo range is 90 degrees, each second tick increments servo angle by 1.5 degree until hitting 60 second limit. Once the time hits 60 seconds, minute hand increments by 15 degrees and second hand returns to its zero position. When time hits preset alarm interval, red LED blinks, reset flag is reset, and the program simply waits for user to start alarm again.

## Sketches and Photos
<!-- ### Wiring
<center><img src="./img/IMG_2259.jpeg" width="80%" /></center>

### Console
<center><img src="./img/console.png" width="80%" /></center>
Enter 4-digit binary number. Each digit corresponds to 1 LED. -->

## Modules, Tools, Source Used in Solution


## Supporting Artifacts
-[Video Demo]()
