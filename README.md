# Eheim ESP32 Feeder
This is used to convert an Eheim feeder to be powered by an ESP32. All the
Eheim parts were replaced with an ESP32.

Why? The primary reason is because I was trying to do a mod to my feeder and I
completely shorted out all the electronics. Since I have a variety of ESP style
microcontrollers on hand, I swapped out the insides and just kept the motor and
body parts.

## Parts

Pieces:
* Eheim autofeeder ([example](https://www.amazon.com/Everyday-Feeder-Programmable-Automatic-Dispenser/dp/B001F2117I/))
* One ESP32 controller ([example](https://www.amazon.com/gp/product/B0B764963C/)
* One USB cord
* One transistor eg a 2N3904 ([example kit](https://www.amazon.com/dp/B07G46LNCG))

## Wiring instructions

**Motor and transistor:**
The motor has 2 wires, red and black.

* Black motor wire => any ground pin on the ESP32
* Transistor -- this sends 3.3V to the motor, controlled on/off by the ESP32:
    * _collector_ leg => the 3.3V output on the ESP32 (5V output likely works too)
    * _base_ leg => GPIO 22 on the ESP32 (`motorPins.powerOuput` in main.cpp)
    * _emitter_ leg => the motor's red wire

**Rotation Sensor**:
The rotation sensor also has 2 wires (red and black). This sensor is how we tell a rotation finished.

* Black sensor wire => any ground pin on the ESP32
* Red sensor wire => GPIO 23 on the ESP32 (`rotationSensorPins.input` in main.cpp)