// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableDoor.h"
#include "DoorInteractionrComponent.h"
#include "Components/CapsuleComponent.h"

AInteractableDoor::AInteractableDoor()
{
	DoorInteractionrComponent = CreateDefaultSubobject<UDoorInteractionrComponent>(TEXT("DoorInteractionrComponent"));
	if (DoorInteractionrComponent->GetTriggerCapsule())
	{
		DoorInteractionrComponent->GetTriggerCapsule()->SetupAttachment(RootComponent);
	}
}

void AInteractableDoor::BeginPlay()
{
	Super::BeginPlay();
	DoorInteractionrComponent->InteractionSuccess.AddDynamic(this, &AInteractableDoor::OnInteractionSuccess);
}

void AInteractableDoor::OnInteractionSuccess()
{
	OnDoorOpen.Broadcast();
}

