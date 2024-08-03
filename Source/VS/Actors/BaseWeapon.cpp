// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "/Projects/VS/Source/VS/VSCharacter.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	SkeletalMeshWeapon = CreateDefaultSubobject <USkeletalMeshComponent > (TEXT("SkeletalMeshWeapon"));
	SkeletalMeshWeapon->SetGenerateOverlapEvents(false);
	SkeletalMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshWeapon->SetupAttachment(RootComponent);

	StaticMeshWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshWeapon"));
	StaticMeshWeapon->SetGenerateOverlapEvents(false);
	StaticMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));

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
		//StaticMeshWeapon->SetVisibility(true);
	}
}

// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	///if (HasAuthority())
	///{
		FireTick(DeltaTime);
		ReloadTick(DeltaTime);
		//DispersionTick(DeltaTime);
		//ClipDropTick(DeltaTime);
		//ShellDropTick(DeltaTime);
	///}
	
}

void ABaseWeapon::ReloadTick(float DeltaTime)
{
	if (WeaponReloading)
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
			Fire();
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

void ABaseWeapon::Fire()
{
	UE_LOG(LogTemp, Warning, TEXT("FIRE"));
	FireTime = WeaponSetting.RateOfFire;
	WeaponInfo.Round = WeaponInfo.Round - 1;

	FTransform ShootTo;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();

	FProjectileInfo ProjectileInfo;
	ProjectileInfo = GetProjectile();

	FHitResult HitResult;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, SkeletalMeshWeapon->GetSocketLocation("Ironsight"), Character->GetForwardVectorFromCamera() * 10000.0f + SkeletalMeshWeapon->GetSocketLocation("Ironsight"),ECollisionChannel::ECC_Visibility))
	{
		ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(SkeletalMeshWeapon->GetSocketLocation("Ironsight"), HitResult.ImpactPoint), SkeletalMeshWeapon->GetSocketLocation("Ironsight"));
	}
	else
	{
		ShootTo = FTransform(UKismetMathLibrary::FindLookAtRotation(HitResult.ImpactPoint, Character->GetForwardVectorFromCamera() * 10000.0f + SkeletalMeshWeapon->GetSocketLocation("Ironsight")), SkeletalMeshWeapon->GetSocketLocation("Ironsight"));
	}

	SpawnProjectileOnServer(ShootTo);
}

void ABaseWeapon::SpawnProjectileOnServer_Implementation(FTransform TransformToSpawn)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();

	FProjectileInfo ProjectileInfo;
	ProjectileInfo = GetProjectile();

	ABaseProjectile* myProjectile = Cast<ABaseProjectile>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &TransformToSpawn, SpawnParams));

	//ABaseProjectile* myProjectile = Cast<ABaseProjectile>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &TransformToSpawn.GetLocation(), &TransformToSpawn.GetRotation(), SpawnParams));
	if (myProjectile)
	{
		myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
		UE_LOG(LogTemp, Warning, TEXT("Success spawn"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed spawn"));
	}
}

void ABaseWeapon::InitReload()
{
	WeaponReloading = true;
	//ReloadTimer = 1.8f; 

	ReloadTimer = WeaponSetting.ReloadTime;
	UE_LOG(LogTemp, Warning, TEXT(" ABaseWeapon::InitReload - Success"));
	//if (WeaponSetting.AnimCharReload)
	//{
	//	OnWeaponReloadStart.Broadcast(WeaponSetting.AnimCharReload);
	//	AnimWeaponStart_Multicast(WeaponSetting.AnimCharReload);
	//}

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

	//int8 AviableAmmoFromInventory = GetAviableAmmoForReload();
	//int8 AmmoNeedTakeFromInv;
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


	//OnWeaponReloadEnd.Broadcast(true, -AmmoNeedTakeFromInv);
}

void ABaseWeapon::CancelReload()
{
	WeaponReloading = false;
	/*if (SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())
		SkeletalMeshWeapon->GetAnimInstance()->StopAllMontages(0.15f);

	OnWeaponReloadEnd.Broadcast(false, 0);*/
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

void ABaseWeapon::SetWeaponStateFire_OnServer_Implementation(bool bIsFire, float Pitch)
{
	if (CheckWeaponCanFire())
	{
		WeaponFiring = bIsFire;
		ServerPitch = Pitch;
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

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABaseWeapon, AdditionalWeaponInfo);
	DOREPLIFETIME(ABaseWeapon, WeaponReloading);
}
