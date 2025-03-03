// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Actors/BaseWeapon.h"
#include "FuncLibrary/Types.h"
#include "Actors/BaseWeapon.h"
#include "VSCharacterHealthComponent.h"
#include "VSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSwitchWeapon, EWeaponType, WeaponType, FAdditionalWeaponInfo, WeaponAdditionalInfo, ABaseWeapon*,CurrentWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoTypeChange, EWeaponType, WeaponType, int32, NewRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChange, int32 , NewAmmo);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAdditionalInfoChange, FAdditionalWeaponInfo, AdditionalInfo);

UENUM(BlueprintType)
enum class EHeroType : uint8
{
	Hunk UMETA(DisplayName = "Hunk"),
	Swat UMETA(DisplayName = "Swat"),
	Observer UMETA(DisplayName = "Observer")
};

UCLASS(config=Game)
class AVSCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly,BlueprintReadOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite)
	FOnSwitchWeapon OnSwitchWeapon;

	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite)
	FOnAmmoTypeChange OnAmmoTypeChange;

	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite)
	FOnAmmoChange OnAmmoChange;
protected:
	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	class UVSCharacterHealthComponent* CharacterHealthComponent;

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

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FirstPersonEquipWeaponAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero")
	EHeroType HeroType = EHeroType::Hunk;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint8 bUsingMotionControllers : 1;

	UPROPERTY(Replicated)
	EMovementState MovementState = EMovementState::Run_State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FCharacterSpeed MovementInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<UAnimMontage*> DeadsAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TArray<USoundBase*> ImpactSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Class")
	TArray<TSubclassOf<class ABaseWeapon>> DefaultWeapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite,Replicated, Category = "State")
	TArray<class ABaseWeapon*> Weapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite,Replicated, Category = "State")
	int32 CurrentIndex = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
	class ABaseWeapon* CurrentWeapon;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsFire = false;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Replicated)
	bool bIsCrouch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsReload = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Replicated)
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsJumping = false;

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

	FTimerHandle InitWeaponTimerHandle;

	FTimerHandle EquipTimerHandle;

	FTimerDelegate EquipTimerDelegate;

	FTimerHandle AimTimerHandle;

	FTimerDelegate AimTimerDelegate;

	FTimerHandle RagDollTimerHandle;

	int BackpackAmmo = 1;

	float Alpha = 0.0f;

	bool bCanAiming = true;
protected:

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

	EMovementState GetMovementState() const;

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

	UFUNCTION(Client,Unreliable)
	void StopAiming_OnClient();

	UFUNCTION(Server, UnReliable)
	void InitAiming_OnServer();

	UFUNCTION(Server, UnReliable)
	void StopAiming_OnServer();

	UFUNCTION(Server, UnReliable)
	void InitCrouch_OnServer();

	UFUNCTION(Server, UnReliable)
	void StopCrouch_OnServer();

	UFUNCTION(Server,Reliable)
	void EquipWeapon_OnServer(const int32 Index);

	UFUNCTION(Server, UnReliable)
	void SetMovementState_OnServer(EMovementState NewState);

	UFUNCTION(Server, Reliable)
	void SetCurrentWeapon_OnServer(class ABaseWeapon* NewWeapon);

	UFUNCTION(Client,UnReliable)
	void BlockActionDuringEquip_OnClient();

	UFUNCTION(NetMulticast, Unreliable)
	void PitchMulticast(float PitchRep);

	UFUNCTION(NetMulticast, UnReliable)
	void SetMovementState_Multicast(EMovementState NewState);

	virtual void SetCurrentWeapon_OnServer_Implementation(class ABaseWeapon* NewWeapon);
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void Jump() override;

	virtual void StopJumping() override;

public:
	
	virtual float TakeDamage(float DamageAmount,
							 struct FDamageEvent const& DamageEvent,
							 class AController* EventInstigator,
							 AActor* DamageCauser) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	FVector GetForwardVectorFromCamera();

	FVector GetLocationFromCamera();

	UFUNCTION(BlueprintCallable)
	EHeroType GetHeroType() const;

	UFUNCTION(BlueprintCallable)
	ABaseWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable)
	void CharDead(AController* DamageInstigator);

	UFUNCTION(BlueprintNativeEvent)
	void CharDead_BP(AController* DamageInstigator);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsAlive();

	UFUNCTION() 
	void InitWeapon();

	UFUNCTION()
	void WeaponReloadEnd();

	UFUNCTION() 
	void StartWeaponReloadAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P);

	UFUNCTION() 
	void StartWeaponFireAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P);

	UFUNCTION()
	void StartWeaponEquipAnimation(UAnimMontage* Anim3P, UAnimMontage* Anim1P);

	UFUNCTION(Server, Unreliable) 
	void S_LookUPSync(FRotator RotationSync);

	UFUNCTION(NetMulticast, Unreliable) 
	void M_LookUPSync(FRotator RotationSync);

	UFUNCTION(NetMulticast, Reliable)
	void ChangeAmmoByShotEvent_Multicast();

	UFUNCTION(NetMulticast, Reliable)
	void EnableRagdoll_Multicast();

	UFUNCTION(NetMulticast,Unreliable)
	void ChangingWeapon(int32 Index);

	UFUNCTION(NetMulticast, Unreliable)
	void PlayDeadMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

	UFUNCTION(NetMulticast,Unreliable)
	void PlayWeaponFireMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

	UFUNCTION(NetMulticast, Unreliable)
	void PlayWeaponEquipMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);
	 
	UFUNCTION(NetMulticast, Unreliable)
	void PlayWeaponReloadMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);
};

