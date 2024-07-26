// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "BaseWeapon.generated.h"

UCLASS()
class VS_API ABaseWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta = (AllowPrivateAcess = "true"),Category = Components)
	class USceneComponent* SceneComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class USkeletalMeshComponent* SkeletalMeshWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class UStaticMeshComponent* StaticMeshWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class UArrowComponent* ShootLocation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon logic")
	bool WeaponFiring = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon logic")
	bool WeaponReloading = false;

	float FireTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReloadLogic")
		
	bool WeaponAiming = false;

	bool BlockFire = false;

	bool DropClipFlag = false;

	bool DropShellFlag = false;

	float DropClipTimer = -1.0f;

	float DropShellTimer = -1.0f;

	float ReloadTimer = 0.0f;

	// Ðàçáðîñ
	//UPROPERTY(Replicated)
	bool ShouldReduseDispersion = false;
	float CurrentDispersion = 0.0f;
	float CurrentDispersionMax = 1.0f;
	float CurrentDispersionMin = 0.1f;
	float CurrentDispersionRecoil = 0.1f;
	float CurrentDispersionReduction = 0.1f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ReloadTick(float DeltaTime);

	void FireTick(float DeltaTime);

	void DispersionTick(float DeltaTime);

	void WeaponInit();

	void Fire();

	void InitReload();

	void FinishReload();

	void CancelReload();

	bool CheckWeaponCanFire();

	bool CheckCanWeaponReload();

	UFUNCTION(BlueprintCallable)
	int32 GetWeaponRound();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SetWeaponStateFire_OnServer(bool bIsFire);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
};
