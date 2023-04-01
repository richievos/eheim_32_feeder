#include <Arduino.h>
#include <TimeLib.h>

#include "feeder.h"
#include "mywifi.h"
#include "ntp.h"

// pull in all the inputs last to make sure they're only being referenced from here
#include "inputs.h"

feeder::RotationSensorPins rotationSensorPins = {
    .input = 23};

feeder::MotorPins motorPins = {
    .powerOutput = 22};

void setup() {
    richiev::connectWifi(hostname, wifiSSID, wifiPassword);

    Serial.begin(115200);
    feeder::setupFeeder(rotationSensorPins, motorPins);

    // trigger a NTP refresh
    ntp::setupNTP();
}

void loop() {
    const unsigned long loopStartedAt = millis();
    feeder::loopFeeder(loopStartedAt);

    // Don't run this when the feeder is going, because this blocks and I
    // don't want networking to affect food being dumped in
    if (!feeder::isInRotation()) {
        ntp::loopNTP();
    }
}
