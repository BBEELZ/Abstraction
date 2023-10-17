// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableDoor.h"
#include "DoorInteractionrComponent.h"

AInteractableDoor::AInteractableDoor()
{
	DoorInteractionrComponent = CreateDefaultSubobject<UDoorInteractionrComponent>(TEXT("DoorInteractionrComponent"));
}
