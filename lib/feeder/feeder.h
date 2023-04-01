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

bool isInFeed() {
    return rotator != nullptr;
}

bool isInRotation() {
    // default to saying we're in a rotation so that we fail thinking we're feeding
    if (rotationInput == nullptr) {
        Serial.println("early exit");
        return true;
    }

    return rotationInput->read();
}

void beginFeed(const int rotationCount, unsigned long startedAt) {
    if (rotator != nullptr) {
        Serial.println("Refusing to create a new rotator when one is already in flight");
    }
    auto newRotator = std::make_unique<Rotator>(rotationCount, startedAt, nsMotorPins);
    newRotator->startup();

    rotator = std::move(newRotator);
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
    rotationInput = std::make_unique<Debounce>(rotationSensorPins.input, 40, true);

    nsRotationSensorPins = rotationSensorPins;
    nsMotorPins = motorPins;
}

void loopFeeder(const unsigned long loopStartedAt) {
    const int curTimeSlice = loopStartedAt / 500;

    const bool curInRotation = isInRotation();
    const bool justFinishedRotation = wasRotating && !curInRotation;
    if (justFinishedRotation && rotator) {
        rotator->finishedARotation();

        rotator->pause();
        delay(sleepPeriodBetweenRotationsMS);

        if (rotator->isDone()) {
            finishFeed(loopStartedAt);
        } else {
            rotator->go();
        }
    }
    wasRotating = curInRotation;

    if (curTimeSlice != lastTimeSlice || justFinishedRotation) {
        Serial.print("rotationInput->read(): ");
        Serial.println(rotationInput->read());
        Serial.print("digitalRead: ");
        Serial.println(digitalRead(nsRotationSensorPins.input));

        Serial.print("curInRotation: ");
        Serial.println(curInRotation);
        Serial.print("justFinishedRotation: ");
        Serial.println(justFinishedRotation);
    }

    lastTimeSlice = curTimeSlice;
}

}  // namespace feeder
