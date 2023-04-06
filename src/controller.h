#pragma once

#include <NTPClient.h>

#include <memory>

#include "feeder.h"
#include "mqtt.h"
#include "web-server.h"

namespace feeder {

namespace controller {

unsigned long lastFeedAsOf = 0;

void triggerFeed(const unsigned long asOf, const unsigned int rotations) {
    if (asOf <= lastFeedAsOf) {
        Serial.print("Refusing to feed because of time mismatch (idempotence check). asOf=");
        Serial.print(asOf);
        Serial.print("<= lastFeedAsOf=");
        Serial.print(lastFeedAsOf);
        Serial.println();
    } else {
        feeder::beginFeed(asOf, rotations);
    }
}

std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers() {
    auto topicsToProcessorPtr = std::make_unique<richiev::mqtt::TopicProcessorMap>();
    auto& topicsToProcessor = *topicsToProcessorPtr;

    topicsToProcessor["execute/triggerFeed"] = [&](const std::string& payload) {
        auto doc = richiev::mqtt::parseInput(payload);
        if (!doc.containsKey("rotations")) {
            return;
        }

        auto rotations = doc["rotations"].as<unsigned int>();
        auto asOf = doc.containsKey("asOf") ? doc["asOf"].as<unsigned long>() : millis();

        triggerFeed(asOf, rotations);
    };
    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

void setupController(MqttBroker& mqttBroker, MqttClient& mqttClient) {
    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers());

    web_server::setupWebServer();
    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);
}

void loopController(std::shared_ptr<NTPClient> timeClient) {
    web_server::loopWebServer();
    auto pendingFeed = web_server::retrievePendingFeedRequest();
    if (pendingFeed) {
        triggerFeed(timeClient->getEpochTime(), pendingFeed->rotations);
    }
}
}  // namespace controller
}  // namespace feeder
