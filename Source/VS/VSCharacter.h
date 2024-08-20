// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Actors/BaseWeapon.h"
#include "FuncLibrary/Types.h"
#include "Actors/BaseWeapon.h"
#include "VSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AVSCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly,BlueprintReadOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;
protected:
	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

public:
	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	AVSCharacter();

protected:
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime);

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AVSProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** Sound to play each time we reloading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* ReloadSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FirstPersonFireAnimation;

	/** AnimMontage to play each time we reloading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FirstPersonReloadAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<UAnimMontage*> DeadsAnim;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint8 bUsingMotionControllers : 1;

	UPROPERTY(Replicated)
	EMovementState MovementState = EMovementState::Run_State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FCharacterSpeed MovementInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Class")
	TArray<TSubclassOf<class ABaseWeapon>> DefaultWeapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite,Replicated, Category = "State")
	TArray<class ABaseWeapon*> Weapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite,Replicated, Category = "State")
	int32 CurrentIndex = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
	class ABaseWeapon* CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsFire = false;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	bool bIsCrouch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsReload = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Replicated)
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, Category = "Animation")
	float Direction;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, Category = "Animation")
	float AimPitch;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, Category = "Animation")
	float Pitch_OnRep;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, Category = "Animation")
	float AimYaw;

	FRotator CamForwardVector;

	FRotator ControlRotationSynchronized;

protected:

	FTimerHandle TimerHandle;

	FTimerHandle AimTimerHandle;

	FTimerDelegate AimTimerDelegate;

	float Alpha = 0.0f;

	void OnFire();

	void EndFire();

	void InitCrouch();

	void StopCrouch();

	void InitReload();
	
	void InitAiming();

	void StopAiming();

	void NextWeapon();

	void LastWeapon();

	void ChangeMovementState();

	void CharacterUpdate();

	void FireEvent(bool bIsFiring);

	void MovementTick(float DeltaTime);

	void MoveForward(float Val);

	void MoveRight(float Val);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);

	void InitAimTimeline(float From, float To);

	EMovementState GetMovementState();

	UFUNCTION()
	void ChangeFoV(float In, float Out);

	UFUNCTION()
	virtual void OnRep_CurrentWeapon(const class ABaseWeapon* OldWeapon);

	UFUNCTION()
	void TryReloadWeapon();

	UFUNCTION(Server,Unreliable)
	void TryReloadWeapon_OnServer();

	UFUNCTION(Server, Unreliable)
	void PitchOnServer(float PitchRep);

	UFUNCTION(Server, UnReliable)
	void InitAiming_OnServer();

	UFUNCTION(Server, UnReliable)
	void StopAiming_OnServer();

	UFUNCTION(Server,Reliable)
	void EquipWeapon_OnServer(const int32 Index);

	UFUNCTION(Server, Reliable)
	void SetMovementState_OnServer(EMovementState NewState);

	UFUNCTION(Server, Reliable)
	void SetCurrentWeapon_OnServer(class ABaseWeapon* NewWeapon);

	UFUNCTION(NetMulticast, Unreliable)
	void PitchMulticast(float PitchRep);

	UFUNCTION(NetMulticast, Reliable)
	void SetMovementState_Multicast(EMovementState NewState);

	virtual void SetCurrentWeapon_OnServer_Implementation(class ABaseWeapon* NewWeapon);
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	///void FireRecoil();

	FVector GetForwardVectorFromCamera();

	FVector GetLocationFromCamera();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	ABaseWeapon* GetCurrentWeapon();

	UFUNCTION()
	void InitWeapon();

	UFUNCTION()
	void WeaponReloadStart(UAnimMontage* Anim3P, UAnimMontage* Anim1P);

	UFUNCTION()
	void WeaponReloadEnd(bool bIsSuccess, int32 AmmoSafe);

	UFUNCTION()
	void WeaponFireStart(UAnimMontage* Anim3P, UAnimMontage* Anim1P);

	UFUNCTION(Server, Reliable)
	void S_LookUPSync(FRotator RotationSync);

	UFUNCTION(NetMulticast,Unreliable)
	void WeaponFireStart_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);
	 
	UFUNCTION(NetMulticast, Unreliable)
	void PlayReloadMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

	UFUNCTION(NetMulticast, Reliable)
	void M_LookUPSync(FRotator RotationSync);
};

