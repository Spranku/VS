// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoringTArray.generated.h"

/**
 * 
 */
UCLASS()
class VS_API USoringTArray : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "MuFunc")
	static TArray<int> SortArrayInt(TArray<int> ArrayToSort, bool bIsIncreasing);
};
