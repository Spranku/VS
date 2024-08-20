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

	///SetReplicates(true);

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

	AVSCharacter* MyChar = Cast<AVSCharacter>(GetOwner());
	if (MyChar)
	{
		Character = MyChar;
		UE_LOG(LogTemp, Warning, TEXT("Success cast to Character"));	
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed cast to Character"));
	}

	WeaponInit();

	if (!CurrentOwner)
	{
		SkeletalMeshWeapon->SetVisibility(true);
	}
}
// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireTick(DeltaTime);
	ReloadTick(DeltaTime);
	DispersionTick(DeltaTime);
	//ClipDropTick(DeltaTime);
	//ShellDropTick(DeltaTime);
}

void ABaseWeapon::ReloadTick(float DeltaTime)
{
	if (WeaponReloading || GetWeaponRound() < 0)
	{
		/// UE_LOG(LogTemp, Warning, TEXT("WeaponReloading || GetWeaponRound = true"));

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
			FVector ShootDirection = Character->GetForwardVectorFromCamera() * 10000.0f; /// 
			FTransform ShootTo;
			FHitResult HitResult;

			if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLocation, ShootDirection + MuzzleLocation, ECollisionChannel::ECC_Visibility))
			{
				ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(MuzzleLocation, HitResult.ImpactPoint), MuzzleLocation);
				/*DrawDebugLine(GetWorld(),
					ShootTo.GetLocation(),
					ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f,
					FColor::Green,
					false,
					5.0f,
					(uint8)'\000',
					0.5f);*/
			}
			else
			{
				ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(MuzzleLocation, ShootDirection + MuzzleLocation), MuzzleLocation);
				/*DrawDebugLine(GetWorld(),
					ShootTo.GetLocation(),
					ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f,
					FColor::Yellow,
					false,
					5.0f,
					(uint8)'\000',
					0.5f);*/
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
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::SetMaterialLense_OnClient_Implementation - drop material"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::SetMaterialLense_OnClient_Implementation - drop material"));
	}
	
}

void ABaseWeapon::Fire_Implementation(FTransform ShootTo)
{
	FireTime = WeaponSetting.RateOfFire;
	WeaponInfo.Round = WeaponInfo.Round - 1;
	
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

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponSetting.EffectFireWeapon, SkeletalMeshWeapon->GetSocketTransform("muzzle"));

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
	SpawnParams.Instigator = GetInstigator();

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
		///	////////////////////HitScan LineTrace////////////////////// 
		UE_LOG(LogTemp, Warning, TEXT("HitScan LineTrace"));

		FHitResult HitResult;
		TArray<AActor*> Actors;

		UKismetSystemLibrary::LineTraceSingle(GetWorld(),
			ShootTo.GetLocation(),
			ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f,
			TraceTypeQuery4,
			false,
			Actors,
			EDrawDebugTrace::ForDuration,
			HitResult,
			true,
			FLinearColor::Red,
			FLinearColor::Green,
			5.0f);

		DrawDebugLine(GetWorld(),
			ShootTo.GetLocation(),
			ShootTo.GetLocation() + UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f,
			FColor::Green,
			false,
			5.0f,
			(uint8)'\000',
			0.5f);

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

				/*UParticleSystem* myParticle = WeaponSetting.ProjectileSetting.HitFXs[mySurfaceType];
				if (myParticle)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
						myParticle,
						FTransform(Hit.ImpactNormal.Rotation(),
						Hit.ImpactPoint,
						FVector(1.0f)));
				}*/
			}

			if (WeaponSetting.ProjectileSetting.HitSound)
			{
				TraceSound_Multicast(WeaponSetting.ProjectileSetting.HitSound, HitResult);

				/*UGameplayStatics::PlaySoundAtLocation(GetWorld(),
					WeaponSetting.ProjectileSetting.HitSound,
					Hit.ImpactNormal);*/

			}

			UGameplayStatics::ApplyPointDamage(HitResult.GetActor(),
										       WeaponSetting.ProjectileSetting.ProjectileDamage,
											   HitResult.TraceStart,
				                               HitResult,
				                               GetInstigatorController(),
				                               this,
				                               NULL);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::Fire_Implementation - HitResult.GetActor() or HitResult.PhysMaterial Is Not Valid!!!"));
		}
	}

	if (GetWeaponRound() <= 0 && !WeaponReloading)
	{
		if (CheckCanWeaponReload())
			InitReload();
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
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::InitReload() - WeaponSetting.ThirdPersonReload = false"));
	}

	//if (WeaponSetting.ClipDropMesh.DropMesh)
	//{
	//	DropClipFlag = true;
	//	DropClipTimer = WeaponSetting.ClipDropMesh.DropMeshTime;
	//}
}

