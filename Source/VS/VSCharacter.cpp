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
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
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
	FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	bReplicates = true;
}

void AVSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(InitWeaponTimerHandle, this, &AVSCharacter::InitWeapon, 0.5f, false);

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

void AVSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	MovementTick(DeltaTime);

	/*if (CurrentWeapon && Controller)
	{
		LastWeaponRotationUpdateTime += DeltaTime;
		if (LastWeaponRotationUpdateTime >= WeaponRotationUpdateInterval)
		{
			FRotator CameraRotation = Controller->GetControlRotation();
			CurrentWeapon->SetWeaponRotation(CameraRotation);
			LastWeaponRotationUpdateTime = 0.0f;
		}
		 ///FRotator CameraRotation = Controller->GetControlRotation(); 
		 /// CurrentWeapon->SetWeaponRotation(CameraRotation);
	}*/
}

void AVSCharacter::MovementTick(float DeltaTime)
{
	//if (CharHealthComponent && CharHealthComponent->GetIsAlive())
	//{

	///	FString SEnum = UEnum::GetValueAsString(GetMovementState());
	///	UE_LOG(LogTemp, Error, TEXT("MovementState - %s"), *SEnum);

	if (GetController() && GetController()->IsLocalPlayerController())
	{
		if (CurrentWeapon)
		{
			FVector Displacement = FVector(0);
			bool bIsReduceDispersion = false;
			switch (MovementState)
			{
			case EMovementState::Run_State:
				Displacement = (FirstPersonCameraComponent->GetForwardVector() * 10000.0f);
				bIsReduceDispersion = true;
				break;
			case EMovementState::AimWalk_State:
				Displacement = (FirstPersonCameraComponent->GetForwardVector() * 10000.0f);
				bIsReduceDispersion = true;
				break;
			default:
				break;
			}
			CurrentWeapon->UpdateWeaponByCharacterMovementStateOnServer((FirstPersonCameraComponent->GetForwardVector() * 10000.0f) + Displacement, bIsReduceDispersion);
		}
	}
	/// Do nothing?

	/*else
	{
		if (CurrentWeapon)
		{
			FVector Displacement = FVector(0);
			bool bIsReduceDispersion = false;
			switch (MovementState)
			{
			case EMovementState::Run_State:
				Displacement = (FirstPersonCameraComponent->GetForwardVector() * 10000.0f);
				bIsReduceDispersion = true;
				break;
			case EMovementState::AimWalk_State:
				bIsReduceDispersion = true;
				Displacement = (FirstPersonCameraComponent->GetForwardVector() * 10000.0f);
				break;
			default:
				break;
			}
			///CurrentWeapon->UpdateWeaponByCharacterMovementStateOnServer((GetForwardVectorFromCamera() * 10000.0f) + Displacement, bIsReduceDispersion);
		}
	}*/
}

void AVSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AVSCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AVSCharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AVSCharacter::InitCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AVSCharacter::StopCrouch);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AVSCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AVSCharacter::EndFire); /// For test

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

	PlayerInputComponent->BindAction("Aiming", IE_Pressed, this, &AVSCharacter::InitAiming);
	PlayerInputComponent->BindAction("Aiming", IE_Released, this, &AVSCharacter::StopAiming);

	PlayerInputComponent->BindAction(FName("NextWeapon"), EInputEvent::IE_Pressed, this, &AVSCharacter::NextWeapon);
	PlayerInputComponent->BindAction(FName("LastWeapon"), EInputEvent::IE_Pressed, this, &AVSCharacter::LastWeapon);
}

void AVSCharacter::Jump()
{
	bIsJumping = true;
	Super::Jump();
}

void AVSCharacter::StopJumping()
{
	bIsJumping = false;
	Super::StopJumping();
}

void AVSCharacter::EquipWeapon_OnServer_Implementation(const int32 Index)
{
	if (!Weapons.IsValidIndex(Index) || CurrentWeapon == Weapons[Index]) return;

	if (IsLocallyControlled() || HasAuthority())
	{
		WeaponEquipAnimStart(ThirdPersonEquipAnimation, FirstPersonEquipWeaponAnimation);
		///CurrentIndex = Index;
		///const ABaseWeapon* OldWeapon = CurrentWeapon;
		///CurrentWeapon = Weapons[Index];
		///OnRep_CurrentWeapon(OldWeapon);
		
		CurrentWeapon->BlockFire = true;
		EquipTimerDelegate.BindUFunction(this, "ChangingWeapon",Index);
		GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, EquipTimerDelegate, 1.5f, false);
	}
	else if (!HasAuthority())
	{
		SetCurrentWeapon_OnServer(Weapons[Index]);
	}
}

