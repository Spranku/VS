// Copyright Epic Games, Inc. All Rights Reserved.

#include "VSCharacter.h"
#include "VSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AVSCharacter

AVSCharacter::AVSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);
}

void AVSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();


	if (HasAuthority())
	{
		for (const TSubclassOf<ABaseWeapon>& WeaponClass : DefaultWeapons)
		{
			if (!WeaponClass) continue;
			FActorSpawnParameters Params;
			Params.Owner = this;
			ABaseWeapon* Weapon3P = GetWorld()->SpawnActor<ABaseWeapon>(WeaponClass, Params);
			const int32 Index = Weapons.Add(Weapon3P);
			if (Index == CurrentIndex)
			{
				CurrentWeapon = Weapon3P;
				OnRep_CurrentWeapon(nullptr);
			}
		}
	}
	FP_Gun->bOnlyOwnerSee = true;

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		Mesh1P->SetHiddenInGame(false, true);
	}
}

void AVSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AVSCharacter::InitCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AVSCharacter::StopCrouch);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AVSCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AVSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AVSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AVSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AVSCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AVSCharacter::InitReload);
	PlayerInputComponent->BindAction("Reload", IE_Released, this, &AVSCharacter::StopReload);

	PlayerInputComponent->BindAction("Aiming", IE_Pressed, this, &AVSCharacter::InitAiming);
	PlayerInputComponent->BindAction("Aiming", IE_Released, this, &AVSCharacter::StopAiming);

	PlayerInputComponent->BindAction(FName("NextWeapon"), EInputEvent::IE_Pressed, this, &AVSCharacter::NextWeapon);
	PlayerInputComponent->BindAction(FName("LastWeapon"), EInputEvent::IE_Pressed, this, &AVSCharacter::LastWeapon);
}

void AVSCharacter::EquipWeapon_OnServer_Implementation(const int32 Index)
{
	if (!Weapons.IsValidIndex(Index) || CurrentWeapon == Weapons[Index]) return;

	if (IsLocallyControlled() || HasAuthority())
	{
		CurrentIndex = Index;
		const ABaseWeapon* OldWeapon = CurrentWeapon;
		CurrentWeapon = Weapons[Index];
		OnRep_CurrentWeapon(OldWeapon);
	}
	else if (!HasAuthority())
	{
		SetCurrentWeapon_OnServer(Weapons[Index]);
	}
}

void AVSCharacter::SetCurrentWeapon_OnServer_Implementation(ABaseWeapon* NewWeapon)
{
	const ABaseWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = NewWeapon;
	OnRep_CurrentWeapon(OldWeapon);
}

void AVSCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			if (!bUsingMotionControllers)
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<AVSProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AVSCharacter::InitCrouch()
{
	bIsCrouch = true;
	UE_LOG(LogTemp, Warning, TEXT("InitCrouch"));
}

void AVSCharacter::StopCrouch()
{
	bIsCrouch = false;
	UE_LOG(LogTemp, Warning, TEXT("StopCrouch"));
}

void AVSCharacter::InitReload()
{
	bIsReload = true;
	UE_LOG(LogTemp, Warning, TEXT("InitReload"));
}

void AVSCharacter::StopReload()
{
	bIsReload = false;
	UE_LOG(LogTemp, Warning, TEXT("StopReload"));
}

void AVSCharacter::InitAiming()
{
	if (HasAuthority())
	{
		bIsAiming = true;
		ChangeMovementState();
	}
	else
	{
		InitAiming_OnServer();
	}
}

void AVSCharacter::InitAiming_OnServer_Implementation()
{
	InitAiming();
}

void AVSCharacter::StopAiming()
{
	if (HasAuthority())
	{
		bIsAiming = false;
		ChangeMovementState();
	}
	else
	{
		StopAiming_OnServer();
	}
}

void AVSCharacter::StopAiming_OnServer_Implementation()
{
	StopAiming();
}

void AVSCharacter::NextWeapon()
{
	const int32 Index = Weapons.IsValidIndex(CurrentIndex + 1) ? CurrentIndex + 1 : 0;

	if (HasAuthority())
	{
		
		EquipWeapon_OnServer(Index);
	}
	else
	{
		EquipWeapon_OnServer(Index);
	}
}

