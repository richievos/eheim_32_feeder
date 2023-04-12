#pragma once

#include <list>
#include <string>

#include "Arduino.h"

namespace feeder {
namespace web_server {

std::string renderFooter(char *temp, size_t bufferSize) {
    unsigned long time = millis();
    int sec = time / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf(temp, bufferSize,
             "<footer>Uptime: %02d:%02d:%02d</footer>",
             hr, min % 60, sec % 60);
    return temp;
}

std::string renderForm(char *temp, size_t bufferSize, unsigned long asOf) {
    std::string formTemplate = R"(
      <section class="row">
        <form class="form-inline row row-cols-lg-auto align-items-center" action="/trigger_feed" method="post">
          <input type="hidden" name="asOf" id="asOf" value="%u"/>

          <div class="col-12 form-floating">
            <input type="number" class="form-control" name="rotations" id="rotations" value="1" />
            <label for="rotations">Rotations</label>
          </div>

          <div class="col-12">
            <button class="btn btn-primary" type="submit">Feed</button>
          </div>
        </form>
      </section>
    )";

    snprintf(temp, bufferSize, formTemplate.c_str(), asOf);
    return temp;
}

void renderRoot(std::string &out, std::string triggered) {
    std::string triggeredContent = "";
    if (triggered == "true") {
        triggeredContent = R"(<section class="alert alert-success">Successfully triggered a feed!</section>)";
    } else if (triggered == "false") {
        triggeredContent = R"(<section class="alert alert-warning">Failed to trigger a feed!</section>)";
    }

    const int bufferSize = 1024;
    char temp[bufferSize];
    memset(temp, 0, bufferSize);

    out += R"(
<!doctype html>
<html lang="en">
  <head>
    <title>Feeder</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css" />
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <meta charset="utf-8">
  </head>
  <body>
    <div class="container-fluid">
      <header class="row">
        <div class="col">
          <h1 id="pageTitle">Feeder</h1>
        </div>
      </header>
    )";

    out += triggeredContent;

    out += renderForm(temp, bufferSize, millis());
    out += renderFooter(temp, bufferSize);
    out += R"(
    </div>
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace feeder
