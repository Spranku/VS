// Fill out your copyright notice in the Description page of Project Settings.


#include "VSCharacterHealthComponent.h"

void UVSCharacterHealthComponent::ChangeHealthValue_OnServer(float ChangeValue, AController* DamageInstigator)
{
	float CurrentDamage = ChangeValue * CoefDamage;

	if (Shield > 0.0f && ChangeValue < 0.0f)
	{
		ChangeShieldValue(ChangeValue);
		if (Shield < 0.0f)
		{
			// FX
			UE_LOG(LogTemp, Warning, TEXT("UTPSCharacterHealthComponent::ChangeHealthValue - Shield < 0"));
		}
	}
	else
	{
		Super::ChangeHealthValue_OnServer(ChangeValue, DamageInstigator);
	}
}

float UVSCharacterHealthComponent::GetCurrentShield()
{
	return Shield;
}

void UVSCharacterHealthComponent::ChangeShieldValue(float ChangeValue)
{
	Shield += ChangeValue;
	ShieldChangeEvent_Multicast(Shield, ChangeValue);
	
	if (Shield > 100.0f)
	{
		Shield = 100.0f;
	}
	else 
	{
		if (Shield < 0.0f)
		{
			Shield = 0.0f;
		}
	}

	/* Get World = nullptr. Why: ? */
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_CoolDownShieldTimer,
			this,
			&UVSCharacterHealthComponent::CoolDownShieldEnd,
			CoolDownShieldRecoveryTime,
			false);

		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ShieldRecoveryRateTimer);
		UE_LOG(LogTemp, Warning, TEXT("UVSCharacterHealthComponent::ChangeShieldValue - Have GetWorld()"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UVSCharacterHealthComponent::ChangeShieldValue - GetWorld()= NULL"));
	}
}

void UVSCharacterHealthComponent::CoolDownShieldEnd()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ShieldRecoveryRateTimer,
											   this,
											   &UVSCharacterHealthComponent::RecovryShield,
											   ShieldRecoveryRate,
											   true);
	}
}

void UVSCharacterHealthComponent::RecovryShield()
{
	float tmp = Shield;
	tmp = tmp + ShieldRecoveryValue;
	if (tmp > 100.0f)
	{
		Shield = 100.0f;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ShieldRecoveryRateTimer);
		}
	}
	else
	{
		Shield = tmp;
	}
	ShieldChangeEvent_Multicast(Shield, ShieldRecoveryValue);
	//OnShieldChange.Broadcast(Shield, ShieldRecoveryValue); 
}

float UVSCharacterHealthComponent::GetShieldValue()
{
	return Shield;
}

void UVSCharacterHealthComponent::ShieldChangeEvent_Multicast_Implementation(float newShield, float Damage)
{
	OnShieldChange.Broadcast(newShield, Damage);
}
