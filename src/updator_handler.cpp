#pragma once

#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>

#include "Containers/Array.hpp"
#include "Containers/FString.hpp"
#include "CoreUObject/UObject/Class.hpp"
#include "_structs.hpp"
#include "Structs/TheIsleStructs.hpp"

using namespace RC::Unreal;

namespace PopulationHandler {
	static UClass* _GameModeBaseClass = nullptr;
	static FProperty* _GameModeAllControllers = nullptr;
	static FProperty* _GameModeClasses = nullptr;
	static FProperty* _GameModeCookedClasses = nullptr;

	static UClass* _PlayerControllerBaseClass = nullptr;
	static FProperty* _PlayerControllerPawn = nullptr;

	static UClass* _DinoClass = nullptr;
	static FProperty* _DinoClassGeneralSettings = nullptr;
	static FProperty* _DinoClassEggClutchSize = nullptr;
	static FProperty* _DinoClassGrowth = nullptr;
	static FProperty* _DinoSteamId = nullptr;

	static UFunction* _SetHealth = nullptr;

	auto Fire(PopulationConfig::PopulationLimiterConfig LimiterConfig) -> void {
		auto* GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
		if (!GameMode) return;

		int TotalPlayersPlaying = 0;
		TMap<FString, TArray<IsleStructs::ATIDinosaurBase*>> AsocDinoArray;
		TMap<FString, TArray<IsleStructs::ATIDinosaurBase*>> AsocDinoChildsArray;

		TArray<IsleStructs::FTIAvailableClassData>* CookedClasses = _GameModeCookedClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
		for (IsleStructs::FTIAvailableClassData& ClassData : *CookedClasses) {
			FString DinoClassName = ClassData.Name.ToFString();
			AsocDinoArray.Add(DinoClassName, TArray<IsleStructs::ATIDinosaurBase*>{});
			AsocDinoChildsArray.Add(DinoClassName, TArray<IsleStructs::ATIDinosaurBase*>{});
		}

		TArray<IsleStructs::ATIPlayerController*>* ActivePlayers = _GameModeAllControllers->ContainerPtrToValuePtr<TArray<IsleStructs::ATIPlayerController*>>(GameMode);
		for (IsleStructs::ATIPlayerController* Player : *ActivePlayers) {
			TotalPlayersPlaying++;
			IsleStructs::APawn* Pawn = *_PlayerControllerPawn->ContainerPtrToValuePtr<IsleStructs::APawn*>(Player);
			if (!Pawn || !Pawn->IsA(_DinoClass)) continue;

			IsleStructs::ATIDinosaurBase* Dino = static_cast<IsleStructs::ATIDinosaurBase*>(Pawn);
			IsleStructs::FGeneralSettings GeneralSettings = *_DinoClassGeneralSettings->ContainerPtrToValuePtr<IsleStructs::FGeneralSettings>(Dino);
			FString DinoClassName = GeneralSettings.ClassName.ToFString();
			AsocDinoArray.Find(DinoClassName)->Add(Dino);

			float DinoGrowthPercent = *_DinoClassGrowth->ContainerPtrToValuePtr<float>(Dino);
			if (DinoGrowthPercent >= 0.25) continue;
			AsocDinoChildsArray.Find(DinoClassName)->Add(Dino);
		}

		TSet<FString> ExcludedClasses;
		for (PopulationConfig::SlotConfig& Slot : LimiterConfig.DinoClasses) {
			FString DinoClassName = FString(to_wstring(Slot.name));
			TArray<IsleStructs::ATIDinosaurBase*>* TargetArray = AsocDinoArray.Find(DinoClassName);
			if (!TargetArray) continue;

			int DinoNumber = TargetArray->Num();
			if (!DinoNumber) continue;
			if (DinoNumber < TotalPlayersPlaying / Slot.value) continue;

			ExcludedClasses.Add(DinoClassName);
		}

		TArray<IsleStructs::FTIAvailableClassData>* AvailableClass = _GameModeClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
		AvailableClass->Reset(CookedClasses->Num() - ExcludedClasses.Num());

		for (IsleStructs::FTIAvailableClassData& ClassData : *CookedClasses) {
			FString DinoClassName = ClassData.Name.ToFString();
			if (ExcludedClasses.Contains(DinoClassName)) {
				TArray<IsleStructs::ATIDinosaurBase*> TargetArray = *AsocDinoChildsArray.Find(DinoClassName);
				if (!TargetArray.Num()) continue;

				IsleStructs::ATIDinosaurBase* Dino = TargetArray.Top();
				uint8 ClutchSize = *_DinoClassEggClutchSize->ContainerPtrToValuePtr<uint8>(Dino);
				if (ClutchSize >= TargetArray.Num()) continue;

				IsleStructs::FSetHealthParams Params{0};
				Dino->ProcessEvent(_SetHealth, &Params);
			} else {
				AvailableClass->Add(ClassData);
			}
		}
	}

	auto Initialize() -> void {
		_GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
		_GameModeAllControllers = _GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerControllers"));
		_GameModeClasses = _GameModeBaseClass->GetPropertyByNameInChain(STR("AvailableClasses"));
		_GameModeCookedClasses = _GameModeBaseClass->GetPropertyByNameInChain(STR("CookedClasses"));

		_PlayerControllerBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIPlayerController"));
		_PlayerControllerPawn = _PlayerControllerBaseClass->GetPropertyByNameInChain(STR("Pawn"));

		_DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
		_DinoClassGeneralSettings = _DinoClass->GetPropertyByNameInChain(STR("GeneralSettings"));
		_DinoClassEggClutchSize = _DinoClass->GetPropertyByNameInChain(STR("EggClutchSize"));
		_DinoClassGrowth = _DinoClass->GetPropertyByNameInChain(STR("Growth"));
		_DinoSteamId = _DinoClass->GetPropertyByNameInChain(STR("SteamId"));

		_SetHealth = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:SetHealth"));
	}
}