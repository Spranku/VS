// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "/Projects/VS/Source/VS/FuncLibrary/Types.h"
#include "PickUpActor.generated.h"

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item settings")
	EWeaponType WeaponTypeAmmo;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Item settings")
	int AmmoCout = 1;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void PickUpSuccess();
};
