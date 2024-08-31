// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VSHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChange, float, Health, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDead,AController*,DamageInstigator);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VS_API UVSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVSHealthComponent();

	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite, Category = "Health")
	FOnHealthChange OnHealthChange;

	UPROPERTY(BlueprintAssignable, EditAnywhere, BlueprintReadWrite, Category = "Health")
	FOnDead OnDead;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	float Health = 1.0f; /// TODO: Refactoring to soul

	UPROPERTY(Replicated)
	bool bIsAlive = true;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Health")
	float CoefDamage = 1.0f;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool GetIsAlive() const;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Health")
	virtual void ChangeHealthValue_OnServer(float ChangeValue, AController* DamageInstigator);

	UFUNCTION(NetMulticast, Reliable)
	void OnHealthChangeEvent_Multicast(float newHealth, float value);

	UFUNCTION(NetMulticast, Reliable)
	void DeadEvent_Multicast(AController* DamageInstigator);

};
