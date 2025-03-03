// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "/Projects/VS/Source/VS/VSCharacter.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	SkeletalMeshWeapon = CreateDefaultSubobject <USkeletalMeshComponent > (TEXT("SkeletalMeshWeapon"));
	SkeletalMeshWeapon->SetGenerateOverlapEvents(false);
	SkeletalMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshWeapon->SetupAttachment(RootComponent);

	StaticMeshWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scope"));
	StaticMeshWeapon->SetGenerateOverlapEvents(false);
	StaticMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	StaticMeshWeapon->SetupAttachment(SkeletalMeshWeapon);

	LenseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LenseMesh"));
	LenseMesh->SetupAttachment(SkeletalMeshWeapon);
	LenseMesh->SetCollisionProfileName(FName("NoCollision"), false);
	LenseMesh->SetRelativeLocation(FVector(0.02f, -5.5f, 21.5f));
	LenseMesh->SetRelativeRotation(FRotator(0.0f, -180.0f, 90.0f));
	LenseMesh->bOnlyOwnerSee = true;

	ShootLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShootLocation"));
	ShootLocation->SetupAttachment(RootComponent);

	SleeveLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("SleeveLocation"));
	SleeveLocation->SetupAttachment(RootComponent);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(SkeletalMeshWeapon);
	SceneCapture->SetRelativeScale3D(FVector(0.1, 0.1, 0.1));
	SceneCapture->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
	SceneCapture->SetRelativeLocation(FVector(0.0, 95.0, 16.0));
	SceneCapture->FOVAngle = 4.0f;
	SceneCapture->Deactivate();
	
	bReplicates = true;
}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	DefaultLenseMaterial ? LenseMesh->SetMaterial(0, DefaultLenseMaterial) : 0;

	WeaponInit();

	if (!CurrentOwner)
	{
		SkeletalMeshWeapon->SetVisibility(true);
	}
}

void ABaseWeapon::ChangeAnimationsForOwner()
{
	switch (CurrentOwner->HeroType)
	{
	case EHeroType::Hunk:
		SetAnimationForHunkHero_BP();
		break;
	case EHeroType::Swat:
		SetAnimationForSwatHero_BP();
		break;
	case EHeroType::Observer:
		break;
	default:
		break;
	}
}

void ABaseWeapon::InitOwnerCharacter()
{
	AVSCharacter* MyChar = Cast<AVSCharacter>(GetOwner());
	if (MyChar)
	{
		CurrentOwner = MyChar;
		CurrentOwner->SetInstigator(CurrentOwner);
		ChangeAnimationsForOwner();
	}
} 

// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireTick(DeltaTime);
	ReloadTick(DeltaTime);
	DispersionTick(DeltaTime);
}

void ABaseWeapon::ReloadTick(float DeltaTime)
{
	if (WeaponReloading || GetWeaponRound() < 0)
	{
		if (ReloadTimer < 0.0f)
		{
			FinishReload();
		}
		else
		{
			ReloadTimer -= DeltaTime;
		}
	}
}

void ABaseWeapon::FireTick(float DeltaTime)
{
	if (WeaponFiring && GetWeaponRound() > 0 && !WeaponReloading)
	{
		if (FireTime < 0.f)
		{
			FVector MuzzleLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight");
			FVector ShootDirection = CurrentOwner->GetForwardVectorFromCamera() * 10000.0f; /// 
			FTransform ShootTo;
			FHitResult HitResult;

			if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, ShootDirection + MuzzleLocation, ECollisionChannel::ECC_Camera))
			{
				ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(MuzzleLocation, HitResult.ImpactPoint), MuzzleLocation);
				if (ShowDebug)
				{
					DrawDebugLine(GetWorld(),
						          ShootTo.GetLocation(),
						          ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(CurrentOwner->GetController()->GetControlRotation()) * 20000.0f,
						          FColor::Green,
						          false,
						          5.0f,
						          (uint8)'\000',
						          0.5f);
				}
			}
			else
			{
				ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(MuzzleLocation, ShootDirection + MuzzleLocation), MuzzleLocation);
				if (ShowDebug)
				{
					DrawDebugLine(GetWorld(),
						          ShootTo.GetLocation(),
						          ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(CurrentOwner->GetController()->GetControlRotation()) * 20000.0f,
						          FColor::Yellow,
						          false,
						          5.0f,
						          (uint8)'\000',
						          0.5f);
				}
			}
			Fire(ShootTo);
		}
		else
			FireTime -= DeltaTime;
	}
}

