#pragma once

#include <list>
#include <string>

#include "Arduino.h"

namespace feeder {
namespace web_server {

std::string renderFooter(char *temp, size_t tempSize) {
    unsigned long time = millis();
    int sec = time / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf(temp, tempSize,
             "<footer>Uptime: %02d:%02d:%02d</footer>",
             hr, min % 60, sec % 60);
    return temp;
}

std::string renderForm(char *temp, size_t tempSize, unsigned long asOf) {
    const char *templateString = R"(
    <form action="/trigger_feed" method="post">
        <input type="number" name="rotations" id="rotations" value="1" />
        <input type="hidden" name="asOf" id="asOf" value="%ul" />
        <input type="submit" name="submit" value="Feed" />
    </form>
    )";

    snprintf(temp, tempSize,
             templateString,
             asOf);
    return temp;
}

void renderRoot(std::string &out, std::string triggered) {
    std::string triggeredContent = "";
    if (triggered == "true") {
        triggeredContent = R"(<section id="alert success"><div>Successfully triggered a feed!</div></section>)";
    } else if (triggered == "false") {
        triggeredContent = R"(<section id="alert failure"><div>Failed to trigger a feed!</div></section>)";
    }

    const int tempSize = 400;
    char temp[tempSize];
    memset(temp, 0, tempSize);

    out += R"(
<html>
  <head>
    <title>Feeder</title>
    <style>
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }
    </style>
  </head>
  <body>
    <header>
      <h1>Feeder</h1></header>
    )";

    out += triggeredContent;
    out += "</header>";

    out += renderForm(temp, tempSize, millis());
    out += renderFooter(temp, tempSize);
    out += R"(
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace feeder
