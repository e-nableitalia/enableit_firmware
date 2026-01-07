#pragma once
#include <Arduino.h>
#include "Feature.h"

// Lightweight registry for protocol/domain features (no ownership)
class FeatureRegistry {
public:
    FeatureRegistry() : count(0) {}

    // Register a feature by pointer (does not take ownership)
    void registerFeature(Feature* feature) {
        if (!feature || count >= MAX_FEATURES) return;
        features[count++] = feature;
    }

    // Lookup a feature by name (const char*)
    Feature* getFeature(const char* name) const {
        for (size_t i = 0; i < count; ++i) {
            if (strcmp(features[i]->name(), name) == 0) {
                return features[i];
            }
        }
        return nullptr;
    }

    // Remove a feature by name
    void unregisterFeature(const char* name) {
        for (size_t i = 0; i < count; ++i) {
            if (strcmp(features[i]->name(), name) == 0) {
                // Shift remaining features down
                for (size_t j = i + 1; j < count; ++j) {
                    features[j - 1] = features[j];
                }
                --count;
                break;
            }
        }
    }

    // Clear all features
    void clear() { count = 0; }

private:
    static constexpr size_t MAX_FEATURES = 8;
    Feature* features[MAX_FEATURES];
    size_t count;
};
