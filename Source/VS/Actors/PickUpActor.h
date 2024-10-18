// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "/Projects/VS/Source/VS/FuncLibrary/Types.h"
#include "PickUpActor.generated.h"

class ABaseWeapon;
class USoundBase;

UCLASS()
class VS_API APickUpActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickUpActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	class USceneComponent* SceneComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	class UStaticMeshComponent* StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	class USphereComponent* SphereCollision = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	class USoundBase* EquipSound = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item settings", meta = (ExposeOnSpawn = true))
	EWeaponType WeaponTypeAmmo;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Item settings")
	int AmmoCout = 1;
	
	void AddAmmoToBackpack(TArray<ABaseWeapon*> Weapons);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void PickUpSuccess();

	UFUNCTION()
	void SphereCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent,
									 AActor* OtherActor,
									 UPrimitiveComponent* OtherComp,
									 int32 OtherBodyIndex,
									 bool bFromSweep,
									 const FHitResult& SweepResult);

};
