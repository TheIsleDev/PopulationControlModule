#include <Unreal/AActor.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/ContainerAllocationPolicies.hpp>
#include <Containers/Array.hpp>
#include <Containers/FString.hpp>
#include <CoreUObject/UObject/Class.hpp>

#include <Reflection/_include_custom.hpp>

#include "_structs.hpp"

namespace PopulationControlComponent {
	using namespace RC::Unreal;

	static PopulationConfig::PopulationLimiterConfig LoadedConfig;

	static UClass* _DinoClass{};
	static ATIGameModeBase* GameMode{};

	auto Fire() -> void {
		if (!GameMode) {
			GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));
			if (!GameMode) return;
		};

		int TotalPlayersPlaying{0};// No, I know I can use Num, but this mod count lobby sitters too and ooman players
		TMap<FString, TArray<ATIDinosaurBase*>> AsocDinoArray;
		TMap<FString, TArray<ATIDinosaurBase*>> AsocDinoChildsArray;

		TArray<FTIAvailableClassData> CookedClasses = GameMode->GetCookedClasses();
		for (FTIAvailableClassData& ClassData : CookedClasses) {
			FString DinoClassName = ClassData.Name.ToFString();
			AsocDinoArray.Add(DinoClassName, TArray<ATIDinosaurBase*>{});
			AsocDinoChildsArray.Add(DinoClassName, TArray<ATIDinosaurBase*>{});
		}

		TSet<ATIPlayerController*> ActivePlayers = GameMode->GetAllPlayerControllers();
		for (ATIPlayerController* Player : ActivePlayers) {
			TotalPlayersPlaying++;
			APawn* Pawn = Player->GetPawn();
			if (!Pawn || !Pawn->IsA(_DinoClass)) continue;// Make sure it's actually dino, not a fucking damn human

			ATIDinosaurBase* Dino = static_cast<ATIDinosaurBase*>(Pawn);
			FGeneralSettings GeneralSettings = Dino->GetGeneralSettings();
			FString DinoClassName = GeneralSettings.GetClassName().ToFString();
			AsocDinoArray.Find(DinoClassName)->Add(Dino);// Not safe, may result in crash, there are edge case can be with dinoses that arent listed in CookedClasses in future

			float DinoGrowthPercent = Dino->GetGrowth();
			if (DinoGrowthPercent >= 0.25) continue;
			AsocDinoChildsArray.Find(DinoClassName)->Add(Dino);
		}

		TSet<FString> ExcludedClasses;
		TMap<FString, int> ClassesClutchLimits;
		for (PopulationConfig::SlotConfig& Slot : LoadedConfig.DinoClasses) {
			FString DinoClassName = FString(RC::to_wstring(Slot.name));
			TArray<ATIDinosaurBase*>* TargetArray = AsocDinoArray.Find(DinoClassName);
			if (!TargetArray) continue;// Safe check, just in case somebody mess up with config, if you set up it right there no chance for it to fail

			int DinoNumber = TargetArray->Num();
			if (!DinoNumber) continue;
			if (TotalPlayersPlaying / Slot.value > DinoNumber) continue;

			ExcludedClasses.Add(DinoClassName);
			ClassesClutchLimits.Add(DinoClassName, Slot.clutch_value);
		}

		TArray<FTIAvailableClassData> AvailableClass = GameMode->GetAvailableClasses();
		AvailableClass.Reset(CookedClasses.Num() - ExcludedClasses.Num());// Just in case

		for (FTIAvailableClassData& ClassData : CookedClasses) {
			FString DinoClassName = ClassData.Name.ToFString();
			if (!ExcludedClasses.Contains(DinoClassName)) {
				AvailableClass.Add(ClassData);
				continue;
			}

			TArray<ATIDinosaurBase*> TargetArray = *AsocDinoChildsArray.Find(DinoClassName);// Here we for sure know this array exist, so no safe checks
			if (!TargetArray.Num()) continue;

			ATIDinosaurBase* Dino = TargetArray.Top();
			int ClutchLimits = *ClassesClutchLimits.Find(DinoClassName);
			uint8 ClutchSize{};
			if (ClutchLimits && TotalPlayersPlaying > ClutchLimits) ClutchSize = static_cast<uint8>(TotalPlayersPlaying / ClutchLimits);
			else ClutchSize = Dino->GetEggClutchSize();

			if (ClutchSize >= TargetArray.Num()) continue;// This way ppl can nest over limit

			float HighestGrow = 1;
			for (ATIDinosaurBase* CurrentDino : TargetArray) {// Find the smallest one, so we wont be doing circle around killing the biggest one after somebody joins in
				float DinoGrowthPercent = CurrentDino->GetGrowth();
				if (DinoGrowthPercent > HighestGrow) continue;
				Dino = CurrentDino;
				HighestGrow = DinoGrowthPercent;
			}

			Dino->SetHealth(0);
		}
	}

	auto Initialize(PopulationConfig::PopulationLimiterConfig Config) -> void {
		LoadedConfig = Config;
		_DinoClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/TheIsle.TIDinosaurBase"));
	}
}