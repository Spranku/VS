// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseWeapon.h"

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

}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	//WeaponInit();

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

	if (HasAuthority())
	{
		FireTick(DeltaTime);
		ReloadTick(DeltaTime);
		DispersionTick(DeltaTime);
		//ClipDropTick(DeltaTime);
		//ShellDropTick(DeltaTime);
	}

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
	//UAnimMontage* AnimToPlay = nullptr;
	//*if (WeaponAiming)
	//	AnimToPlay = WeaponSetting.AnimWeaponInfo.AnimCharFireAim;
	//else
	//	AnimToPlay = WeaponSetting.AnimWeaponInfo.AnimCharFire;*/

	//if (WeaponSetting.AnimWeaponInfo.AnimWeaponFire)
	//{
	//	AnimWeaponStart_Multicast(WeaponSetting.AnimWeaponInfo.AnimWeaponFire);
	//}

	//*if (WeaponSetting.ShellBullets.DropMesh)
	//{
	//	if (WeaponSetting.ShellBullets.DropMeshTime < 0.0f)
	//	{
	//		InitDropMesh_OnServer(WeaponSetting.ShellBullets.DropMesh,
	//			WeaponSetting.ShellBullets.DropMeshOffset,
	//			WeaponSetting.ShellBullets.DropMeshImpulseDir,
	//			WeaponSetting.ShellBullets.DropMeshLifeTime,
	//			WeaponSetting.ShellBullets.ImpulseRandomDispersion,
	//			WeaponSetting.ShellBullets.PowerImpulse,
	//			WeaponSetting.ShellBullets.CustomMass);
	//	}
	//	else
	//	{
	//		DropShellFlag = true;
	//		DropShellTimer = WeaponSetting.ShellBullets.DropMeshTime;
	//	}
	//}
	//
	FireTime = WeaponSetting.RateOfFire;

	//AdditionalWeaponInfo.Round = AdditionalWeaponInfo.Round - 1; \
	//	ChangeDispersionByShoot();*/

	//OnWeaponFireStart.Broadcast(AnimToPlay);

	//FXWeaponFire_Multicast(WeaponSetting.EffectFireWeapon, WeaponSetting.SoundFireWeapon);

	//int8 NumberProjectile = GetNumberProjectileByShot();

	if (ShootLocation)
	{

		FVector SpawnLocation = ShootLocation->GetComponentLocation();
		FRotator SpawnRotation = ShootLocation->GetComponentRotation();

		FProjectileInfo ProjectileInfo;
		ProjectileInfo = GetProjectile();
		FVector EndLocation;

		//

		//	for (int8 i = 0; i < NumberProjectile; i++)
		//	{
		//		EndLocation = GetFireEndLocation();

		//	
		if (ProjectileInfo.Projectile)
		{
			//			
			//			FVector Dir = /*ShootEndLocation*/ GetFireEndLocation() - SpawnLocation;
			//			Dir.Normalize();
			//		
			//			FMatrix myMatrix(Dir, FVector(0, 0, 0), FVector(0, 0, 0), FVector::ZeroVector);
			//			SpawnRotation = myMatrix.Rotator();

			
		
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();

			ABaseProjectile* myProjectile = Cast<ABaseProjectile>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &SpawnLocation, &SpawnRotation, SpawnParams));
			if (myProjectile /* && myProjectile != nullptr*/)
			{
				myProjectile->InitialLifeSpan = 20.0f;
				UE_LOG(LogTemp, Warning, TEXT(" ABaseWeapon::Fire - cuccess spawn projectile;"));
				//Projectile->BulletProjectileMovement->InitialSpeed = 2500.0f;

//				myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
			}
			else
			{

			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT(" ABaseWeapon::Fire - (ProjectileInfo.Projectile = NULL;"));
		}
	}
}

void ABaseWeapon::InitReload()
{
	WeaponReloading = true;
	ReloadTimer = 1.8f/*WeaponSetting.ReloadTime*/; 

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
	return 30;
}

void ABaseWeapon::SetWeaponStateFire_OnServer_Implementation(bool bIsFire)
{
	if (CheckWeaponCanFire())
		WeaponFiring = bIsFire;
	else
		WeaponFiring = false;
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
	//DOREPLIFETIME(AWeaponDefault, ShootEndLocation);
}
