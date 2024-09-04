// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseProjectile.h"
#include <Kismet/GameplayStatics.h> 
#include "Perception/AISense_Damage.h"
#include <PhysicalMaterials/PhysicalMaterial.h>
#include "/Projects/VS/Source/VS/FuncLibrary/Types.h"

// Sets default values
ABaseProjectile::ABaseProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BulletCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	BulletCollisionSphere->SetSphereRadius(16.f);
	BulletCollisionSphere->bReturnMaterialOnMove = true;//hit event return physMaterial
	BulletCollisionSphere->SetCanEverAffectNavigation(false);//collision not affect navigation (P keybord on editor)

	RootComponent = BulletCollisionSphere;

	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bullet Projectile Mesh"));
	BulletMesh->SetupAttachment(RootComponent);
	BulletMesh->SetCanEverAffectNavigation(false);

	BulletFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Bullet FX"));
	BulletFX->SetupAttachment(RootComponent);

	
	BulletProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Bullet ProjectileMovement"));
	BulletProjectileMovement->UpdatedComponent = RootComponent;
	//BulletProjectileMovement->InitialSpeed = 1.f;
	//BulletProjectileMovement->MaxSpeed = 0.f;

	BulletSound = CreateDefaultSubobject<UAudioComponent>(TEXT("BulletAudio"));
	BulletSound->SetupAttachment(RootComponent);

	BulletProjectileMovement->bRotationFollowsVelocity = true;
	BulletProjectileMovement->bShouldBounce = true;

	bReplicates = true;
}

// Called when the game starts or when spawned
void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	BulletCollisionSphere->OnComponentHit.AddDynamic(this, &ABaseProjectile::BulletCollisionSphereHit);
	BulletCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::BulletCollisionSphereBeginOverlap);
	BulletCollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseProjectile::BulletCollisionSphereEndOverlap);
}

// Called every frame
void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseProjectile::InitProjectile(FProjectileInfo InitParam)
{
	this->SetLifeSpan(InitParam.ProjectileLifeTime);

	if (InitParam.ProjectileStaticMesh)
	{
		InitVisualMeshProjectile_Multicast(InitParam.ProjectileStaticMesh, InitParam.ProjectileStaticMeshOffset);
	}
	else
	{
		BulletMesh->DestroyComponent();
	}

	if (InitParam.ProjectileTrialFX)
	{
		InitVisualTrailProjectile_Multicast(InitParam.ProjectileTrialFX, InitParam.ProjectileTrialFxOffset);
	}
	else
		BulletFX->DestroyComponent();

	InitVelocity_Multicast(InitParam.ProjcetileInitSpeed, BulletProjectileMovement->MaxSpeed = InitParam.ProjcetileInitSpeed);

	ProjectileSetting = InitParam;
}

void ABaseProjectile::InitSleeve(FProjectileInfo InitParam)
{
	this->SetLifeSpan(InitParam.SleeveLifeTime);

	if (InitParam.SleeveStaticMesh)
	{
		InitVisualMeshProjectile_Multicast(InitParam.SleeveStaticMesh, InitParam.ProjectileStaticMeshOffset);
	}
	else
	{
		BulletMesh->DestroyComponent();
	}
	InitVelocity_Multicast(InitParam.SleeveInitSpeed, BulletProjectileMovement->MaxSpeed = InitParam.SleeveMaxSpeed);
	ProjectileSetting = InitParam;
}

void ABaseProjectile::BulletCollisionSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && Hit.PhysMaterial.IsValid())
	{
		EPhysicalSurface mySurfaceType = UGameplayStatics::GetSurfaceType(Hit);

		if (ProjectileSetting.HitDecals.Contains(mySurfaceType))
		{
			UMaterialInterface* myMaterial = ProjectileSetting.HitDecals[mySurfaceType];
	
			if (myMaterial && OtherComp)
			{
				SpawnHitDecal_Multicast(myMaterial, OtherComp, Hit);
			}
		}
		
		if (ProjectileSetting.HitFXs.Contains(mySurfaceType))
		{
			UParticleSystem* myParticle = ProjectileSetting.HitFXs[mySurfaceType];
	
			if (mySurfaceType)
			{
				SpawnHitFX_Multicast(myParticle, Hit);
			}
		}
		
		if (ProjectileSetting.HitSound)
		{
			SpawnHitSound_Multicast(ProjectileSetting.HitSound, Hit);
		}

		///	UType::AddEffecttBySurfaceType(Hit.GetActor(), Hit.BoneName, ProjectileSetting.Effect, mySurfaceType);
	}


	/*if (GetInstigatorController())
	{
		UE_LOG(LogTemp, Warning, TEXT("---ABaseProjectile::BulletCollisionSphereHit - Success GetInstigatorController()---"));
		FString Name = GetInstigatorController()->GetName();
		///UE_LOG(LogTemp, Warning, TEXT("The Actor's name is %s"), *GetInstigatorController() - &gt; GetName());
		UE_LOG(LogTemp, Warning, TEXT("The Actor's name is %s"), *Name);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("---ABaseProjectile::BulletCollisionSphereHit - Failed GetInstigatorController()---"));
	}*/
	

	UGameplayStatics::ApplyPointDamage(OtherActor, ProjectileSetting.ProjectileDamage, Hit.TraceStart, Hit, GetInstigatorController(), this, NULL);
	UAISense_Damage::ReportDamageEvent(GetWorld(), Hit.GetActor(), GetInstigator(), ProjectileSetting.ProjectileDamage, Hit.Location, Hit.Location);
	ImpactProjectile();
}

void ABaseProjectile::BulletCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void ABaseProjectile::BulletCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void ABaseProjectile::ImpactProjectile()
{
	this->Destroy();
}

void ABaseProjectile::InitVisualMeshProjectile_Multicast_Implementation(UStaticMesh* newMesh, FTransform MeshRelative)
{
	BulletMesh->SetStaticMesh(newMesh);
	BulletMesh->SetRelativeTransform(MeshRelative);
}

void ABaseProjectile::InitVisualTrailProjectile_Multicast_Implementation(UParticleSystem* NewTemplate, FTransform TemplateRelative)
{
	BulletFX->SetTemplate(NewTemplate);
	BulletFX->SetRelativeTransform(TemplateRelative);
}

void ABaseProjectile::InitVelocity_Multicast_Implementation(float InitSpeed, float MaxSpeed)
{
	if (BulletProjectileMovement)
	{
		BulletProjectileMovement->Velocity = GetActorForwardVector() * InitSpeed;
		BulletProjectileMovement->MaxSpeed = MaxSpeed;
		BulletProjectileMovement->InitialSpeed = InitSpeed;
	}
}

void ABaseProjectile::SpawnHitDecal_Multicast_Implementation(UMaterialInterface* DecalMaterial, UPrimitiveComponent* OtherComp, FHitResult HitResult)
{
	UGameplayStatics::SpawnDecalAttached(DecalMaterial, FVector(20.0f), OtherComp, NAME_None, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, 10.0f);
}

void ABaseProjectile::SpawnHitFX_Multicast_Implementation(UParticleSystem* FxTemplate, FHitResult HitResult)
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FxTemplate, FTransform(HitResult.ImpactNormal.Rotation(), HitResult.ImpactPoint, FVector(1.0f)));
}

void ABaseProjectile::SpawnHitSound_Multicast_Implementation(USoundBase* HitSound, FHitResult HitResult)
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitResult.ImpactPoint);
}

void ABaseProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (BulletProjectileMovement)
	{
		BulletProjectileMovement->Velocity = NewVelocity;
	}
}
