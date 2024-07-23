// Copyright Epic Games, Inc. All Rights Reserved.

#include "VSGameMode.h"
#include "VSHUD.h"
#include "VSCharacter.h"
#include "UObject/ConstructorHelpers.h"

AVSGameMode::AVSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/Core/BP_Character"/*"/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"*/));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AVSHUD::StaticClass();
	//Blueprint'/Game/Blueprints/Core/BP_Character.BP_Character'
}
