#pragma once

#include <Preferences.h>

#include <functional>
#include <memory>
#include <vector>

#include "feeder-common.h"

// #include "numeric.h"

namespace feeding_store {

const char* PREFERENCE_NS = "feeder";

/************
 * I/O
 ***********/
Preferences preferences;

const size_t FEEDINGS_TO_KEEP = 200;

// +1 to avoid inserting a null pointer at the beginning of the string
const auto KEY_I_OFFSET = static_cast<unsigned char>(1);
#define ROTATIONS_KEY(i) \
    { static_cast<unsigned char>(KEY_I_OFFSET + i), 'D', 0 }
#define AS_OF_KEY(i) \
    { static_cast<unsigned char>(KEY_I_OFFSET + i), 'A', 0 }
#define INDEX_KEY \
    { 'I', 0 }

void persistFeeding(const unsigned char i, const feeder::Feeding& feeding) {
    char rotationsKey[] = ROTATIONS_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    ;

    preferences.putUInt(rotationsKey, feeding.rotations);
    preferences.putULong(asOfKey, feeding.asOfAdjustedSec);
}

feeder::Feeding readFeeding(const unsigned char i) {
    feeder::Feeding feeding;

    char rotationsKey[] = ROTATIONS_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    ;

    feeding.rotations = preferences.getUInt(rotationsKey, 0);
    feeding.asOfAdjustedSec = preferences.getULong(asOfKey, 0);

    return feeding;
}

void persistIndex(const unsigned char i) {
    char indexKey[] = INDEX_KEY;
    preferences.putUChar(indexKey, i);
}

unsigned char readIndex() {
    char indexKey[] = INDEX_KEY;
    return preferences.getUChar(indexKey);
}

/************
 * FeedingStore
 ***********/
template <size_t N>
class FeedingStore {
   private:
    std::vector<feeder::Feeding> _mostRecentFeedings;
    unsigned char _tipIndex = 0;

   public:
    FeedingStore() : _mostRecentFeedings(N) {}

    void addFeeding(const feeder::Feeding feeding, bool persist = false) {
        _mostRecentFeedings[_tipIndex] = feeding;
        _tipIndex++;
        if (_tipIndex >= N) {
            _tipIndex = 0;
        }
    };

    const std::vector<feeder::Feeding>& getFeedings() {
        return _mostRecentFeedings;
    }

    const std::vector<std::reference_wrapper<feeder::Feeding>> getFeedingsSortedByAsOf() {
        std::vector<std::reference_wrapper<feeder::Feeding>> sorted{_mostRecentFeedings.begin(), _mostRecentFeedings.end()};
        std::sort(sorted.begin(), sorted.end(),
                  [](std::reference_wrapper<feeder::Feeding>& a, std::reference_wrapper<feeder::Feeding>& b) { return a.get().asOfAdjustedSec > b.get().asOfAdjustedSec; });
        return sorted;
    }

    void
    updateTipIndex(const unsigned char tipIndex) {
        _tipIndex = tipIndex;
    }

    const unsigned char getTipIndex() { return _tipIndex; }
};

template <size_t N>
void persistFeedingStore(std::shared_ptr<FeedingStore<N>> feedingStore) {
    preferences.begin(PREFERENCE_NS, false);
    auto& feedings = feedingStore->getFeedings();
    for (unsigned char i = 0; i < feedings.size(); i++) {
        persistFeeding(i, feedings[i]);
    }

    persistIndex(feedingStore->getTipIndex());
    preferences.end();
}

#include "Arduino.h"
template <size_t N>
std::unique_ptr<FeedingStore<N>> setupFeedingStore() {
    auto feedingStore = std::make_unique<FeedingStore<N>>();

    preferences.begin(PREFERENCE_NS, true);
    for (unsigned char i = 0; i < N; i++) {
        feeder::Feeding feeding = readFeeding(i);
        if (feeding.rotations != 0) {
            feedingStore->addFeeding(feeding);
        }
    }

    auto index = readIndex();
    feedingStore->updateTipIndex(index);
    preferences.end();

    return std::move(feedingStore);
}

}  // namespace feeding_store
