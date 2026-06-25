#pragma once

#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>

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
      } else Number++;
    }
    
    for (PopulationConfig::SlotConfig Slot : LimiterConfig.DinoClasses) {
      int Number = *DinoNumbers.Find(FString(to_wstring(Slot.name)));
      if (Number < TotalPlayersPlaying / Slot.value) continue;
      //here we loc species and kill new spawns if they over limit
    }

    std::wstring Out = STR("\n  -> CHECKED DATA");

    TArray<IsleStructs::FTIAvailableClassData>* Classes = GameModeClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
    TArray<IsleStructs::FTIAvailableClassData>* CookedClasses = GameModeCookedClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);

    for (IsleStructs::FTIAvailableClassData ClassData : *Classes) {
      auto DinoName = ClassData.Name.ToString();
      Out += STR("\nName: ") + DinoName;
    }

    std::wstring Out += STR("\n  -> CHECKED DATA NEXT");

    for (IsleStructs::FTIAvailableClassData ClassData : *CookedClasses) {
      auto DinoName = ClassData.Name.ToString();
      Out += STR("\nAAAName: ") + DinoName;
    }

    Output::send(STR("\n{}\n"), Out);
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

/*
/mnt/nvme_one/Development/UnrealEngine/TheIsleEvrima/UE4SS/custom_mods/PopulationControl/src/updator_handler.cpp(62): error C2677: binary '+': no global operator found which takes type 'RC::Unreal::FString' (or there is no acceptable conversion)
/mnt/nvme_one/Development/UnrealEngine/TheIsleEvrima/UE4SS/deps/first/Unreal/include/Unreal/Core/Containers/Array.hpp(162): note: could be 'RC::Unreal::TIndexedContainerIterator<ContainerType,ElementType,SizeType> RC::Unreal::operator +(SizeType,RC::Unreal::TIndexedContainerIterator<ContainerType,ElementType,SizeType>)'
/mnt/nvme_one/Development/UnrealEngine/TheIsleEvrima/UE4SS/custom_mods/PopulationControl/src/updator_handler.cpp(62): note: 'RC::Unreal::TIndexedContainerIterator<ContainerType,ElementType,SizeType> RC::Unreal::operator +(SizeType,RC::Unreal::TIndexedContainerIterator<ContainerType,ElementType,SizeType>)': could not deduce template argument for 'RC::Unreal::TIndexedContainerIterator<ContainerType,ElementType,SizeType>' from 'RC::Unreal::FString'
to_string(property->GetName()).c_str()

TArray<FTIAvailableClassData> AvailableClasses
TArray<FString> BlockedPlayableClasses
*/