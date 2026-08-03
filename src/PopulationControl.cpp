
// Declare local debugging if you want more logging on test run
//#define LOCAL_DEBUGGING

#include <TheIsleHelpers/ConfigReader.hpp>
#include <PopulationControl.hpp>


PopulationControlSystem::PopulationControlSystem() {
	ModName = STR("PopulationControl");
	ModVersion = STR("1.0.2");
	ModDescription = STR("Hehe");
	ModAuthors = STR("Shiza");

	RC::ConfigLoader::LoadModConfig(&Config);
}

PopulationControlSystem::~PopulationControlSystem() {
	delete TickingPopulationControl;
}


void PopulationControlSystem::on_unreal_init() {
	static PopulationControlSubsystem Ticker{&Config};
	TickingPopulationControl = &Ticker;
}


#define MOD_API __declspec(dllexport)
extern "C" {
	MOD_API RC::CppUserModBase* start_mod() {
		return new PopulationControlSystem();
	}

	MOD_API void uninstall_mod(RC::CppUserModBase* mod) {
		delete mod;
	}
}
