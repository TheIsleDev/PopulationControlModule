
#pragma once

#include <Mod/CppUserModBase.hpp>
#include <Subsystem.hpp>
#include <PopulationConfig.hpp>


class PopulationControlSystem : public RC::CppUserModBase {
private:
	PopulationControlConfig Config{};

public:
	std::unique_ptr<PopulationControlSubsystem> TickingPopulationControl{};

    PopulationControlSystem();
	~PopulationControlSystem() override;

	void on_unreal_init() override;
};
