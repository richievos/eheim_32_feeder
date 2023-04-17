#pragma once

#include <memory>

#include <NTPClient.h>

#include "feeder.h"
#include "feeding-store.h"
#include "mqtt.h"
#include "web-server.h"

namespace feeder {

namespace controller {

unsigned long lastFeedAsOf = 0;
std::shared_ptr<NTPClient> timeClient = nullptr;
std::shared_ptr<feeding_store::FeedingStore<feeding_store::FEEDINGS_TO_KEEP>> feedingStore = nullptr;
std::unique_ptr<feeder::web_server::FeederWebServer<feeding_store::FEEDINGS_TO_KEEP>> feedWebServer = nullptr;

void triggerFeed(const unsigned long asOf, const unsigned long adjustedTimeSec, const unsigned int rotations) {
    if (asOf <= lastFeedAsOf) {
        Serial.print("Refusing to feed because of time mismatch (idempotence check). asOf=");
        Serial.print(asOf);
        Serial.print("<= lastFeedAsOf=");
        Serial.print(lastFeedAsOf);
        Serial.println();
    } else {
        const feeder::Feeding feeding = {
            .asOfAdjustedSec = adjustedTimeSec,
            .rotations = rotations
        };
        feeder::beginFeed(asOf, adjustedTimeSec, rotations);
        feedingStore->addFeeding(feeding);
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

        triggerFeed(asOf, timeClient->getEpochTime(), rotations);
    };
    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

void setupController(MqttBroker& mqttBroker, MqttClient& mqttClient, std::shared_ptr<NTPClient> tc) {
    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers());
    timeClient = tc;

    feedingStore = std::move(feeding_store::setupFeedingStore<feeding_store::FEEDINGS_TO_KEEP>());

    feedWebServer = std::make_unique<feeder::web_server::FeederWebServer<feeding_store::FEEDINGS_TO_KEEP>>(feedingStore);
    feedWebServer->setupWebServer();
    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);
}

void loopController() {
    feedWebServer->loopWebServer();
    auto pendingFeed = feedWebServer->retrievePendingFeedRequest();
    if (pendingFeed) {
        triggerFeed(millis(), timeClient->getEpochTime(), pendingFeed->rotations);
    }
}
}  // namespace controller
}  // namespace feeder
