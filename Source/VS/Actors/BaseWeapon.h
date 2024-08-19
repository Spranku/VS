// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "/Projects/VS/Source/VS/FuncLibrary/Types.h"
#include "Net/UnrealNetwork.h"
#include "BaseProjectile.h"
#include "BaseWeapon.generated.h"

class AVSCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponReloadStart, UAnimMontage*, Anim3P, UAnimMontage*, Anim1P);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponReloadEnd, bool, bIsSuccess, int32, AmmoSafe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponFireStart, UAnimMontage*, Anim3P, UAnimMontage*, Anim1P);

UCLASS()
class VS_API ABaseWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	FOnWeaponReloadStart OnWeaponReloadStart;
	FOnWeaponReloadEnd OnWeaponReloadEnd;
	FOnWeaponFireStart OnWeaponFireStart;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta = (AllowPrivateAcess = "true"),Category = Components)
	class USceneComponent* SceneComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class USkeletalMeshComponent* SkeletalMeshWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class UStaticMeshComponent* StaticMeshWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Components)
	class UStaticMeshComponent* LenseMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class UArrowComponent* ShootLocation = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAcess = "true"), Category = Components)
	class UArrowComponent* SleeveLocation = nullptr;

	UPROPERTY(VisibleInstanceOnly,BlueprintReadWrite, Category = "State")
	class AVSCharacter* CurrentOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScopeMaterial")
	UMaterialInstance* CustomLenseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScopeMaterial")
	UMaterialInstance* DefaultLenseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScopeMaterial")
	class UTextureRenderTarget2D* TextureTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScopeMaterial")
	class USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire logic")
	bool ShowDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire logic")
	bool WeaponFiring = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Fire logic")
	bool WeaponReloading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire logic")
	FWeaponInfo WeaponSetting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Info")
	FAdditionalWeaponInfo WeaponInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReloadLogic")
	float ReloadTimer = 0.0f;

	UPROPERTY(Replicated)
	FVector ShootEndLocation = FVector(0);

	UPROPERTY(Replicated)
	bool WeaponAiming = false;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "State")
	bool bIsRailGun = false;

	AVSCharacter* Character = nullptr;

	float FireTime = 0.0f;
		
	float DropClipTimer = -1.0f;

	float DropShellTimer = -1.0f;

	bool BlockFire = false;

	bool DropClipFlag = false;

	bool DropShellFlag = false;

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

	void InitReload();

	void FinishReload();

	void CancelReload();

	void InitAiming();

	UFUNCTION(Client, Unreliable)
	void CancelAiming();

	void ChangeDispersionByShoot();

	float GetCurrentDispersion() const;

	bool CheckWeaponCanFire();

	bool CheckCanWeaponReload();

	FVector GetFireEndLocation() const;

	FVector ApplyDispersionToShoot(FVector DirectionShoot) const;

	UFUNCTION(BlueprintCallable)
	int32 GetWeaponRound() const;

	UFUNCTION(BlueprintCallable)
	EWeaponType GetWeaponType() const;

	UFUNCTION()
	FProjectileInfo GetProjectile();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SetWeaponStateFire_OnServer(bool bIsFire);

	UFUNCTION(Server, Reliable)
	void Fire(FTransform ShootTo);

	UFUNCTION(Server, Unreliable)
	void UpdateStateWeapon_OnServer(EMovementState NewMovementState);

	UFUNCTION(Server, Unreliable)
	void UpdateWeaponByCharacterMovementStateOnServer(FVector NewShootEndLocation, bool NewShouldReduceDispersion);

	UFUNCTION(Client, Unreliable)
	void SetMaterialLense_OnClient();

	UFUNCTION(NetMulticast, Unreliable)
	void AnimWeaponStart_Multicast(UAnimMontage* AnimThirdPerson, UAnimMontage* AnimFirstPerson);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
};