void ABaseWeapon::DispersionTick(float DeltaTime)
{
	if (!WeaponReloading) 
	{
		if (!WeaponFiring)
		{
			if (ShouldReduseDispersion) 
				CurrentDispersion = CurrentDispersion - CurrentDispersionReduction;
			else                        
				CurrentDispersion = CurrentDispersion + CurrentDispersionReduction;
		}

		if (CurrentDispersion < CurrentDispersionMin)
		{
			CurrentDispersion = CurrentDispersionMin;
		}
		else
		{
			if (CurrentDispersion > CurrentDispersionMax)
			{
				CurrentDispersion = CurrentDispersionMax;
			}
		}
	}
}

void ABaseWeapon::WeaponInit()
{
	if (SkeletalMeshWeapon && !SkeletalMeshWeapon->SkeletalMesh)
	{
		SkeletalMeshWeapon->DestroyComponent(true);
	}

	if (StaticMeshWeapon && !StaticMeshWeapon->GetStaticMesh())
	{
		StaticMeshWeapon->DestroyComponent();
	}
}

void ABaseWeapon::SetMaterialLense_OnClient_Implementation()
{
	if (DefaultLenseMaterial && CustomLenseMaterial)
	{
		SceneCapture->Activate();
		UMaterialInstanceDynamic* DynMaterial = LenseMesh->CreateDynamicMaterialInstance(0, CustomLenseMaterial, FName("None"));
		if (DynMaterial)
		{
			TextureTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 1024, 1024, ETextureRenderTargetFormat::RTF_RGBA16f, FLinearColor::Black, false);
			if (TextureTarget)
			{
				SceneCapture->TextureTarget = TextureTarget;
				DynMaterial->SetTextureParameterValue(FName(TEXT("TextureScope")), SceneCapture->TextureTarget);
				///LenseMesh->SetMaterial(0, DynMaterial); Do nothing?
				GetWorld()->GetTimerManager().ClearTimer(ScopeTimerHandle);
			}
		}
	}
}

