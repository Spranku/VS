// Copyright Epic Games, Inc. All Rights Reserved.

#include "VSGameMode.h"
#include "VSHUD.h"
#include "VSCharacter.h"
#include "UObject/ConstructorHelpers.h"

AVSGameMode::AVSGameMode() : Super()
{
	UE_LOG(LogTemp, Warning, TEXT("Init"));
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/Core/BP_Character"/*"/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"*/));
	if (PlayerPawnClassFinder.Succeeded())
	{
		DefaultPawnClass = PlayerPawnClassFinder.Class;
		UE_LOG(LogTemp, Warning, TEXT("Success find BP_Character"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Don`t find BP_Character"));
	}

	// use our custom HUD class
	HUDClass = AVSHUD::StaticClass();
	//Blueprint'/Game/Blueprints/Core/BP_Character.BP_Character'
}
