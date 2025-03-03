// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "/Projects/VS/Source/VS/FuncLibrary/Types.h"
#include "Components/AudioComponent.h"
#include "BaseProjectile.generated.h"

UCLASS()
class VS_API ABaseProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, /*meta = (AllowPrivateAccess = "true")*/ Category = Components)
	class UStaticMeshComponent* BulletMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, /*meta = (AllowPrivateAccess = "true")*/ Category = Components)
	class UParticleSystemComponent* BulletFX = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, /*meta = (AllowPrivateAccess = "true"),*/ Category = Components)
	class USphereComponent* BulletCollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, /*meta = (AllowPrivateAccess = "true")*/ Category = Components)
	class UProjectileMovementComponent* BulletProjectileMovement = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, /*meta = (AllowPrivateAccess = "true")*/ Category = Components)
	class UAudioComponent* BulletSound = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FProjectileInfo ProjectileSetting;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void InitProjectile(FProjectileInfo InitParam);

	UFUNCTION()
	void InitSleeve(FProjectileInfo InitParam);

	UFUNCTION()
	virtual void BulletCollisionSphereHit(class UPrimitiveComponent* HitComp,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			FVector NormalImpulse,
			const FHitResult& Hit);
	UFUNCTION()
	void BulletCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex,
			bool bFromSweep,
			const FHitResult& SweepResult);
	UFUNCTION()
	void BulletCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

	UFUNCTION()
	virtual void ImpactProjectile();

	UFUNCTION(NetMulticast, Unreliable)
	void InitVisualMeshProjectile_Multicast(UStaticMesh* newMesh, FTransform MeshRelative);

	UFUNCTION(NetMulticast, Unreliable)
	void InitVisualTrailProjectile_Multicast(UParticleSystem* NewTemplate, FTransform TemplateRelative);

	UFUNCTION(NetMulticast, Unreliable)
	void SpawnHitDecal_Multicast(UMaterialInterface* DecalMaterial, UPrimitiveComponent* OtherComp, FHitResult HitResult);

	UFUNCTION(NetMulticast, Unreliable)
	void SpawnHitFX_Multicast(UParticleSystem* FxTemplate, FHitResult HitResult);

	UFUNCTION(NetMulticast, Unreliable)
	void SpawnHitSound_Multicast(USoundBase* HitSound, FHitResult HitResult);

	UFUNCTION(NetMulticast, Reliable)
	void InitVelocity_Multicast(float InitSpeed, float MaxSpeed);

	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;
};
