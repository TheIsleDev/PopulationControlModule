
#pragma once

#include <Unreal/FString.hpp>
#include <Unreal/Hooks/Hooks.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>
#include <TheIsle/_simple_structs.hpp>
#include <TheIsle/ATIPlayerController.hpp>
#include <TheIsle/ATIGameModeBase.hpp>
#include <PopulationConfig.hpp>


using namespace RC::Unreal;
using namespace RC::Unreal::TheIsle;

class PopulationControlSubsystem {
private:
	/// How often it fires
	int TicksFired = 0;
	static constexpr int TickRate{30};

	PopulationControlConfig* Config;
	Hook::GlobalCallbackId FireCallBackID;

public:
	PopulationControlSubsystem(PopulationControlConfig* SharedConfig);
	~PopulationControlSubsystem();

	// Добавить в будущем что оно не тикает а регает калбэки -> OnACharacterDied / PlayerCharacterDied и OnPlayerRespawned
	void Tick(float DeltaSeconds, bool bIdleMode);
};
