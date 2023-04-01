#pragma once

#include <Arduino.h>

#include <memory>

#include "Debounce.h"

namespace feeder {
struct RotationSensorPins {
    int input;
};

struct MotorPins {
    int powerOutput;
};

/************************
 * Config (via setup)
 ************************/
RotationSensorPins nsRotationSensorPins;
MotorPins nsMotorPins;

const unsigned int sleepPeriodBetweenRotationsMS = 500;

/************************
 * State
 ************************/
bool wasRotating = false;
unsigned long lastTimeSlice = 0;

/************************
 * Rotation management
 ************************/
class Rotator {
   public:
    Rotator(int rotationCount, unsigned long startedAt, const MotorPins motorPins) : _rotationCount(rotationCount), _startedAt(startedAt), _motorPins(motorPins){};

    bool finishedARotation() {
        Serial.print("Finished a rotation (");
        Serial.print(_numRotationsDone + 1);
        Serial.print(") out of (");
        Serial.print(_rotationCount);
        Serial.println(")");

        _numRotationsDone++;

        return isDone();
    }

    const bool isDone() {
        return _numRotationsDone >= _rotationCount;
    }

    void go() {
        digitalWrite(_motorPins.powerOutput, HIGH);
    }

    void startup() {
        Serial.print("Beginning a feed! rotationCount=");
        Serial.print(_rotationCount);
        Serial.println(")");

        go();
        // HACK to make sure the rotation sensor works (debounces?)
        sleep(1);
    }

    void pause() {
        digitalWrite(_motorPins.powerOutput, LOW);
    }

    void shutdown() {
        Serial.println("Finished a feed!");
        pause();
    }

   private:
    const int _rotationCount;
    const unsigned long _startedAt;
    const MotorPins _motorPins;

    int _numRotationsDone = 0;
};

std::unique_ptr<Rotator> rotator = nullptr;
std::unique_ptr<Debounce> rotationInput = nullptr;

bool isInRotation() {
    // default to saying we're in a rotation so that we fail thinking we're feeding
    if (rotationInput == nullptr) {
        Serial.println("early exit");
        return true;
    }

    return rotationInput->read() == LOW;
    // const int rotationSensorVal = digitalRead(nsRotationSensorPins.input);

    // return rotationSensorVal == 0;
}

std::unique_ptr<Rotator> beginFeed(const int rotationCount, unsigned long startedAt) {
    rotator = std::make_unique<Rotator>(2, startedAt, nsMotorPins);
    rotator->startup();
    return std::move(rotator);
}

void finishFeed(const unsigned long loopStartedAt) {
    if (rotator) rotator->shutdown();
    rotator = nullptr;

    if (rotationInput) {
        Serial.print("Total rotations since reboot: rotation_count=");
        Serial.print(rotationInput->read());
        Serial.print(", time_since_reboot=");
        Serial.print(loopStartedAt);
        Serial.println();
    }
}


/************************
 * Setup & Loop
 ************************/
void setupFeeder(const RotationSensorPins rotationSensorPins, const MotorPins motorPins) {
    digitalWrite(motorPins.powerOutput, LOW);
    pinMode(motorPins.powerOutput, OUTPUT);

    pinMode(rotationSensorPins.input, INPUT_PULLUP);
    rotationInput = std::make_unique<Debounce>(rotationSensorPins.input, 10, true);

    nsRotationSensorPins = rotationSensorPins;
    nsMotorPins = motorPins;

    // TODO: temporary
    rotator = beginFeed(2, 0);
}

void loopFeeder(const unsigned long loopStartedAt) {
    const int curTimeSlice = loopStartedAt / 500;

    const bool curInRotation = isInRotation();
    const bool justFinishedRotation = wasRotating && !curInRotation;
    if (justFinishedRotation) {
        rotator->finishedARotation();

        rotator->pause();
        delay(sleepPeriodBetweenRotationsMS);

        if (rotator->isDone()) {
            finishFeed(loopStartedAt);

            // TODO: temp
            rotator = beginFeed(3, loopStartedAt);
        } else {
            rotator->go();
        }
    }
    wasRotating = curInRotation;

    // if (curTimeSlice != lastTimeSlice || justFinishedRotation) {
    //     Serial.print("rotationInput->read(): ");
    //     Serial.println(rotationInput->read());
    //     Serial.print("digitalRead: ");
    //     Serial.println(digitalRead(nsRotationSensorPins.input));

    //     Serial.print("curInRotation: ");
    //     Serial.println(curInRotation);
    //     Serial.print("justFinishedRotation: ");
    //     Serial.println(justFinishedRotation);
    // }

    lastTimeSlice = curTimeSlice;
}

}  // namespace feeder