void ABaseWeapon::FinishReload()
{
	WeaponReloading = false;
	WeaponInfo.Round = WeaponSetting.MaxRound;

	//int8 AviableAmmoFromInventory =  GetAviableAmmoForReload();
	int8 AmmoNeedTakeFromInv;
	//int8 NeedToReload = WeaponSetting.MaxRound - AdditionalWeaponInfo.Round;

	//if (NeedToReload > AviableAmmoFromInventory)
	//{
	//	AdditionalWeaponInfo.Round = AviableAmmoFromInventory;
	//	AmmoNeedTakeFromInv = AviableAmmoFromInventory;
	//}
	//else
	//{
	//	AdditionalWeaponInfo.Round += NeedToReload;
	//	AmmoNeedTakeFromInv = NeedToReload;
	//}
	OnWeaponReloadEnd.Broadcast(true, -AmmoNeedTakeFromInv);
}

void ABaseWeapon::CancelReload()
{
	WeaponReloading = false;
	/*if (SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())
		SkeletalMeshWeapon->GetAnimInstance()->StopAllMontages(0.15f);

	OnWeaponReloadEnd.Broadcast(false, 0);*/
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
	return  true /*!BlockFire*/;
}

bool ABaseWeapon::CheckCanWeaponReload()
{
	/*bool result = true;
	if (GetOwner())
	{
		UTPSInventoryComponent* MyInv = Cast<UTPSInventoryComponent>(GetOwner()->GetComponentByClass(UTPSInventoryComponent::StaticClass()));
		if (MyInv)
		{
			int8 AviableAmmoForWeapon;
			if (!MyInv->CheckAmmoForWeapon(WeaponSetting.WeaponType, AviableAmmoForWeapon))
			{
				result = false;
			}
		}
	}
	return result;*/
	return true;
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

void ABaseWeapon::FireSpread()
{
	/*float*/ BaseRecoil = 0.25f;
	/*float*/ RecoilCoef = 2.0f;
	/*float*/ MultiplierSpread = -1.0f;

	float PitchRecoil = BaseRecoil * MultiplierSpread;
	float YawRecoil = (PitchRecoil / RecoilCoef * FMath::RandRange(PitchRecoil / RecoilCoef * MultiplierSpread, PitchRecoil / RecoilCoef));

	if (Character)
	{
		Character->AddControllerPitchInput(PitchRecoil);
		Character->AddControllerYawInput(YawRecoil);
	}
}

void ABaseWeapon::ShowScopeTimeline(float Value, bool bIsAiming)
{
	bIsAiming ? GetWorld()->GetTimerManager().SetTimer(ScopeTimerHandle, this, &ABaseWeapon::SetMaterialLense_OnClient, Value, false) : GetWorld()->GetTimerManager().SetTimer(ScopeTimerHandle, this, &ABaseWeapon::RemoveMaterialLense, Value, false);
}

void ABaseWeapon::TraceFX_Multicast_Implementation(UParticleSystem* FxTemplate, FHitResult HitResult)
{
	if (FxTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
												 FxTemplate,
												 FTransform(HitResult.ImpactNormal.Rotation(),
												 HitResult.ImpactPoint,
												 FVector(1.0f)));
	}
}

void ABaseWeapon::TraceSound_Multicast_Implementation(USoundBase* HitSound, FHitResult HitResult)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),
										      WeaponSetting.ProjectileSetting.HitSound,
										      HitResult.ImpactNormal);
	}
}

void ABaseWeapon::CancelAiming_Implementation()
{
	if (bIsRailGun)
	{
		ShowScopeTimeline(0.2f,false);
	}
}

void ABaseWeapon::RemoveMaterialLense()
{
	LenseMesh->SetMaterial(0, DefaultLenseMaterial);
	SceneCapture->Deactivate();
}

void ABaseWeapon::AnimWeaponStart_Multicast_Implementation(UAnimMontage* AnimThirdPerson, UAnimMontage* AnimFirstPerson)
{
	if (Character && AnimThirdPerson && AnimFirstPerson && SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())//Bad Code? maybe best way init local variable or in func
	{
		Character->PlayReloadMontage_Multicast(AnimThirdPerson, AnimFirstPerson);
	}
}

void ABaseWeapon::UpdateStateWeapon_OnServer_Implementation(EMovementState NewMovementState)
{
	/// BlockFire = false; for crouch

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
	//ChangeDispersion();
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
	Character ? FactEndLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight") + ApplyDispersionToShoot(UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f) : void(0);

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

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABaseWeapon, AdditionalWeaponInfo);
	DOREPLIFETIME(ABaseWeapon, WeaponReloading);
	DOREPLIFETIME(ABaseWeapon, WeaponAiming);
	DOREPLIFETIME(ABaseWeapon, ShootEndLocation);
}


