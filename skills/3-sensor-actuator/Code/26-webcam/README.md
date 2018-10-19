#  Webcam on raspberry pi

Author: Ellen Lo, 2018-10-19

## Summary
In this skill assignment, I was able to set up a webcam server on raspberry pi with Motion. Motion was easy to set up with this [guide](https://pimylifeup.com/raspberry-pi-webcam-server/). Webcam output can be viewed on the (pi's IP address + ':8081') via web browser. The first try failed because the stream was freezing. So, reading from the same guide, I changed the "output_pictures" and "ffmpeg_output_movies" properties in configuration file (/etc/motion/motion.conf) and the stream did not freeze anymore. It was able to capture movements, but only at a really slow frame rate.

## Photos
### Webcam stream preview
<center><img src=".webcam.png" width="60%"/></center>

## Modules, Tools, Source Used in Solution
-[Build a Raspberry Pi Webcam Server in Minutes](https://pimylifeup.com/raspberry-pi-webcam-server/)

## Supporting Artifacts
-[Video Demo](https://youtu.be/ikgtoRGMxGM)
