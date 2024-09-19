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
	/*Health*/ Souls = NewHealth;
}

float UVSHealthComponent::GetCurrentHealth() const
{
	return /*Health*/Souls;
}

bool UVSHealthComponent::GetIsAlive() const
{
	return bIsAlive;
}

void UVSHealthComponent::ChangeHealthValue_OnServer_Implementation(float ChangeValue, AController* DamageInstigator)
{
	if (GetOwner()->HasAuthority())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Server is changing health: %f"), ChangeValue);
		if (bIsAlive)
		{
			ChangeValue *= CoefDamage;
			/*Health*/Souls += ChangeValue;
			//	OnHealthChange.Broadcast(Health, ChangeValue);
			OnHealthChangeEvent_Multicast(/*Health*/ Souls, ChangeValue);
			if (/*Health*/Souls > /*1.0f*/MaxSouls)
			{
				/*Health*/Souls = /*1.0f*/MaxSouls;
			}
			else
			{
				if (/*Health*/Souls < 0.0f)
				{
					bIsAlive = false;
					UE_LOG(LogTemp, Error, TEXT("The boolean value is %s"), (bIsAlive ? TEXT("true") : TEXT("false")));
					//OnDead.Broadcast();
					DeadEvent_Multicast(DamageInstigator);
					/*Health*/Souls = 0.0f;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Client attempted to change health: %f"), ChangeValue);
	}
}

void UVSHealthComponent::OnHealthChangeEvent_Multicast_Implementation(float newHealth, float value)
{
	OnHealthChange.Broadcast(newHealth, value);
}

void UVSHealthComponent::DeadEvent_Multicast_Implementation(AController* DamageInstigator)
{
	OnDead.Broadcast(DamageInstigator);
}

void UVSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVSHealthComponent, /*Health*/Souls);
	DOREPLIFETIME(UVSHealthComponent, bIsAlive);
}

