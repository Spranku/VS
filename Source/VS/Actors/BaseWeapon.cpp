// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
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

	ShootLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShootLocation"));
	ShootLocation->SetupAttachment(RootComponent);

	bReplicates = true;
}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

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
			}
			else
			{
				ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(MuzzleLocation, ShootDirection + MuzzleLocation), MuzzleLocation);
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

void ABaseWeapon::Fire_Implementation(FTransform ShootTo)
{
	FireTime = WeaponSetting.RateOfFire;
	WeaponInfo.Round = WeaponInfo.Round - 1;

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

	ABaseProjectile* myProjectile = Cast<ABaseProjectile>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &SpawnLocation,&SpawnRotation, SpawnParams));
	if (myProjectile)
	{
		myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
	}
	else
	{ 
		UE_LOG(LogTemp, Error, TEXT("Failed spawn"));
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
	//ReloadTimer = 1.8f; 
	ReloadTimer = WeaponSetting.ReloadTime;

	if (WeaponSetting.ThirdPersonReload)
	{
		OnWeaponReloadStart.Broadcast(WeaponSetting.ThirdPersonReload);
		AnimWeaponStart_Multicast(WeaponSetting.ThirdPersonReload);
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

FVector ABaseWeapon::ApplyDispersionToShoot(FVector DirectionShoot) const
{
	return FMath::VRandCone(DirectionShoot, GetCurrentDispersion() * PI / 180.f);
}

float ABaseWeapon::GetCurrentDispersion() const
{
	float Result = CurrentDispersion;
	return Result;
}

FVector ABaseWeapon::GetFireEndLocation() const
{
	bool bShootDirection = false;
	//FVector EndLocation = FVector(0.f);
	//FVector tmpV = (SkeletalMeshWeapon->GetSocketLocation("Ironsight") - ShootEndLocation);
	//if (tmpV.Size() > 100.0f)
	//{
	//	EndLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight") + ApplyDispersionToShoot((SkeletalMeshWeapon->GetSocketLocation("Ironsight") - ShootEndLocation).GetSafeNormal() * -20000.0f);
	//	//UE_LOG(LogTemp, Error, TEXT("True"));
	//}
	//else
	//{
	//	EndLocation = /*SkeletalMeshWeapon->GetSocketLocation("Ironsight")*/ ShootLocation->GetComponentLocation() + ApplyDispersionToShoot(/*SkeletalMeshWeapon->GetSocketLocation("Ironsight")*/ShootLocation->GetForwardVector()) * 20000.0f;
	//	UE_LOG(LogTemp, Error, TEXT("False"));
	//}
	//UE_LOG(LogTemp, Error, TEXT("EndLocation: %s"), *EndLocation.ToString());
	// return EndLocation;

	FVector FactEndLocation = FVector(0.0f);
	if (Character)
	{
		//FHitResult HitResult;
		//GetWorld()->LineTraceSingleByChannel(HitResult, SkeletalMeshWeapon->GetSocketLocation("Ironsight"), (UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f) + SkeletalMeshWeapon->GetSocketLocation("Ironsight"), ECollisionChannel::ECC_Visibility);
		FactEndLocation = SkeletalMeshWeapon->GetSocketLocation("Ironsight") + ApplyDispersionToShoot(UKismetMathLibrary::GetForwardVector(Character->GetController()->GetControlRotation()) * 20000.0f);
	}
	return FactEndLocation;
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

int32 ABaseWeapon::GetWeaponRound()
{
	return WeaponInfo.Round;
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

FProjectileInfo ABaseWeapon::GetProjectile()
{
	return WeaponSetting.ProjectileSetting;
}

void ABaseWeapon::AnimWeaponStart_Multicast_Implementation(UAnimMontage* AnimMontage)
{
	if (AnimMontage && SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())//Bad Code? maybe best way init local variable or in func
	{
		SkeletalMeshWeapon->GetAnimInstance()->Montage_Play(AnimMontage);
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
		break;
	case EMovementState::Run_State:
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Run_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Run_StateDispersionAimReduction;
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

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABaseWeapon, AdditionalWeaponInfo);
	DOREPLIFETIME(ABaseWeapon, WeaponReloading);
	DOREPLIFETIME(ABaseWeapon, ShootEndLocation);
	
}
