#pragma once

#include <ctime>
#include <list>
#include <string>

#include "Arduino.h"

namespace feeder {
namespace web_server {

std::string renderTime(char *temp, size_t bufferSize, const unsigned long timeInSec) {
    // millis to time
    const time_t rawtime = (time_t)timeInSec;
    struct tm *dt = gmtime(&rawtime);

    // format
    strftime(temp, bufferSize, "%Y-%m-%d %H:%M:%S", dt);
    return temp;
}

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

std::string renderMeasurementList(char *temp, size_t bufferSize, const std::vector<std::reference_wrapper<feeder::Feeding>> &mostRecentFeedings) {
    std::string measurementString = R"(<section class="row mt-3"><div class="col"><table class="table table-striped">)";
    const auto alkMeasureTemplate = R"(
      <tr class="measurement">
        <td class="asOfAdjustedSec converted-time" data-epoch-sec="%lu">%s</td>
        <td class="rotations">%u</td>
      </tr>
    )";
    for (auto &feedingRef : mostRecentFeedings) {
        auto &feeding = feedingRef.get();
        if (feeding.asOfAdjustedSec != 0) {
            snprintf(temp, bufferSize, alkMeasureTemplate,
                     feeding.asOfAdjustedSec,
                     renderTime(temp, bufferSize, feeding.asOfAdjustedSec).c_str(),
                     feeding.rotations);
            measurementString += temp;
        }
    }
    measurementString += "</table></div></section>";
    return measurementString;
}

void renderRoot(std::string &out, const std::string &triggered, const std::vector<std::reference_wrapper<feeder::Feeding>> &mostRecentFeedings) {
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
    out += renderMeasurementList(temp, bufferSize, mostRecentFeedings);
    out += renderFooter(temp, bufferSize);
    out += R"(
    </div>

    <script src="https://code.jquery.com/jquery-3.6.4.slim.min.js" integrity="sha256-a2yjHM4jnF9f54xUQakjZGaqYs/V1CYvWpoqZzC2/Bw=" crossorigin="anonymous"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ENjdO4Dr2bkBIFxQpeoTz1HIcje39Wm4jDKdf19U8gI4ddQ3GYNS7NTKfAdVQSZe" crossorigin="anonymous"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/luxon/3.3.0/luxon.min.js" integrity="sha512-KKbQg5o92MwtJKR9sfm/HkREzfyzNMiKPIQ7i7SZOxwEdiNCm4Svayn2DBq7MKEdrqPJUOSIpy1v6PpFlCQ0YA==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
    <script>
      $('.converted-time').each(function(index, item) {
        const { DateTime } = luxon;
        var epochSec = $(item).data("epoch-sec");
        var timeString = DateTime.fromSeconds(epochSec).toFormat('yyyy-MM-dd HH:mm:ss');
        $(item).text(timeString)
      })
    </script>
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace feeder
