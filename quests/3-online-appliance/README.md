# Online appliance
Author: Ellen Lo, 2018-10-27

## Summary

## How to run
Given that server port is 1111 and esp32 port is 80,

1. to start node server, run `node server.js` under /mac-server
2. to start python client, run `python2 client.py smartsystems.ddns.net 80 1111` under /httpd-client
3. to run httpd server on ESP32, run `make flash` under /httpd-server
4. to control LED, browse `http://smartsystems.ddns.net:1111/`

## Implementation


## Sketches and Photos
### Mobile client interface


## Modules, Tools, Source Used in Solution
<!-- -[my modular program for alphanumeric display](https://github.com/BU-EC444/Lo-Ellen/tree/master/skills/3-sensor-actuator/Code/alpha-display) -->

## Supporting Artifacts
<!-- -[Video Demo](https://youtu.be/CzaMl4gHX_E) -->