void AVSCharacter::ChangingWeapon(int32 Index)
{
	GetWorld()->GetTimerManager().ClearTimer(EquipTimerHandle);
	CurrentIndex = Index;
	const ABaseWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = Weapons[Index];
	OnRep_CurrentWeapon(OldWeapon);
	CurrentWeapon->BlockFire = false;
}

void AVSCharacter::SetCurrentWeapon_OnServer_Implementation(ABaseWeapon* NewWeapon)
{
	const ABaseWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = NewWeapon;
	OnRep_CurrentWeapon(OldWeapon);
}

void AVSCharacter::OnFire()
{
	bIsFire = true;
	FireEvent(true);

	// try and fire a projectile
	//if (ProjectileClass != nullptr)
	//{
	//	UWorld* const World = GetWorld();
	//	if (World != nullptr)
	//	{
	//		if (!bUsingMotionControllers)
	//		{
	// 
	// 
	//			const FRotator SpawnRotation = GetControlRotation();
	//			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	//			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

	//			//Set Spawn Collision Handling Override
	//			FActorSpawnParameters ActorSpawnParams;
	//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	//			// spawn the projectile at the muzzle
	//			World->SpawnActor<AVSProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
	//		}
	//	}
	//}

	//// try and play the sound if specified
	//if (FireSound != nullptr)
	//{
	//	UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	//}

	//// try and play a firing animation if specified
	//if (FireAnimation != nullptr)
	//{
	//	// Get the animation object for the arms mesh
	//	UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
	//	if (AnimInstance != nullptr)
	//	{
	//		AnimInstance->Montage_Play(FireAnimation, 1.f);
	//	}
	//}
}

