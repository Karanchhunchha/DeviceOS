#include "core/RulesEngine.h"
#include <iostream>

namespace DeviceOS {

    void RulesEngine::addRule(const Rule& rule) {
        rules.push_back(rule);
    }

    void RulesEngine::evaluate(const std::string& sensorName, float value) {
        for (const auto& rule : rules) {
            if (rule.sensorName == sensorName) {
                if (rule.triggerAbove && value > rule.threshold) {
                    std::cout << "[RulesEngine] Rule triggered for " << sensorName 
                              << ": " << value << " > " << rule.threshold << std::endl;
                    if (rule.action) rule.action();
                } else if (!rule.triggerAbove && value < rule.threshold) {
                    std::cout << "[RulesEngine] Rule triggered for " << sensorName 
                              << ": " << value << " < " << rule.threshold << std::endl;
                    if (rule.action) rule.action();
                }
            }
        }
    }

} // namespace DeviceOS
