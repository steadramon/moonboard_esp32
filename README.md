# MOONBOARD LED SYSTEM ESP32

This repo contain an Arduino sketch implementing the moonboard LED system for an Espressif ESP32.

## Part list

- 200 (4x50) WS2811 
- ESP32
- 3A 5V usb power supply or  5V (> 15W) power supply
- wire + wire connectors

## Led stripes connection

Beware that LED stripes wire colors can be different.  Soldering necessary

- Data (yellow green) -> PIN2
- Ground (blue) -> Gnd
- Power (brown) -> USB PIN

## Power

if using a USB power adapter
- via USB connector

if using an external power supply
- GND to Gnd (parallel to led stripes)
- +5V to USB (parallel to led stripes)

## TODO

- bring out a RESET  or forcer DISCONNECT  button.
- implement different LED layouts - WiFi control panel?
- HTML based lights on/off
- set led brigntness 
- implement led test
