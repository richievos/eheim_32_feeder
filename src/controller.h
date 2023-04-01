#include <memory>

#include "feeder.h"
#include "mqtt.h"

namespace feeder {
unsigned long lastFeedAsOf = 0;

std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers() {
    auto topicsToProcessorPtr = std::make_unique<richiev::mqtt::TopicProcessorMap>();
    auto& topicsToProcessor = *topicsToProcessorPtr;

    topicsToProcessor["execute/triggerFeed"] = [&](const std::string& payload) {
        auto doc = richiev::mqtt::parseInput(payload);
        if (!doc.containsKey("rotations")) {
            return;
        }

        auto rotations = doc["rotations"].as<int>();
        auto asOf = doc.containsKey("asOf") ? doc["asOf"].as<unsigned long>() : millis();

        if (asOf <= lastFeedAsOf) {
            Serial.print("Refusing to feed because of time mismatch (idempotence check). asOf=");
            Serial.print(asOf);
            Serial.print("<= lastFeedAsOf=");
            Serial.print(lastFeedAsOf);
            Serial.println();
        }

        feeder::beginFeed(rotations, asOf);
    };
    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

void setupController(MqttBroker& mqttBroker, MqttClient& mqttClient) {
    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers());

    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);
}
}  // namespace feeder
