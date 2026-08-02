
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
#include <TheIsle/ATIDinosaurBase.hpp>
#include <Subsystem.hpp>


PopulationControlSubsystem::PopulationControlSubsystem(PopulationControlConfig* SharedConfig) {
	Config = SharedConfig;

	// Мы хотим что бы эта хуйня тикала только когда игра тикает, и не на оборот...
	FireCallBackID = Hook::RegisterEngineTickPreCallback(
		[this](Hook::TCallbackIterationData<void>& info, UEngine* Context, float DeltaSeconds, bool bIdleMode) {
			Tick(DeltaSeconds, bIdleMode);
		}
		, {false, true, STR("PopulationControl"), STR("TickingControl")}
	);
}

PopulationControlSubsystem::~PopulationControlSubsystem() {
	Hook::UnregisterCallback(FireCallBackID);
}


void PopulationControlSubsystem::Tick(float DeltaSeconds, bool bIdleMode) {
	// Нахуя тикать если ничего не изменилось?
	if (bIdleMode) return;

	if (++TicksFired < TickRate) return;
	TicksFired = 0;

	static ATIGameModeBase* GameMode = static_cast<ATIGameModeBase*>(UObjectGlobals::FindFirstOf(STR("BP_SurvivalGameMode_C")));

	int TotalPlayersPlaying{0};// No, I know I can use Num, but this mod count lobby sitters too and ooman players
	TMap<FString, TArray<ATIDinosaurBase*>> AsocDinoArray;
	TMap<FString, TArray<ATIDinosaurBase*>> AsocDinoChildsArray;

	TArray<FTIAvailableClassData> CookedClasses = GameMode->CookedClasses();
	for (FTIAvailableClassData& ClassData : CookedClasses) {
		FString DinoClassName = ClassData.Name.ToFString();
		AsocDinoArray.Add(DinoClassName, TArray<ATIDinosaurBase*>{});
		AsocDinoChildsArray.Add(DinoClassName, TArray<ATIDinosaurBase*>{});
	}

	TSet<ATIPlayerController*> ActivePlayers = GameMode->AllPlayerControllers();
	for (ATIPlayerController* Player : ActivePlayers) {
		TotalPlayersPlaying++;
		APawn* Pawn = Player->Pawn();
		if (!Pawn || !Pawn->IsA(ATIDinosaurBase::StaticClass())) continue;// Make sure it's actually dino, not a fucking damn human

		ATIDinosaurBase* Dino = static_cast<ATIDinosaurBase*>(Pawn);
		FGeneralSettings GeneralSettings = Dino->GeneralSettings();
		FString DinoClassName = GeneralSettings.ClassName().ToFString();
		AsocDinoArray.Find(DinoClassName)->Add(Dino);// Not safe, may result in crash, there are edge case can be with dinoses that arent listed in CookedClasses in future

		float DinoGrowthPercent = Dino->GetGrowth();
		if (DinoGrowthPercent >= Config->spawn_growth) continue;
		AsocDinoChildsArray.Find(DinoClassName)->Add(Dino);
	}

	TSet<FString> ExcludedClasses;
	TMap<FString, int> ClassesClutchLimits;
	for (SlotConfig& Slot : Config->DinoClasses) {
		FString DinoClassName = FString(RC::to_wstring(Slot.name));
		TArray<ATIDinosaurBase*>* TargetArray = AsocDinoArray.Find(DinoClassName);
		if (!TargetArray) continue;// Safe check, just in case somebody mess up with config, if you set up it right there no chance for it to fail

		int DinoNumber = TargetArray->Num();
		if (!DinoNumber) continue;
		if (TotalPlayersPlaying / Slot.value > DinoNumber) continue;

		ExcludedClasses.Add(DinoClassName);
		ClassesClutchLimits.Add(DinoClassName, Slot.clutch_value);
	}

	TArray<FTIAvailableClassData> AvailableClass = GameMode->AvailableClasses();
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
		else ClutchSize = Dino->EggClutchSize();

		if (ClutchSize >= TargetArray.Num()) continue;// This way ppl can nest over limit

		float HighestGrow = 1;
		for (ATIDinosaurBase* CurrentDino : TargetArray) {// Find the smallest one, so we wont be doing circle around killing the biggest one after somebody joins in
			float DinoGrowthPercent = CurrentDino->GetGrowth();
			// Make sure we are not going to slay entombed dudes in any case
			if (DinoGrowthPercent > HighestGrow || CurrentDino->ElderReplicationStacks()) continue;
			Dino = CurrentDino;
			HighestGrow = DinoGrowthPercent;
		}

		Dino->SetHealth(0);
	}
}
