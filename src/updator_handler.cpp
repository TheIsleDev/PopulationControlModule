#pragma once

#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>

#include "Containers/FString.hpp"
#include "_structs.hpp"
#include "Structs/TheIsleStructs.hpp"

using namespace RC::Unreal;

namespace PopulationHandler {
  static UClass* GameModeBaseClass = nullptr;
  static FProperty* GameModeAllPlayers = nullptr;
  static FProperty* GameModeClasses = nullptr;
  static FProperty* GameModeCookedClasses = nullptr;

  static UClass* DinoClass = nullptr;
  static FProperty* DinoGeneralSettings = nullptr;

  auto Fire(PopulationConfig::PopulationLimiterConfig LimiterConfig) -> void {
    auto* GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
    if (!GameMode) return;

    int TotalPlayersPlaying = 0;
    TMap<FString, int> DinoNumbers;
    TArray<IsleStructs::ATIDinosaurBase*>* ActiveDinos = GameModeAllPlayers->ContainerPtrToValuePtr<TArray<IsleStructs::ATIDinosaurBase*>>(GameMode);
    for (IsleStructs::ATIDinosaurBase* Dino : *ActiveDinos) {
      if (!Dino || !Dino->IsA(DinoClass)) continue;

      TotalPlayersPlaying++;
      IsleStructs::FGeneralSettings GeneralSettings = *DinoGeneralSettings->ContainerPtrToValuePtr<IsleStructs::FGeneralSettings>(Dino);
      FString DinoName = GeneralSettings.ClassName.ToFString();
      int* Number = DinoNumbers.Find(DinoName);
      if(!Number) {
        DinoNumbers.Add(DinoName, 1);
      } else (*Number)++;
    }

    int Locked = 0;
    TSet<FString> ExcludedClasses;
    for (PopulationConfig::SlotConfig& Slot : LimiterConfig.DinoClasses) {
      FString DinoClassName = FString(to_wstring(Slot.name));
      int* NumberPtr = DinoNumbers.Find(DinoClassName);
      if (NumberPtr && (*NumberPtr < TotalPlayersPlaying / Slot.value)) continue;

      Locked++;
      ExcludedClasses.Add(DinoClassName);
    }

    TArray<IsleStructs::FTIAvailableClassData>* Classes = GameModeClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
    TArray<IsleStructs::FTIAvailableClassData>* CookedClasses = GameModeCookedClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
    Classes->Reset(CookedClasses->Num() - Locked);
    for (IsleStructs::FTIAvailableClassData& ClassData : *CookedClasses) {
      if (ExcludedClasses.Contains(ClassData.Name.ToFString())) continue;
      Classes->Add(ClassData);
    }
  }

  auto Initialize() -> void {
    GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
    GameModeAllPlayers = GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerCharacters"));
    GameModeClasses = GameModeBaseClass->GetPropertyByNameInChain(STR("AvailableClasses"));
    GameModeCookedClasses = GameModeBaseClass->GetPropertyByNameInChain(STR("CookedClasses"));

    DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
    DinoGeneralSettings = DinoClass->GetPropertyByNameInChain(STR("GeneralSettings"));
  }
}