void ABaseWeapon::Fire_Implementation(FTransform ShootTo)
{
	FireTime = WeaponSetting.RateOfFire;
	WeaponInfo.Round = WeaponInfo.Round - 1;
	FireBP();

	UAnimMontage* ThirdPersonAnim = nullptr;
	UAnimMontage* FirstPersonAnim = nullptr;
	if (WeaponAiming)
	{
		ThirdPersonAnim = WeaponSetting.ThirdPersonFireIronsight;
		FirstPersonAnim = WeaponSetting.FirstPersonFireIronsight;
	}
	else
	{
		ThirdPersonAnim = WeaponSetting.ThirdPersonFireRelax;
		FirstPersonAnim = WeaponSetting.FirstPersonFireRelax;
	}
	OnWeaponFireStart.Broadcast(ThirdPersonAnim,FirstPersonAnim);

	if (WeaponSetting.EffectFireWeapon)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponSetting.EffectFireWeapon, SkeletalMeshWeapon->GetSocketTransform("muzzle"));
		FireWeaponSocketFX_Multicast(WeaponSetting.EffectFireWeapon, SkeletalMeshWeapon->GetSocketTransform("muzzle"));
	}

	if (FireSound)
	{
		
		FireSound_Multicast(FireSound,this->GetActorLocation());
		//UGameplayStatics::SpawnSoundAtLocation(GetWorld(), FireSound, this->GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FireSound = null of other probles"));
	}

	ChangeDispersionByShoot();

	FVector SpawnLocation = ShootTo.GetLocation();
	FRotator SpawnRotation = ShootTo.GetRotation().Rotator();
	FVector EndLocation = GetFireEndLocation();
	FVector Dir = EndLocation - SpawnLocation;
	Dir.Normalize();
	FMatrix myMatrix(Dir, FVector(0, 0, 0), FVector(0, 0, 0),  FVector::ZeroVector);
	SpawnRotation = myMatrix.Rotator(); 
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	//UE_LOG(LogTemp, Warning, TEXT("ABaseWeapon::Fire_Implementation - Owner: %s"), *GetOwner()->GetName());
	SpawnParams.Instigator = GetInstigator();
	//UE_LOG(LogTemp, Warning, TEXT("ABaseWeapon::Fire_Implementation - Instigator: %s"), (GetInstigator() ? *GetInstigator()->GetName() : TEXT("None")));

	AController* Controller = GetInstigatorController();
	
	FProjectileInfo ProjectileInfo;
	ProjectileInfo = GetProjectile();

	FVector SleeveSpawnLocation = SkeletalMeshWeapon->GetSocketLocation("sleeve");
	FRotator SleeveSpawnRotation = SleeveLocation->GetComponentRotation();

	FProjectileInfo SleeveInfo;
	SleeveInfo = ProjectileInfo = GetProjectile();

	FireSpread();

	ABaseProjectile* mySleeve = Cast<ABaseProjectile>(GetWorld()->SpawnActor(SleeveInfo.Sleeve, &SleeveSpawnLocation, &SleeveSpawnRotation, SpawnParams));
	if (mySleeve)
	{
		mySleeve->InitSleeve(WeaponSetting.ProjectileSetting);
	}

	ABaseProjectile* myProjectile = Cast<ABaseProjectile>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &SpawnLocation,&SpawnRotation, SpawnParams));
	if (myProjectile)
	{
		myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
	}
	else
	{ 
		if (!BlockFire)
		{
			FHitResult HitResult;
			TArray<AActor*> Actors;

			UKismetSystemLibrary::LineTraceSingle(GetWorld(),
												  ShootTo.GetLocation(),
												  ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(/*Character*/CurrentOwner->GetController()->GetControlRotation()) * 20000.0f,
												  TraceTypeQuery1,
												  false,
												  Actors,
												  EDrawDebugTrace::None,
												  HitResult,
												  true,
												  FLinearColor::Red,
												  FLinearColor::Green,
												  5.0f);
			if (ShowDebug)
			{

				DrawDebugLine(GetWorld(),
							  ShootTo.GetLocation(),
							  ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(/*Character*/CurrentOwner->GetController()->GetControlRotation()) * 20000.0f,
							  FColor::Green,
							  false,
							  5.0f,
							  (uint8)'\000',
							  0.5f);
			}

			BlockFire = true;
			GetWorld()->GetTimerManager().SetTimer(FireTimerHande, this, &ABaseWeapon::CheckRateOfFire, WeaponSetting.RateOfFire, false);

			if (HitResult.GetActor() && HitResult.PhysMaterial.IsValid())
			{
				EPhysicalSurface mySurfaceType = UGameplayStatics::GetSurfaceType(HitResult);

				if (WeaponSetting.ProjectileSetting.HitDecals.Contains(mySurfaceType))
				{
					UMaterialInterface* myMaterial = WeaponSetting.ProjectileSetting.HitDecals[mySurfaceType];

					if (myMaterial && HitResult.GetComponent())
					{
						UGameplayStatics::SpawnDecalAttached(myMaterial,
							FVector(20.0f),
							HitResult.GetComponent(),
							NAME_None,
							HitResult.ImpactPoint,
							HitResult.ImpactNormal.Rotation(),
							EAttachLocation::KeepWorldPosition,
							5.0f);
					}
				}

				if (WeaponSetting.ProjectileSetting.HitFXs.Contains(mySurfaceType))
				{
					TraceFX_Multicast(WeaponSetting.ProjectileSetting.HitFXs[mySurfaceType], HitResult);
				}

				if (WeaponSetting.EffectFireWeapon)
				{
					FireWeaponFX_Multicast(WeaponSetting.EffectFireWeapon, HitResult);
				}

				/*if (WeaponSetting.ProjectileSetting.HitSound)
				{
					TraceSound_Server(WeaponSetting.ProjectileSetting.HitSound, HitResult);
				}*/

				UGameplayStatics::ApplyPointDamage(HitResult.GetActor(),
												   WeaponSetting.WeaponDamage,
												   HitResult.TraceStart,
												   HitResult,
												   GetInstigatorController(),
												   this,
												   NULL);
			}
		}
	}

	if (GetWeaponRound() <= 0 && !WeaponReloading)
	{
		if (CurrentOwner && CheckCanWeaponReload())
		{
			InitReload();
		}
	}
}

