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

//////////////////////////////////////////////////////////////////////////// AVSCharacter

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

	CharacterHealthComponent = CreateDefaultSubobject<UVSCharacterHealthComponent>(TEXT("CharacterHealthComponent"));
	CharacterHealthComponent ? CharacterHealthComponent->OnDead.AddDynamic(this, &AVSCharacter::CharDead) : void(0);

	bReplicates = true;
}

void AVSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (CharacterHealthComponent)
	{
		FString OwnerName = CharacterHealthComponent->GetOwner() ? CharacterHealthComponent->GetOwner()->GetName() : TEXT("None");
		///UE_LOG(LogTemp, Warning, TEXT("Owner Name in BeginPlay: %s"), *OwnerName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Don`t have CharacterHealthComponent"));
	}

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
}

void AVSCharacter::MovementTick(float DeltaTime)
{
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
	bCanAiming = false;
	StopAiming();
	Super::Jump();
}

void AVSCharacter::StopJumping()
{
	bIsJumping = false;
	bCanAiming = true;
	Super::StopJumping();
}

void AVSCharacter::CharDead(AController* DamageInstigator)
{
	UE_LOG(LogTemp, Warning, TEXT("CharDead!"));
	CharDead_BP(DamageInstigator);

	if (HasAuthority())
	{
		float TimeAnim = 0.0f;

		int32 rnd = FMath::RandHelper(DeadsAnim.Num());
		if (DeadsAnim.IsValidIndex(rnd) && DeadsAnim[rnd] && GetMesh() && GetMesh()->GetAnimInstance())
		{
			TimeAnim = DeadsAnim[rnd]->GetPlayLength();
			PlayDeadMontage_Multicast(DeadsAnim[rnd], DeadsAnim[rnd]);
		}

		GetController() ? GetController()->UnPossess() : void(0);
		GetWorldTimerManager().SetTimer(RagDollTimerHandle, this, &AVSCharacter::EnableRagdoll_Multicast, (TimeAnim - 1.0f), false);
		SetLifeSpan(5.0f);
		GetCurrentWeapon() ? GetCurrentWeapon()->SetLifeSpan(5.0f) : void(0);
	}
	else
	{
		FireEvent(false);
	}

	GetCapsuleComponent() ? GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore) : void(0);
}

void AVSCharacter::CharDead_BP_Implementation(AController* DamageInstigator){}

void AVSCharacter::EquipWeapon_OnServer_Implementation(const int32 Index)
{
	if (!Weapons.IsValidIndex(Index) || CurrentWeapon == Weapons[Index]) return;


	if (IsLocallyControlled() || HasAuthority())
	{
		StartWeaponEquipAnimation(/*ThirdPersonEquipAnimation*/ CurrentWeapon->WeaponSetting.ThirdPersonEquipAnimation, FirstPersonEquipWeaponAnimation);
		BlockActionDuringEquip_OnClient();
	
		EquipTimerDelegate.BindUFunction(this, "ChangingWeapon",Index);
		GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, EquipTimerDelegate, 1.5f, false);
	}
	else if (!HasAuthority())
	{
		SetCurrentWeapon_OnServer(Weapons[Index]);
	}
}

void AVSCharacter::BlockActionDuringEquip_OnClient_Implementation()
{
	CurrentWeapon->BlockFire = true;
	bIsAiming ? StopAiming() : void(0);
	bCanAiming = false;
}

void AVSCharacter::ChangingWeapon_Implementation(int32 Index)
{
	GetWorld()->GetTimerManager().ClearTimer(EquipTimerHandle);

	CurrentIndex = Index;
	const ABaseWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = Weapons[Index];
	OnAmmoTypeChange.Broadcast(CurrentWeapon->GetWeaponType(), CurrentWeapon->GetWeaponRound());
	OnRep_CurrentWeapon(OldWeapon);

	CurrentWeapon->BlockFire = false;
	bCanAiming = true;
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
}

void AVSCharacter::EndFire()
{
	bIsFire = false;
	FireEvent(false);
}

