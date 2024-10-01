// Fill out your copyright notice in the Description page of Project Settings.


#include "SoringTArray.h"

TArray<int> USoringTArray::SortArrayInt(TArray<int> ArrayToSort, bool bIsIncreasing)
{
	if (bIsIncreasing) ArrayToSort.Sort([](const int& A, const int& B)
	{
			return A < B;
	});
	else ArrayToSort.Sort([](const int& A, const int& B)
	{
			return A > B;
	});

	return ArrayToSort;
}
