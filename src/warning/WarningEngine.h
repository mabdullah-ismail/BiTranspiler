#pragma once
#include <vector>
#include <string>
#include <iostream>

class WarningEngine {
private:
    std::vector<std::string> warnings;

public:
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }

    void addWarnings(const std::vector<std::string>& newWarnings) {
        warnings.insert(warnings.end(), newWarnings.begin(), newWarnings.end());
    }

    const std::vector<std::string>& getWarnings() const {
        return warnings;
    }

    bool hasWarnings() const {
        return !warnings.empty();
    }

    void clear() {
        warnings.clear();
    }

    void printSummary() const {
        if (warnings.empty()) {
            std::cout << "\n>>> [SUCCESS] Transpilation completed with 0 warnings!\n";
        } else {
            std::cout << "\n>>> [WARNINGS] Transpilation completed with " << warnings.size() << " warning(s):\n";
            for (const auto& w : warnings) {
                std::cout << "  - " << w << "\n";
            }
        }
    }
};
