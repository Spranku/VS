// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VSHealthComponent.h"
#include "VSCharacterHealthComponent.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChange, float, Shield, float, Damage);

UCLASS()
class VS_API UVSCharacterHealthComponent : public UVSHealthComponent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite, Category = "Health")
	FOnShieldChange OnShieldChange;
	
	FTimerHandle TimerHandle_CoolDownShieldTimer;

	FTimerHandle TimerHandle_ShieldRecoveryRateTimer;

protected:

	float Shield = 100.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float CoolDownShieldRecoveryTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float ShieldRecoveryValue = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float ShieldRecoveryRate = 0.1f;

	void ChangeHealthValue_OnServer(float ChangeValue, AController* DamageInstigator) override;

	float GetCurrentShield();

	void ChangeShieldValue(float ChangeValue);

	void CoolDownShieldEnd();

	void RecovryShield();

	UFUNCTION(BlueprintCallable)
	float GetShieldValue();

	UFUNCTION(NetMulticast, Reliable)
	void ShieldChangeEvent_Multicast(float newShield, float Damage);
};