void AVSCharacter::InitCrouch()
{
	bIsCrouch = true;
}

void AVSCharacter::StopCrouch()
{
	bIsCrouch = false;
	UE_LOG(LogTemp, Warning, TEXT("StopCrouch"));
}

void AVSCharacter::TryReloadWeapon()
{
	if (CharacterHealthComponent && CharacterHealthComponent->GetIsAlive() && CurrentWeapon && !CurrentWeapon->WeaponReloading)
	{
		TryReloadWeapon_OnServer();
	}
}

void AVSCharacter::TryReloadWeapon_OnServer_Implementation()
{
	if (CurrentWeapon->GetWeaponRound() < CurrentWeapon->WeaponSetting.MaxRound && CurrentWeapon->GetAmmoFromBackpack() != 0 && CurrentWeapon->CheckCanWeaponReload())
	{
		bIsAiming ? StopAiming() : void(0);
		bCanAiming = false;
		bIsReload = true;
		CurrentWeapon->InitReload();
	}
}

void AVSCharacter::InitReload()
{
	TryReloadWeapon();
}

void AVSCharacter::WeaponReloadEnd()
{
	CurrentWeapon ? OnAmmoChange.Broadcast(CurrentWeapon->WeaponInfo.Round) : void(0);

	bIsReload = false;
	bCanAiming = true;
}

void AVSCharacter::StopAiming_OnClient_Implementation()
{
	StopAiming();
}

void AVSCharacter::StartWeaponReloadAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P) 
{
	StopAiming_OnClient();
	
	if (Anim3P && Anim1P)
	{
		PlayWeaponReloadMontage_Multicast(Anim3P, Anim1P);
	}
}

void AVSCharacter::StartWeaponFireAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P)
{
	CurrentWeapon ? ChangeAmmoByShotEvent_Multicast() : void(0);
	
	if (Anim3P && Anim1P)
	{	
		PlayWeaponFireMontage_Multicast(Anim3P, Anim1P);
	}
} 

void AVSCharacter::StartWeaponEquipAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P)
{
	if (Anim3P && Anim1P)
	{
		PlayWeaponEquipMontage_Multicast(Anim3P, Anim1P);
	}
}

void AVSCharacter::EnableRagdoll_Multicast_Implementation()
{
	if (GetMesh())
	{
		GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);
		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetSimulatePhysics(true);
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
}

