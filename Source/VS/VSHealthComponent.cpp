// Fill out your copyright notice in the Description page of Project Settings.


#include "VSHealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UVSHealthComponent::UVSHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UVSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UVSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVSHealthComponent::SetCurrentHealth(float NewHealth)
{
	Health = NewHealth;
}

float UVSHealthComponent::GetCurrentHealth()
{
	return Health;
}

bool UVSHealthComponent::GetIsAlive()
{
	return bIsAlive;
}

void UVSHealthComponent::ChangeHealthValue_OnServer_Implementation(float ChangeValue)
{
	if (bIsAlive)
	{
		ChangeValue = ChangeValue * CoefDamage;
		Health += ChangeValue;
		//OnHealthChange.Broadcast(Health, ChangeValue);
		OnHealthChangeEvent_Multicast(Health, ChangeValue);

		if (Health > 100.0f)
		{
			Health = 100.0f;
		}
		else
		{
			if (Health < 0.0f)
			{
				bIsAlive = false;
				//OnDead.Broadcast();
				DeadEvent_Multicast();
			}
		}
	}
}

void UVSHealthComponent::OnHealthChangeEvent_Multicast_Implementation(float newHealth, float value)
{
	OnHealthChange.Broadcast(newHealth, value);
}

void UVSHealthComponent::DeadEvent_Multicast_Implementation()
{
	OnDead.Broadcast();
}

void UVSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	///DOREPLIFETIME(UVSHealthComponent, Health);
	DOREPLIFETIME(UVSHealthComponent, bIsAlive);
}

