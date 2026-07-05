#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>
#include "Containers/Array.hpp"
#include "Containers/FString.hpp"
#include "CoreUObject/UObject/Class.hpp"

#include "Structs/TheIsleStructs.hpp"

#include "_structs.hpp"

namespace PopulationControlComponent {
	using namespace RC::Unreal;

	static UClass* _GameModeBaseClass{};
	static FProperty* _GameModeAllControllers{};
	static FProperty* _GameModeClasses{};
	static FProperty* _GameModeCookedClasses{};

	static UClass* _PlayerControllerBaseClass{};
	static FProperty* _PlayerControllerPawn{};

	static UClass* _DinoClass{};
	static FProperty* _DinoClassGeneralSettings{};
	static FProperty* _DinoClassEggClutchSize{};
	static FProperty* _DinoClassGrowth{};
	static FProperty* _DinoSteamId{};

	static UFunction* _SetHealth{};

	static UObject* GameMode{};

	auto Fire(PopulationConfig::PopulationLimiterConfig LimiterConfig) -> void {
		if (!GameMode) {
			GameMode = UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C"));
			if (!GameMode) return;
		};

		int TotalPlayersPlaying{0};// No, I know I can use Num, but this mod count lobby sitters too and ooman players
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
			if (!Pawn || !Pawn->IsA(_DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			IsleStructs::ATIDinosaurBase* Dino = static_cast<IsleStructs::ATIDinosaurBase*>(Pawn);
			IsleStructs::FGeneralSettings GeneralSettings = *_DinoClassGeneralSettings->ContainerPtrToValuePtr<IsleStructs::FGeneralSettings>(Dino);
			FString DinoClassName = GeneralSettings.ClassName.ToFString();
			AsocDinoArray.Find(DinoClassName)->Add(Dino);// Not safe, may result in crash, there are edge case can be with dinoses that arent listed in CookedClasses in future

			float DinoGrowthPercent = *_DinoClassGrowth->ContainerPtrToValuePtr<float>(Dino);
			if (DinoGrowthPercent >= 0.25) continue;
			AsocDinoChildsArray.Find(DinoClassName)->Add(Dino);
		}

		TSet<FString> ExcludedClasses;
		for (PopulationConfig::SlotConfig& Slot : LimiterConfig.DinoClasses) {
			FString DinoClassName = FString(to_wstring(Slot.name));
			TArray<IsleStructs::ATIDinosaurBase*>* TargetArray = AsocDinoArray.Find(DinoClassName);
			if (!TargetArray) continue;// Safe check, just in case somebody mess up with config, if you set up it right there no chance for it to fail

			int DinoNumber = TargetArray->Num();
			if (!DinoNumber) continue;
			if (TotalPlayersPlaying / Slot.value > DinoNumber) continue;

			ExcludedClasses.Add(DinoClassName);
		}

		TArray<IsleStructs::FTIAvailableClassData>* AvailableClass = _GameModeClasses->ContainerPtrToValuePtr<TArray<IsleStructs::FTIAvailableClassData>>(GameMode);
		AvailableClass->Reset(CookedClasses->Num() - ExcludedClasses.Num());// Just in case

		for (IsleStructs::FTIAvailableClassData& ClassData : *CookedClasses) {
			FString DinoClassName = ClassData.Name.ToFString();
			if (!ExcludedClasses.Contains(DinoClassName)) {
				AvailableClass->Add(ClassData);
				continue;
			}

			TArray<IsleStructs::ATIDinosaurBase*> TargetArray = *AsocDinoChildsArray.Find(DinoClassName);// Here we for sure know this array exist, so no safe checks
			if (!TargetArray.Num()) continue;

			IsleStructs::ATIDinosaurBase* Dino = TargetArray.Top();
			uint8 ClutchSize = *_DinoClassEggClutchSize->ContainerPtrToValuePtr<uint8>(Dino);// This way ppl can nest over limit
			if (ClutchSize >= TargetArray.Num()) continue;

			float HighestGrow = 1;
			for (IsleStructs::ATIDinosaurBase* CurrentDino : TargetArray) {// Find the smallest one, so we wont be doing circle around killing the biggest one after somebody joins in
				float DinoGrowthPercent = *_DinoClassGrowth->ContainerPtrToValuePtr<float>(CurrentDino);
				if (DinoGrowthPercent > HighestGrow) continue;
				Dino = CurrentDino;
				HighestGrow = DinoGrowthPercent;
			}

			IsleStructs::FSetHealthParams SetHealthParams{0};
			Dino->ProcessEvent(_SetHealth, &SetHealthParams);
		}
	}

	auto Initialize() -> void {
		_GameModeBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIGameModeBase"));
		_GameModeAllControllers = _GameModeBaseClass->GetPropertyByNameInChain(STR("AllPlayerControllers"));// We go this way, because this way for sure we only detect active dinos, other variant provide dead too
		_GameModeClasses = _GameModeBaseClass->GetPropertyByNameInChain(STR("AvailableClasses"));// I have no idea what the fuck is going on here on game side, I tried to do how it supposed to be, but nuh uh
		_GameModeCookedClasses = _GameModeBaseClass->GetPropertyByNameInChain(STR("CookedClasses"));// They are not the same ref to objects, in upper array another set of same type of object, however nothing bad happens if I use them

		_PlayerControllerBaseClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIPlayerController"));
		_PlayerControllerPawn = _PlayerControllerBaseClass->GetPropertyByNameInChain(STR("Pawn"));// Dinos/Humans/Spectator

		_DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
		_DinoClassGeneralSettings = _DinoClass->GetPropertyByNameInChain(STR("GeneralSettings"));
		_DinoClassEggClutchSize = _DinoClass->GetPropertyByNameInChain(STR("EggClutchSize"));
		_DinoClassGrowth = _DinoClass->GetPropertyByNameInChain(STR("Growth"));
		_DinoSteamId = _DinoClass->GetPropertyByNameInChain(STR("SteamId"));

		_SetHealth = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/TheIsle.TICharacterBase:SetHealth"));// Safe way to kill it
	}
}