void ABaseWeapon::InitReload()
{
	WeaponReloading = true;
	ReloadTimer = WeaponSetting.ReloadTime;

	if (WeaponSetting.ThirdPersonReload)
	{
		OnWeaponReloadStart.Broadcast(WeaponSetting.ThirdPersonReload, WeaponSetting.FirstPersonReload);
		AnimWeaponStart_Multicast(WeaponSetting.ThirdPersonReload, WeaponSetting.FirstPersonReload);
	}
}

int32 ABaseWeapon::GetAmmoFromBackpack() const
{
	return AmmoBackpack;
}

void ABaseWeapon::ChangeAmmoCountInBackpack(int NewAmmo)
{
	AmmoBackpack = AmmoBackpack + NewAmmo;
}

void ABaseWeapon::FinishReload()
{
	WeaponReloading = false;
	int32 AmmoNeedTake = WeaponSetting.MaxRound - WeaponInfo.Round; 

	if (GetAmmoFromBackpack() >= AmmoNeedTake)
	{
		int NewAmmoForBackpack = GetAmmoFromBackpack() - AmmoNeedTake;
		ChangeAmmoCountInBackpack(-AmmoNeedTake);
		WeaponInfo.Round = WeaponInfo.Round + AmmoNeedTake;
	}
	else
	{
		WeaponInfo.Round = WeaponInfo.Round + GetAmmoFromBackpack();
		ChangeAmmoCountInBackpack(-GetAmmoFromBackpack());
	}
	
	OnWeaponReloadEnd.Broadcast(); 
}

void ABaseWeapon::CancelReload()
{
	WeaponReloading = false;
	OnWeaponReloadEnd.Broadcast();
}

void ABaseWeapon::ChangeDispersionByShoot()
{
	CurrentDispersion = CurrentDispersion + CurrentDispersionRecoil;
}

float ABaseWeapon::GetCurrentDispersion() const
{
	float Result = CurrentDispersion;
	return Result;
}

bool ABaseWeapon::CheckWeaponCanFire()
{
	return !BlockFire;
}

bool ABaseWeapon::CheckCanWeaponReload()
{
	if (GetAmmoFromBackpack() > 0)
	{
		return true;
	}
	else
	{
		return false;
		CancelReload();
	}
	return false;
	CancelReload();
}

void ABaseWeapon::SetWeaponStateFire_OnServer_Implementation(bool bIsFire)
{
	if (CheckWeaponCanFire())
	{
		WeaponFiring = bIsFire;
	}
	else
	{
		WeaponFiring = false;
	}
	FireTime = 0.01f;
}

void ABaseWeapon::InitAiming()
{
	if (bIsRailGun)
	{
		ShowScopeTimeline(0.2f,true);
	}
}

void ABaseWeapon::FireSpread_Implementation()
{
	///BaseRecoil = 0.25f;
	///RecoilCoef = 2.0f;
	///MultiplierSpread = -1.0f;

	float PitchRecoil = BaseRecoil * MultiplierSpread;
	float YawRecoil = (PitchRecoil / RecoilCoef * FMath::RandRange(PitchRecoil / RecoilCoef * MultiplierSpread, PitchRecoil / RecoilCoef));

	if (CurrentOwner)
	{
		CurrentOwner->AddControllerPitchInput(PitchRecoil);
		CurrentOwner->AddControllerYawInput(YawRecoil);
	}
}

void ABaseWeapon::ShowScopeTimeline(float Value, bool bIsAiming)
{
	bIsAiming ? GetWorld()->GetTimerManager().SetTimer(ScopeTimerHandle, this, &ABaseWeapon::SetMaterialLense_OnClient, Value, false) : GetWorld()->GetTimerManager().SetTimer(ScopeTimerHandle, this, &ABaseWeapon::RemoveMaterialLense, Value, false);
}

void ABaseWeapon::TraceFX_Multicast_Implementation(UParticleSystem* FX, FHitResult HitResult)
{
	if (FX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
												 FX,
												 FTransform(HitResult.ImpactNormal.Rotation(),
												 HitResult.ImpactPoint,
												 FVector(1.0f)));
	}
}

void ABaseWeapon::FireWeaponSocketFX_Multicast_Implementation(UParticleSystem* newFX, FTransform SocketTransform)
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), newFX, SocketTransform);
}

void ABaseWeapon::FireWeaponFX_Multicast_Implementation(UParticleSystem* FX, FHitResult HitResult)
{
	if (FX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
												 FX,
												 FTransform(HitResult.ImpactNormal.Rotation(),
												 HitResult.TraceStart,
												 FVector(1.0f)));
	}
}

