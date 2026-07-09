#include <Mod/CppUserModBase.hpp>

#include "Config/config_reader.hpp"

#include "_structs.hpp"
#include "handler.cpp"

class PopulationControl : public RC::CppUserModBase {
private:
	PopulationConfig::PopulationLimiterConfig LimiterConfig;

	int TicksFired{0};
	static constexpr int PerTicksFired{1440};// 120 per sec, 30 game ticks

public:
	PopulationControl() : CppUserModBase() {
		ModName = STR("PopulationControl");
		ModVersion = STR("1.0");
		ModDescription = STR("Hehe");
		ModAuthors = STR("Shiza");
	}

	auto on_unreal_init() -> void override {
		ModConfigReader::LoadModConfig(&LimiterConfig);

		PopulationControlComponent::Initialize();
	}

	auto on_update() -> void override {
		if (++TicksFired < PerTicksFired) return;
		TicksFired = 0;

		PopulationControlComponent::Fire(LimiterConfig);
	}
};

#define MOD_API __declspec(dllexport)
extern "C" {
	MOD_API RC::CppUserModBase* start_mod() {
		return new PopulationControl();
	}

	MOD_API void uninstall_mod(RC::CppUserModBase* mod) {
		delete mod;
	}
}