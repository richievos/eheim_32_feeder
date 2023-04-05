#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <list>
#include <string>
#include <memory>

#include "web-server-renderers.h"
namespace feeder {
namespace web_server {

WebServer server(80);

struct FeedRequest {
    unsigned long asOf;
    unsigned int rotations;
};

std::unique_ptr<FeedRequest> pendingFeedRequest;

void handleRoot() {
    std::string bodyText;
    String triggered = server.arg("triggered");

    renderRoot(bodyText, std::string(triggered.c_str()));

    server.send(200, "text/html", bodyText.c_str());
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}

void handleFeed() {
    String rotationsString = server.arg("rotations");
    if (rotationsString == "") {
        Serial.println("Bad rotations!");
    }
    String asOfString = server.arg("asOf");
    
    const long asOf = atol(asOfString.c_str());
    const int rotations = atoi(rotationsString.c_str());

    if (asOf > 0 && rotations > 0) {
        auto newFeedRequest = std::make_unique<FeedRequest>();
        newFeedRequest->asOf = asOf;
        newFeedRequest->rotations = rotations;
        pendingFeedRequest = std::move(newFeedRequest);

        server.sendHeader("Location", "/?triggered=true", true);
        server.send(302, "text/plain", "triggered=true");
    } else {
        server.sendHeader("Location", "/?triggered=false", true);
        server.send(302, "text/plain", "triggered=false");
    }
}

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/trigger_feed", HTTPMethod::HTTP_POST, handleFeed);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

void loopWebServer() {
    server.handleClient();
}

std::unique_ptr<FeedRequest> retrievePendingFeedRequest() {
    if (pendingFeedRequest) {
        auto feedReq = std::move(pendingFeedRequest);
        pendingFeedRequest = nullptr;
        return feedReq;
    }
    return nullptr;
}

}  // namespace web_server
}  // namespace buff