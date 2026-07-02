#ifndef DEVICEOS_CORE_RULES_ENGINE_H
#define DEVICEOS_CORE_RULES_ENGINE_H

#include <vector>
#include <string>
#include <functional>

namespace DeviceOS {

    struct Rule {
        std::string sensorName;
        float threshold;
        bool triggerAbove; // true if triggers when value > threshold
        std::function<void()> action;
    };

    class RulesEngine {
    public:
        RulesEngine() = default;
        ~RulesEngine() = default;

        void addRule(const Rule& rule);
        void evaluate(const std::string& sensorName, float value);

    private:
        std::vector<Rule> rules;
    };

} // namespace DeviceOS

#endif // DEVICEOS_CORE_RULES_ENGINE_H