//void ABaseWeapon::TraceSound_Server_Implementation(USoundBase* HitSound, FHitResult HitResult)
//{
//	TraceSound_Multicast(HitSound,HitResult);
//}
//
//void ABaseWeapon::TraceSound_Multicast_Implementation(USoundBase* HitSound, FHitResult HitResult)
//{
//	if (HitSound)
//	{
//		UGameplayStatics::PlaySoundAtLocation(GetWorld(),
//			WeaponSetting.ProjectileSetting.HitSound,
//			HitResult.ImpactNormal);
//	}
//}

void ABaseWeapon::CancelAiming_Implementation()
{
	if (bIsRailGun)
	{
		ShowScopeTimeline(0.2f,false);
	}
}

void ABaseWeapon::SetAnimationForHunkHero_BP_Implementation() {}

void ABaseWeapon::SetAnimationForSwatHero_BP_Implementation() {}

void ABaseWeapon::FireBP_Implementation()	{}

void ABaseWeapon::CheckRateOfFire()
{
	BlockFire = false;
}

void ABaseWeapon::RemoveMaterialLense()
{
	LenseMesh->SetMaterial(0, DefaultLenseMaterial);
	SceneCapture->Deactivate();
}

void ABaseWeapon::AnimWeaponStart_Multicast_Implementation(UAnimMontage* AnimThirdPerson, UAnimMontage* AnimFirstPerson)
{
	if (CurrentOwner && AnimThirdPerson && AnimFirstPerson && SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())
	{
		CurrentOwner->PlayWeaponReloadMontage_Multicast(AnimThirdPerson, AnimFirstPerson);
	}
}

void ABaseWeapon::UpdateStateWeapon_OnServer_Implementation(EMovementState NewMovementState)
{
	switch (NewMovementState)
	{
	case EMovementState::AimWalk_State:
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimReduction;
		WeaponAiming = true;
		InitAiming();
		break;
	case EMovementState::Run_State:
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Run_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Run_StateDispersionAimReduction;
		WeaponAiming = false;
		CancelAiming();
		break;
	default:
		break;
	}
}

void ABaseWeapon::UpdateWeaponByCharacterMovementStateOnServer_Implementation(FVector NewShootEndLocation, bool NewShouldReduceDispersion)
{
	ShootEndLocation = NewShootEndLocation;
	ShouldReduseDispersion = NewShouldReduceDispersion;
}

FVector ABaseWeapon::GetFireEndLocation() const
{
	bool bShootDirection = false;
	FVector FactEndLocation = FVector(0.0f);
	//CurrentOwner ? FactEndLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight") + ApplyDispersionToShoot(UKismetMathLibrary::GetForwardVector(CurrentOwner->GetController()->GetControlRotation()) * 20000.0f) : void(0);

	if (CurrentOwner && SkeletalMeshWeapon)
	{
		FactEndLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight") + ApplyDispersionToShoot(UKismetMathLibrary::GetForwardVector(CurrentOwner->GetController()->GetControlRotation()) * 20000.0f);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Current Owner or SkeletalMeshWeapon is NULL"));
		//return FactEndLocation;
	}

	return FactEndLocation;
}

FVector ABaseWeapon::ApplyDispersionToShoot(FVector DirectionShoot) const
{
	return FMath::VRandCone(DirectionShoot, GetCurrentDispersion() * PI / 180.f);
}

int32 ABaseWeapon::GetWeaponRound() const
{
	return WeaponInfo.Round;
}

FProjectileInfo ABaseWeapon::GetProjectile()
{
	return WeaponSetting.ProjectileSetting;
}

EWeaponType ABaseWeapon::GetWeaponType() const
{
	return WeaponSetting.WeaponType;
}

void ABaseWeapon::FireSound_Multicast_Implementation(USoundBase* Sound, FVector Location)
{
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Sound, Location);
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWeapon, WeaponReloading);
	DOREPLIFETIME(ABaseWeapon, WeaponAiming);
	DOREPLIFETIME(ABaseWeapon, ReloadTimer);
	DOREPLIFETIME(ABaseWeapon, ShootEndLocation);
	DOREPLIFETIME(ABaseWeapon, WeaponInfo);
}