void AVSCharacter::LastWeapon()
{
	const int32 Index = Weapons.IsValidIndex(CurrentIndex - 1) ? CurrentIndex - 1 : Weapons.Num() - 1;

	if (HasAuthority())
	{
		EquipWeapon_OnServer(Index);
	}
	else
	{
		EquipWeapon_OnServer(Index);
	}
}

void AVSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AVSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AVSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AVSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AVSCharacter::ChangeMovementState()
{
	EMovementState NewState = EMovementState::Run_State;
	if (!bIsCrouch && !bIsAiming)
	{
		NewState = EMovementState::Run_State;
	}
	else
	{
		if (bIsCrouch)
		{
			bIsAiming = false;
			NewState = EMovementState::Crouch_State;
		}
		else
		{
			if (bIsAiming)
			{
				bIsCrouch = false;
				NewState = EMovementState::AimWalk_State;

			}
		}
	}

	SetMovementState_OnServer(NewState);

	CharacterUpdate();

	////Weapon state update
	//AWeaponDefault* myWeapon = GetCurrentWeapon();
	//if (myWeapon)
	//{
	//	myWeapon->UpdateStateWeapon_OnServer(NewState);
	//}
}

void AVSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;

	switch (MovementState)
	{
	case EMovementState::Aim_State:
		ResSpeed = MovementInfo.AimSpeed;
		break;
	case EMovementState::AimWalk_State:
		ResSpeed = MovementInfo.AimSpeed;
		break;
	case EMovementState::Crouch_State:
		ResSpeed = MovementInfo.CrouchSpeed;
		break;
	case EMovementState::Run_State:
		ResSpeed = MovementInfo.RunSpeed;
		break;
	default:
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void AVSCharacter::SetMovementState_OnServer_Implementation(EMovementState NewState)
{
	SetMovementState_Multicast(NewState);
}

void AVSCharacter::SetMovementState_Multicast_Implementation(EMovementState NewState)
{
	MovementState = NewState;
	CharacterUpdate();
}

void AVSCharacter::OnRep_CurrentWeapon(const ABaseWeapon* OldWeapon)
{
	if (CurrentWeapon)
	{
		if (!CurrentWeapon->CurrentOwner)
		{
			CurrentWeapon->SetActorTransform(GetMesh()->GetSocketTransform(FName("WeaponSocket")), false, nullptr, ETeleportType::TeleportPhysics);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, FName("WeaponSocket"));
			CurrentWeapon->CurrentOwner = this;

			CurrentWeapon->SkeletalMeshWeapon->SetOwnerNoSee(true);
			
		}
		CurrentWeapon->SkeletalMeshWeapon->SetVisibility(true);

		FP_Gun->SetSkeletalMesh(CurrentWeapon->SkeletalMeshWeapon->SkeletalMesh, false); 
		FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("WeaponSocket"));
		FP_Gun->SetWorldScale3D(FVector(2));
	}

	if (OldWeapon)
	{
		OldWeapon->SkeletalMeshWeapon->SetVisibility(false);
	}
}

void AVSCharacter::InitWeapon()
{
	/*if (!DefaultWeapons.IsValidIndex())
	{
		FVector SpawnLocation = FVector(0);
		FRotator SpawnRotation = FRotator(0);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = GetInstigator();

		ABaseWeapon* myWeapon = Cast<ABaseWeapon>(GetWorld()->SpawnActor(WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
		if (myWeapon)
		{
			FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
			myWeapon->AttachToComponent(GetMesh(), Rule, FName("WeaponSocket"));
			CurrentWeapon = myWeapon;
		}
	}*/
}

ABaseWeapon* AVSCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void AVSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVSCharacter, MovementState);
	DOREPLIFETIME(AVSCharacter, bIsAiming);
	DOREPLIFETIME(AVSCharacter, bIsMoving);

	DOREPLIFETIME_CONDITION(AVSCharacter, Weapons, COND_None);
	DOREPLIFETIME_CONDITION(AVSCharacter, CurrentWeapon, COND_None);
	DOREPLIFETIME_CONDITION(AVSCharacter, CurrentIndex, COND_None);

	/*DOREPLIFETIME(ATPSCharacter, Effects);
	DOREPLIFETIME(ATPSCharacter, EffectAdd);
	DOREPLIFETIME(ATPSCharacter, EffectRemove);*/
}

