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

const unsigned int sleepPeriodBetweenRotationsMS = 100;
// const unsigned long DEBOUNCE_INTERVAL_MS = 300;
const unsigned long DEBOUNCE_INTERVAL_MS = 125;

// based on running it a handful of times. Seems to take ~9500-9800ms for a clean rotation
const unsigned long APPROXIMATE_ROTATION_DURATION_MS = 9900;

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
    Rotator(int rotationCount, unsigned long startedAt, const MotorPins motorPins, unsigned long expectedRotationDuration) : _rotationCount(rotationCount), _startedAt(startedAt), _motorPins(motorPins), _expectedRotationDuration(expectedRotationDuration){};

    bool finishedARotation() {
        auto endedAt = millis();

        Serial.print("Finished a rotation (");
        Serial.print(_numRotationsDone + 1);
        Serial.print(") out of (");
        Serial.print(_rotationCount);
        Serial.print(", duration=");
        Serial.print(currentRotationDuration(endedAt));
        Serial.print(", totalDuration=");
        Serial.print(endedAt - _startedAt);
        Serial.print(", currentRotationStartAt=");
        Serial.print(_currentRotationStartAt);
        Serial.println();

        _numRotationsDone++;

        return isDone();
    }

    bool shouldHaveFinishedARotation(const unsigned long asOfMS) {
        return hasStarted &&
               ((_currentRotationStartAt + _expectedRotationDuration) < asOfMS);
    }

    const bool isDone() {
        return _numRotationsDone >= _rotationCount;
    }

    void go(unsigned long asOf) {
        hasStarted = true;
        _currentRotationStartAt = asOf;
        digitalWrite(_motorPins.powerOutput, HIGH);
    }

    void startup(unsigned long asOf) {
        Serial.print("Beginning a feed! rotationCount=");
        Serial.print(_rotationCount);
        Serial.print(", startedAt=");
        Serial.print(_startedAt);
        Serial.println();

        go(asOf);
    }

    void pause() {
        digitalWrite(_motorPins.powerOutput, LOW);
    }

    const unsigned long projectedRotationEndAt() {
        return _currentRotationStartAt + _expectedRotationDuration;
    }

    const unsigned long currentRotationDuration(unsigned long asOfMS) {
        return asOfMS - _currentRotationStartAt;
    }

    void shutdown(unsigned long asOf) {
        Serial.print("Finished a feed!");
        Serial.print(" duration=");
        Serial.print(asOf - _startedAt);
        Serial.println();

        pause();
    }

   private:
    const int _rotationCount;
    const unsigned long _startedAt;
    const unsigned long _expectedRotationDuration;
    const MotorPins _motorPins;

    int _numRotationsDone = 0;
    int _currentRotationStartAt = 0;
    bool hasStarted = false;
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

void beginFeed(const int rotationCount) {
    if (rotator != nullptr) {
        Serial.println("Refusing to create a new rotator when one is already in flight");
    }
    unsigned long startedAt = millis();
    auto newRotator = std::make_unique<Rotator>(rotationCount, startedAt, nsMotorPins, APPROXIMATE_ROTATION_DURATION_MS);
    newRotator->startup(startedAt);

    rotator = std::move(newRotator);
}

void finishFeed(const unsigned long loopStartedAt) {
    if (rotator) rotator->shutdown(loopStartedAt);
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
    rotationInput = std::make_unique<Debounce>(rotationSensorPins.input, DEBOUNCE_INTERVAL_MS, true);

    nsRotationSensorPins = rotationSensorPins;
    nsMotorPins = motorPins;
}

unsigned long continueAt = 0;

void loopFeeder(const unsigned long loopStartedAt) {
    const int curTimeSlice = loopStartedAt / 300;

    const bool curInRotation = isInRotation();
    bool justFinishedRotation = wasRotating && !curInRotation;

    if (rotator) {
        if (continueAt != 0 && continueAt <= loopStartedAt) {
            continueAt = 0;
            rotator->go(millis());
        } else {
            if (!justFinishedRotation && rotator->shouldHaveFinishedARotation(loopStartedAt)) {
                Serial.print("WARNING: based on time should have finished a rotation but didn't. Forcing a rotation finish to avoid infinitely dropping food.");
                Serial.print(" duration=");
                Serial.print(rotator->currentRotationDuration(loopStartedAt));
                Serial.print(", expected_duration<=");
                Serial.print(APPROXIMATE_ROTATION_DURATION_MS);
                Serial.println();

                justFinishedRotation = true;
            }

            if (justFinishedRotation) {
                rotator->finishedARotation();

                rotator->pause();

                if (rotator->isDone()) {
                    finishFeed(loopStartedAt);
                } else {
                    continueAt = loopStartedAt + sleepPeriodBetweenRotationsMS;
                }
            }
        }
    }

    wasRotating = curInRotation;
    if (curTimeSlice != lastTimeSlice || justFinishedRotation) {
        Serial.print("\trotationInput->read()=");
        Serial.print(rotationInput->read());
        Serial.print(", digitalRead=");
        Serial.print(digitalRead(nsRotationSensorPins.input));
        Serial.print(", curInRotation=");
        Serial.print(curInRotation);
        Serial.print(", justFinishedRotation=");
        Serial.print(justFinishedRotation);
        Serial.println();
    }

    lastTimeSlice = curTimeSlice;
}

}  // namespace feeder