void AVSCharacter::EndFire() /// For test
{
	bIsFire = false;
	FireEvent(false);
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

void AVSCharacter::TryReloadWeapon()
{
	if (/*CharHealthComponent && CharHealthComponent->GetIsAlive() && */CurrentWeapon && !CurrentWeapon->WeaponReloading)
	{
		TryReloadWeapon_OnServer();
	}
}

void AVSCharacter::TryReloadWeapon_OnServer_Implementation()
{
	if (CurrentWeapon->GetWeaponRound() < CurrentWeapon->WeaponSetting.MaxRound && CurrentWeapon->CheckCanWeaponReload())
	{
		bIsReload = true;
		CurrentWeapon->InitReload();
	}
}

void AVSCharacter::InitReload()
{
	/// bIsReload = true;
	TryReloadWeapon();
}

void AVSCharacter::WeaponReloadAnimStart(UAnimMontage* Anim3P, UAnimMontage* Anim1P)
{
	if (Anim3P && Anim1P)
	{
		/// WeaponReloadStart_BP(Anim3P, Anim1P);
		PlayWeaponReloadMontage_Multicast(Anim3P, Anim1P);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AVSCharacter::WeaponReloadStart - Anim3P && Anim1P = 0"));
	}
}

void AVSCharacter::PlayWeaponReloadMontage_Multicast_Implementation(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim)
{
	UAnimInstance* AnimInstance3P = GetMesh()->GetAnimInstance();
	UAnimInstance* AnimInstance1P = Mesh1P->GetAnimInstance();
	if (AnimInstance3P != nullptr && AnimInstance1P != nullptr)
	{
		AnimInstance3P->Montage_Play(ThirdPersonAnim);
		AnimInstance1P->Montage_Play(FirstPersonAnim);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AVSCharacter::PlayReloadMontage_Multicast_Implementation - AnimInstance3P = nullptr && AnimInstance1P = nullptr"));
	}
}

void AVSCharacter::WeaponReloadEnd(bool bIsSuccess, int32 AmmoSafe)
{
	bIsReload = false;
	/// WeaponReloadEnd_BP(bIsSuccess);
}

void AVSCharacter::WeaponFireAnimStart(UAnimMontage* Anim3P, UAnimMontage* Anim1P)
{
	///if (InventoryComponent && CurrentWeapon)
	///{
	///	InventoryComponent->SetAdditionalInfoWeapon(CurrentIndexWeapon, CurrentWeapon->AdditionalWeaponInfo);
	///}
	/// WeaponFireStart_BP(Anim3P,Anim1P);
	
	if (Anim3P && Anim1P)
	{	
		PlayWeaponFireMontage_Multicast(Anim3P, Anim1P);
	}
}

void AVSCharacter::WeaponEquipAnimStart(UAnimMontage* Anim3P, UAnimMontage* Anim1P)
{
	if (Anim3P && Anim1P)
	{
		PlayWeaponEquipMontage_Multicast(Anim3P, Anim1P);
	}
}

void AVSCharacter::PlayWeaponEquipMontage_Multicast_Implementation(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim)
{
	UAnimInstance* AnimInstance3P = GetMesh()->GetAnimInstance();
	UAnimInstance* AnimInstance1P = Mesh1P->GetAnimInstance();
	if (AnimInstance3P != nullptr && AnimInstance1P != nullptr)
	{
		AnimInstance3P->Montage_Play(ThirdPersonAnim);
		AnimInstance1P->Montage_Play(FirstPersonAnim);
	}
}

void AVSCharacter::PlayWeaponFireMontage_Multicast_Implementation(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim)
{
	UAnimInstance* AnimInstance3P = GetMesh()->GetAnimInstance();
	UAnimInstance* AnimInstance1P = Mesh1P->GetAnimInstance();
	if (AnimInstance3P != nullptr && AnimInstance1P != nullptr)
	{
		AnimInstance3P->Montage_Play(ThirdPersonAnim);
		AnimInstance1P->Montage_Play(FirstPersonAnim);
		///FireRecoil(); /// Here ?
	}
}

void AVSCharacter::InitAiming()
{

	if (CurrentWeapon && CurrentWeapon->bIsRailGun)
	{
		InitAimTimeline(90.0f, 30.0f);
	}
	else
	{
		InitAimTimeline(90.0f, 60.0f);
	}
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
	if (CurrentWeapon && CurrentWeapon->bIsRailGun)
	{
		InitAimTimeline(30.0f, 90.0f);
	}
	else
	{
		InitAimTimeline(60.0f, 90.0f);
	}

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

void AVSCharacter::ChangeFoV(float In, float Out)
{
	if (CurrentWeapon && CurrentWeapon->WeaponSetting.ADS && Alpha >= 1.0f)
	{
		Alpha = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(AimTimerHandle);
	}
	else
	{
		Alpha += GetWorld()->DeltaTimeSeconds * CurrentWeapon->WeaponSetting.ADS;
		FirstPersonCameraComponent->SetFieldOfView(UKismetMathLibrary::Lerp(In, Out, Alpha));
	}
}

void AVSCharacter::InitAimTimeline(float From, float To)
{
	AimTimerDelegate.BindUFunction(this, "ChangeFoV", From, To);
	GetWorld()->GetTimerManager().SetTimer(AimTimerHandle, AimTimerDelegate, GetWorld()->DeltaTimeSeconds, true);
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


void AVSCharacter::FireEvent(bool bIsFiring)
{
	ABaseWeapon* myWeapon = nullptr;
	myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->SetWeaponStateFire_OnServer(bIsFiring);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AVSCharacter::FireEvent - Current weapon = NULL"));
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

	if (BaseLookUpRate != 0)
	{
		float Pitch = UKismetMathLibrary::Clamp(UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), GetActorRotation()).Pitch, -90.0f, 90.0f);

		if (HasAuthority())
		{
			PitchMulticast(Pitch);
			Pitch_OnRep = Pitch;
		}
		else
		{
			PitchOnServer(Pitch);
			Pitch_OnRep = Pitch;
		}

		if (IsLocallyControlled())
		{
			FRotator Rotation = GetControlRotation();
			if (HasAuthority())
			{
				M_LookUPSync(Rotation);
			}
			else
			{
				S_LookUPSync(Rotation);
			}
		}
	}
}

void AVSCharacter::PitchMulticast_Implementation(float PitchRep)
{
	if (!IsLocallyControlled())
	{
		Pitch_OnRep = PitchRep;
	}
}

void AVSCharacter::PitchOnServer_Implementation(float PitchRep)
{
	PitchMulticast(PitchRep);
}

void AVSCharacter::S_LookUPSync_Implementation(FRotator RotationSync)
{
	ControlRotationSynchronized = RotationSync;
	if (!IsLocallyControlled())
	{
		FirstPersonCameraComponent->SetWorldRotation(ControlRotationSynchronized);
	}
}

void AVSCharacter::M_LookUPSync_Implementation(FRotator RotationSync)
{
	ControlRotationSynchronized = RotationSync;
	if (!IsLocallyControlled())
	{
		FirstPersonCameraComponent->SetWorldRotation(ControlRotationSynchronized);
	}
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

	ABaseWeapon* myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->UpdateStateWeapon_OnServer(NewState);
	}
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
			CurrentWeapon->SetActorTransform(/*GetMesh()*/Mesh1P->GetSocketTransform(FName("WeaponSocket")), false, nullptr, ETeleportType::TeleportPhysics);
			CurrentWeapon->AttachToComponent(/*GetMesh()*/ Mesh1P, FAttachmentTransformRules::KeepWorldTransform, FName("WeaponSocket"));
			CurrentWeapon->CurrentOwner = this;
			CurrentWeapon->SkeletalMeshWeapon->SetOwnerNoSee(false);
		}
		CurrentWeapon->SkeletalMeshWeapon->SetVisibility(true, true);
		CurrentWeapon->WeaponInfo.Round = CurrentWeapon->WeaponSetting.MaxRound; /// Here?

		FP_Gun->SetSkeletalMesh(CurrentWeapon->SkeletalMeshWeapon->SkeletalMesh, false);

		if (CurrentWeapon->bIsRailGun)
		{
			FP_Gun->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("SecondaryWeaponSocket"));

		}
		else
		{
			FP_Gun->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("WeaponSocket"));
		}

		CurrentWeapon->OnWeaponReloadStart.RemoveDynamic(this, &AVSCharacter::WeaponReloadAnimStart);
		CurrentWeapon->OnWeaponReloadEnd.RemoveDynamic(this, &AVSCharacter::WeaponReloadEnd);
		CurrentWeapon->OnWeaponFireStart.RemoveDynamic(this, &AVSCharacter::WeaponFireAnimStart);

		CurrentWeapon->OnWeaponReloadStart.AddDynamic(this, &AVSCharacter::WeaponReloadAnimStart);
		CurrentWeapon->OnWeaponReloadEnd.AddDynamic(this, &AVSCharacter::WeaponReloadEnd);
		CurrentWeapon->OnWeaponFireStart.AddDynamic(this, &AVSCharacter::WeaponFireAnimStart);
	}

	if (OldWeapon)
	{
		OldWeapon->SkeletalMeshWeapon->SetVisibility(false,true);
	}

	GetWorld()->GetTimerManager().ClearTimer(EquipTimerHandle);
}

