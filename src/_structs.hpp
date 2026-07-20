#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace PopulationConfig {
	struct SlotConfig {
		std::string name;
		int value{0};// How much players required per one slot
		int clutch_value{0};// How much players required per one clutch slot
	};

	struct PopulationLimiterConfig {
		std::vector<SlotConfig> DinoClasses;
		float spawn_growth{0.25f};
	};
}