void AVSCharacter::PlayWeaponFireMontage_Multicast_Implementation(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim)
{
	UAnimInstance* AnimInstance3P = GetMesh()->GetAnimInstance();
	UAnimInstance* AnimInstance1P = Mesh1P->GetAnimInstance();
	if (AnimInstance3P != nullptr && AnimInstance1P != nullptr)
	{
		AnimInstance3P->Montage_Play(ThirdPersonAnim);
		AnimInstance1P->Montage_Play(FirstPersonAnim);
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

void AVSCharacter::PlayDeadMontage_Multicast_Implementation(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim)
{
	UAnimInstance* AnimInstance3P = GetMesh()->GetAnimInstance();
	UAnimInstance* AnimInstance1P = Mesh1P->GetAnimInstance();
	if (AnimInstance3P != nullptr && AnimInstance1P != nullptr)
	{
		AnimInstance3P->Montage_Play(ThirdPersonAnim);
		AnimInstance1P->Montage_Play(FirstPersonAnim);
	}
}

void AVSCharacter::ChangeAmmoByShotEvent_Multicast_Implementation() 
{
	HasAuthority() ? OnAmmoChange.Broadcast(CurrentWeapon->WeaponInfo.Round) : OnAmmoChange.Broadcast(CurrentWeapon->WeaponInfo.Round - 1);
}

void AVSCharacter::InitAiming()
{
	if (bCanAiming && CurrentWeapon && !CurrentWeapon->WeaponReloading) /// && !CurrentWeapon->WeaponReloading WORK FOR SERVER ONLY
	{
		CurrentWeapon->WeaponSetting.InAimingSound ? UGameplayStatics::PlaySound2D(GetWorld(), CurrentWeapon->WeaponSetting.InAimingSound) : void(0);
		CurrentWeapon->bIsRailGun ? InitAimTimeline(90.0f, 30.0f) : InitAimTimeline(90.0f, 60.0f); 

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
}

void AVSCharacter::InitAiming_OnServer_Implementation()
{
	InitAiming();
}

void AVSCharacter::StopAiming()
{
	if (bIsAiming && CurrentWeapon)
	{
		CurrentWeapon->WeaponSetting.InAimingSound ? UGameplayStatics::PlaySound2D(GetWorld(), CurrentWeapon->WeaponSetting.OutAimingSound) : void(0);
		CurrentWeapon->bIsRailGun ? InitAimTimeline(30.0f, 90.0f) : InitAimTimeline(60.0f, 90.0f);
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
}

void AVSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AVSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
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
		OnSwitchWeapon.Broadcast(CurrentWeapon->GetWeaponType(), CurrentWeapon->WeaponInfo,CurrentWeapon); 
		
		if (!CurrentWeapon->CurrentOwner)
		{
			CurrentWeapon->SetActorTransform(Mesh1P->GetSocketTransform(FName("WeaponSocket")), false, nullptr, ETeleportType::TeleportPhysics);
			CurrentWeapon->AttachToComponent(Mesh1P, FAttachmentTransformRules::KeepWorldTransform, FName("WeaponSocket"));
			CurrentWeapon->InitOwnerCharacter();
			CurrentWeapon->SkeletalMeshWeapon->SetOwnerNoSee(false);
		}
		CurrentWeapon->SkeletalMeshWeapon->SetVisibility(true, true);

		FP_Gun->SetSkeletalMesh(CurrentWeapon->SkeletalMeshWeapon->SkeletalMesh, false);

		if (CurrentWeapon->bIsRailGun)
		{
			FP_Gun->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("SecondaryWeaponSocket"));
		}
		else
		{
			FP_Gun->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("WeaponSocket"));
		}

		CurrentWeapon->OnWeaponReloadStart.RemoveDynamic(this, &AVSCharacter::StartWeaponReloadAnimation);
		CurrentWeapon->OnWeaponReloadEnd.RemoveDynamic(this, &AVSCharacter::WeaponReloadEnd);
		CurrentWeapon->OnWeaponFireStart.RemoveDynamic(this, &AVSCharacter::StartWeaponFireAnimation);

		CurrentWeapon->OnWeaponReloadStart.AddDynamic(this, &AVSCharacter::StartWeaponReloadAnimation);
		CurrentWeapon->OnWeaponReloadEnd.AddDynamic(this, &AVSCharacter::WeaponReloadEnd);
		CurrentWeapon->OnWeaponFireStart.AddDynamic(this, &AVSCharacter::StartWeaponFireAnimation);
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

			AController* myPC = GetController();
			APawn* Pawn = myPC ? myPC->GetPawn() : nullptr;
			Pawn ? Params.Instigator = Pawn : void(0);

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

EMovementState AVSCharacter::GetMovementState() const
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

float AVSCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (CharacterHealthComponent && CharacterHealthComponent->GetIsAlive())
	{
		CharacterHealthComponent->ChangeHealthValue_OnServer(-DamageAmount, EventInstigator);

		if (HasAuthority())
		{
			int32 rnd = FMath::RandHelper(ImpactSound.Num());
			if (ImpactSound.IsValidIndex(rnd) && ImpactSound[rnd] && GetWorld())
			{
				UGameplayStatics::PlaySound2D(GetWorld(), ImpactSound[rnd]);
			}
		}
	}
	return ActualDamage;
}

ABaseWeapon* AVSCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

bool AVSCharacter::GetIsAlive()
{
	bool result = false;
	if (CharacterHealthComponent)
	{
		result = CharacterHealthComponent->GetIsAlive();
	}
	return result;
}

EHeroType AVSCharacter::GetHeroType() const
{
	return HeroType;
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


