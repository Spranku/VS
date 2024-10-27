// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUpActor.h"
#include "/Projects/VS/Source/VS/VSCharacter.h"
#include "BaseWeapon.h"
#include <Kismet/GameplayStatics.h> 
#include "Components/SphereComponent.h"

// Sets default values
APickUpActor::APickUpActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetupAttachment(RootComponent);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
}

// Called when the game starts or when spawned
void APickUpActor::BeginPlay()
{
	Super::BeginPlay();

	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &APickUpActor::SphereCollisionBeginOverlap);
}

// Called every frame
void APickUpActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickUpActor::PickUpSuccess()
{
	if (EquipSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), EquipSound);
	}
	Destroy();
}

void APickUpActor::SphereCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
	AVSCharacter* Character = Cast<AVSCharacter>(OtherActor);
	if (Character && Character->GetIsAlive() && Character->GetCurrentWeapon())
	{
		if (WeaponTypeAmmo == Character->GetCurrentWeapon()->GetWeaponType())
		{
			Character->GetCurrentWeapon()->ChangeAmmoCountInBackpack(AmmoCout);
			PickUpSuccess();
		}
		else
		{
			switch (WeaponTypeAmmo)
			{
			case EWeaponType::PrimaryWeapon:
				AddAmmoToBackpack(Character->Weapons);
				break;
			case EWeaponType::SecondaryWeapon:
				AddAmmoToBackpack(Character->Weapons);
				break;
			default:
				break;
			}
		}
	}
}

void APickUpActor::AddAmmoToBackpack(TArray<ABaseWeapon*> Weapons)
{
	for (int8 i = 0; i < Weapons.Num(); i++)
	{
		if (Weapons[i]->GetWeaponType() == WeaponTypeAmmo)
		{
			Weapons[i]->ChangeAmmoCountInBackpack(AmmoCout);
		}
	}
	PickUpSuccess();
}



