#include <Arduino.h>
#include <NTPClient.h>

#include "feeder.h"
#include "mqtt.h"
#include "mywifi.h"
#include "ntp.h"
#include "controller.h"

// pull in all the inputs last to make sure they're only being referenced from here
#include "inputs.h"

namespace feeder {
/*******************************
 * Shared vars
 *******************************/
MqttBroker mqttBroker(MQTT_BROKER_PORT);
MqttClient mqttClient(&mqttBroker);

std::shared_ptr<NTPClient> timeClient;

RotationSensorPins rotationSensorPins = {
    .input = 23};

MotorPins motorPins = {
    .powerOutput = 22};

void setup() {
    Serial.begin(115200);

    richiev::connectWifi(hostname, wifiSSID, wifiPassword);
    controller::setupController(mqttBroker, mqttClient);

    setupFeeder(rotationSensorPins, motorPins);

    // trigger a NTP refresh
    timeClient = std::move(ntp::setupNTP());
}

void loop() {
    const unsigned long loopStartedAt = timeClient->getEpochTime();
    loopFeeder(loopStartedAt);

    // Don't run this when the feeder is going, because this blocks and I
    // don't want networking to affect food being dumped in
    if (!isInFeed()) {
        richiev::mqtt::loopMQTT(mqttBroker, feeder::mqttClient);
        controller::loopController();

        ntp::loopNTP(timeClient);
    }
}
}  // namespace feeder

void setup() { feeder::setup(); }
void loop() { feeder::loop(); }
