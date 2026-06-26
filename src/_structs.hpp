#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace PopulationConfig {
	struct SlotConfig {
		std::string name;
		int value;
	};

	struct PopulationLimiterConfig {
		std::vector<SlotConfig> DinoClasses;
	};
}