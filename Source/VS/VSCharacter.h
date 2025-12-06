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

	UPROPERTY(BlueprintAssignable/*, EditAnywhere, BlueprintReadWrite*/)
	FOnSwitchWeapon OnSwitchWeapon;

	UPROPERTY(BlueprintAssignable/*, EditAnywhere, BlueprintReadWrite*/)
	FOnAmmoTypeChange OnAmmoTypeChange;

	UPROPERTY(BlueprintAssignable/*, EditAnywhere, BlueprintReadWrite*/)
	FOnAmmoChange OnAmmoChange;

	int16 CoutJumps = 0;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	AVSCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY()
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY()
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero")
	EHeroType HeroType = EHeroType::Hunk;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = "State")
	TArray<class ABaseWeapon*> Weapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "State")
	int32 CurrentIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | Thid Person")
	UAnimMontage* ThirdPersonReload = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | Thid Person")
	UAnimMontage* ThirdPersonFireIronsight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | Thid Person")
	UAnimMontage* ThirdPersonFireRelax = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | Thid Person")
	UAnimMontage* ThirdPersonEquipAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | First Person")
	UAnimMontage* FirstPersonEquipAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | First Person")
	UAnimMontage* FirstPersonFireIronsight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | First Person")
	UAnimMontage* FirstPersonFireRelax = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations | First Person")
	UAnimMontage* FirstPersonReload = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	FVector GetForwardVectorFromCamera();

	FVector GetLocationFromCamera();

	UFUNCTION()
	void TryReloadWeapon();

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
	void StartWeaponReloadAnimation();

	UFUNCTION()
	void StartWeaponThirdPersonFireAnimation();

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

	UFUNCTION(NetMulticast, Unreliable)
	void ChangingWeapon(int32 Index);

	UFUNCTION(NetMulticast, Unreliable)
	void StopFireMontage_Multicast();

	UFUNCTION(NetMulticast, Unreliable)
	void PlayDeadMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

	UFUNCTION(NetMulticast, Unreliable)
	void PlayWeaponFireMontage_Multicast(UAnimMontage* ThirdPersonAnim/*, UAnimMontage* FirstPersonAnim*/);

	UFUNCTION(NetMulticast, Unreliable)
	void PlayWeaponEquipMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

	UFUNCTION(NetMulticast, Unreliable)
	void PlayWeaponReloadMontage_Multicast(UAnimMontage* ThirdPersonAnim, UAnimMontage* FirstPersonAnim);

protected:
	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly,BlueprintReadOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	class UVSCharacterHealthComponent* CharacterHealthComponent;

protected:
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
	class ABaseWeapon* CurrentWeapon;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsFire = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsCrouch = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsReload = false;

	UPROPERTY(BlueprintReadOnly,Replicated)
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsJumping = false;

	UPROPERTY(BlueprintReadWrite, Replicated)
	float Direction;

	UPROPERTY(BlueprintReadWrite, Replicated)
	float AimPitch;

	UPROPERTY(BlueprintReadWrite, Replicated)
	float Pitch_OnRep;

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

	void LookUpAtRate(float Rate);

	void InitAimTimeline(float From, float To);

	virtual void SetCurrentWeapon_OnServer_Implementation(class ABaseWeapon* NewWeapon);
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void Jump() override;

	virtual void StopJumping() override;

	EMovementState GetMovementState() const;

	virtual float TakeDamage(float DamageAmount,
							 struct FDamageEvent const& DamageEvent,
							 class AController* EventInstigator,
							 AActor* DamageCauser) override;

	UFUNCTION()
	void ChangeFoV(float In, float Out);

	UFUNCTION()
	virtual void OnRep_CurrentWeapon(const class ABaseWeapon* OldWeapon);

	UFUNCTION(BlueprintCallable)
	void TurnAtRate(float Rate);

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

};