void AVSCharacter::InitWeapon()
{
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
	InitWeaponTimerHandle.Invalidate();
}

//void AVSCharacter::FireRecoil()
//{
//	float BaseRecoil = 0.25f;
//	float RecoilCoef = 2.0f;
//	float Multiplier = -1.0f;
//
//	float PitchRecoil = BaseRecoil * Multiplier;
//	float YawRecoil = (PitchRecoil / RecoilCoef * FMath::RandRange(PitchRecoil / RecoilCoef * Multiplier, PitchRecoil / RecoilCoef));
//
//	AddControllerPitchInput(PitchRecoil);
//	AddControllerYawInput(YawRecoil);
//}

EMovementState AVSCharacter::GetMovementState()
{
	return MovementState;
}

FVector AVSCharacter::GetForwardVectorFromCamera()
{
	CamForwardVector = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator; 
	return CamForwardVector.Vector();
}

FVector AVSCharacter::GetLocationFromCamera()
{
	return FirstPersonCameraComponent->GetComponentLocation();
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
	DOREPLIFETIME(AVSCharacter, bIsReload);
	DOREPLIFETIME(AVSCharacter, bIsFire);
	DOREPLIFETIME(AVSCharacter, Direction);
	DOREPLIFETIME(AVSCharacter, AimPitch);
	DOREPLIFETIME(AVSCharacter, Pitch_OnRep);
	DOREPLIFETIME(AVSCharacter, AimYaw);
	DOREPLIFETIME(AVSCharacter, CurrentWeapon);


	DOREPLIFETIME_CONDITION(AVSCharacter, Weapons, COND_None);
	DOREPLIFETIME_CONDITION(AVSCharacter, CurrentWeapon, COND_None);
	DOREPLIFETIME_CONDITION(AVSCharacter, CurrentIndex, COND_None);
}

