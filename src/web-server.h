#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <list>
#include <memory>
#include <string>

#include "feeding-store.h"
#include "web-server-renderers.h"

namespace feeder {
namespace web_server {

struct FeedRequest {
    unsigned long asOf;
    unsigned int rotations;
};

template <size_t N>
class FeederWebServer {
   private:
    WebServer _server;
    std::shared_ptr<feeding_store::FeedingStore<N>> _feedStore;
    std::unique_ptr<FeedRequest> _pendingFeedRequest;

   public:
    FeederWebServer(std::shared_ptr<feeding_store::FeedingStore<N>> feedStore) : _feedStore(feedStore), _server(80) {}

    void handleRoot() {
        std::string bodyText;
        const String triggered = _server.arg("triggered");
        const auto &mostRecentReadings = _feedStore->getFeedingsSortedByAsOf();

        renderRoot(bodyText, std::string(triggered.c_str()), mostRecentReadings);

        _server.send(200, "text/html", bodyText.c_str());
    }

    void handleNotFound() {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += _server.uri();
        message += "\nMethod: ";
        message += (_server.method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += _server.args();
        message += "\n";

        for (uint8_t i = 0; i < _server.args(); i++) {
            message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
        }

        _server.send(404, "text/plain", message);
    }

    void handleFeed() {
        String rotationsString = _server.arg("rotations");
        if (rotationsString == "") {
            Serial.println("Bad rotations!");
        }
        String asOfString = _server.arg("asOf");

        const long asOf = atol(asOfString.c_str());
        const int rotations = atoi(rotationsString.c_str());

        if (asOf > 0 && rotations > 0) {
            auto newFeedRequest = std::make_unique<FeedRequest>();
            newFeedRequest->asOf = asOf;
            newFeedRequest->rotations = rotations;
            _pendingFeedRequest = std::move(newFeedRequest);

            _server.sendHeader("Location", "/?triggered=true", true);
            _server.send(302, "text/plain", "triggered=true");
        } else {
            _server.sendHeader("Location", "/?triggered=false", true);
            _server.send(302, "text/plain", "triggered=false");
        }
    }

    void setupWebServer() {
        _server.on("/", [&]() { handleRoot(); });
        _server.on("/trigger_feed", HTTPMethod::HTTP_POST, [&]() { handleFeed(); });
        _server.onNotFound([&]() { handleNotFound(); });
        _server.begin();
        Serial.println("HTTP server started");
    }

    void loopWebServer() {
        _server.handleClient();
    }

    std::unique_ptr<FeedRequest> retrievePendingFeedRequest() {
        if (_pendingFeedRequest) {
            auto feedReq = std::move(_pendingFeedRequest);
            _pendingFeedRequest = nullptr;
            return feedReq;
        }
        return nullptr;
    }
};

}  // namespace web_server
}  // namespace feeder
