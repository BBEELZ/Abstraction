// Copyright Epic Games, Inc. All Rights Reserved.


#include "AbstractionGameModeBase.h"
#include "ObjectiveWorldSubsystem.h"
#include "ObjectiveHUD.h"

void AAbstractionGameModeBase::StartPlay()
{
	Super::StartPlay();
	UObjectiveWorldSubsystem* ObjectiveWorldSubsystem = GetWorld()->GetSubsystem<UObjectiveWorldSubsystem>();
	if (ObjectiveWorldSubsystem)
	{
		ObjectiveWorldSubsystem->CreateObjectiveWidgets();
		ObjectiveWorldSubsystem->DisplayObjectiveWidget();
	}
}

