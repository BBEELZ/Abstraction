// Fill out your copyright notice in the Description page of Project Settings.


#include "DoorInteractionrComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ObjectiveComponent.h"
#include "InteractionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/TextRenderComponent.h"
#include "AbstractionPlayerCharacter.h"
#include "DrawDebugHelpers.h"

constexpr float FLT_METERS(float meters) { return meters * 100.0f; }

static TAutoConsoleVariable<bool> CVarToggleDebugDoor(
	TEXT("Abstraction.DoorInteractionComponent.Debug"),
	false,
	TEXT("Toggle DoorInteractionComponent debug display."),
	ECVF_Default);

// Sets default values for this component's properties
UDoorInteractionrComponent::UDoorInteractionrComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	DoorState = EDoorState::DS_Closed;

	CVarToggleDebugDoor.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&UDoorInteractionrComponent::OnDebugToggled));

	// ...
}

void UDoorInteractionrComponent::InteractionStart()
{
	Super::InteractionStart();
	if (InteractingActor)
	{
		OpenDoor();
	}
}


// Called when the game starts
void UDoorInteractionrComponent::BeginPlay()
{
	Super::BeginPlay();
	StartRotation = GetOwner()->GetActorRotation();
	FinalRotation = GetOwner()->GetActorRotation() + DesiredRotation;
	// ensure TimeToRotate is greater than EPSILON
	CurrentRotationTime = 0.0f;

	AudioComponent = GetOwner()->FindComponentByClass<UAudioComponent>();
	//check(AudioComponent);
	if (!AudioComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDoorInteractionComponent::BeginPlay() Missing Audio Component"));
	}

	TextRenderComponent = GetOwner()->FindComponentByClass<UTextRenderComponent>();
}

void UDoorInteractionrComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("UDoorInteractionComponent::OnOverlapBegin"));
	//we already have somebody interacting, currently we don't support multiple interactions
	if (InteractingActor || !bActive)
	{
		return;
	}

	//for now we will get that component and set visible
	if (OtherActor->ActorHasTag("Player"))
	{
		InteractingActor = OtherActor;
		if (TextRenderComponent)
		{
			TextRenderComponent->SetText(InteractionPrompt);
			TextRenderComponent->SetVisibility(true);
		}
	}
}

void UDoorInteractionrComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("UDoorInteractionComponent::OnOverlapEnd"));
	if (OtherActor == InteractingActor)
	{
		InteractingActor = nullptr;
		TextRenderComponent->SetVisibility(false);
	}
}

void UDoorInteractionrComponent::InteractionRequested()
{
	//ideally we would make sure this is allowed
	if (InteractingActor)
	{
		bActive = false;
		if (TextRenderComponent)
		{
			TextRenderComponent->SetText(InteractionPrompt);
			TextRenderComponent->SetVisibility(false);
		}

		AAbstractionPlayerCharacter* APC = Cast<AAbstractionPlayerCharacter>(InteractingActor);
		if (APC)
		{
			APC->DoorOpenInteractionStarted(GetOwner());
		}

		//this will be called from the owner to be in sync with animation
		//OpenDoor();
	}
}

void UDoorInteractionrComponent::OpenDoor()
{
	if (IsOpen() || DoorState == EDoorState::DS_Opening)
	{
		return;
	}

	if (AudioComponent)
	{
		AudioComponent->Play();
	}

	DoorState = EDoorState::DS_Opening;
	CurrentRotationTime = 0.0f;
}


// Called every frame
void UDoorInteractionrComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//if (CurrentRotationTime < TimeToRotate)
	if (DoorState == EDoorState::DS_Closed)
	{
		if (TriggerBox && GetWorld() && GetWorld()->GetFirstLocalPlayerFromController())
		{
			APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
			if (PlayerPawn && TriggerBox->IsOverlappingActor(PlayerPawn))
			{
				DoorState = EDoorState::DS_Opening;
				CurrentRotationTime = 0.0f;
			}
		}
	}

	else if (DoorState == EDoorState::DS_Opening)
	{
		CurrentRotationTime += DeltaTime;
		const float TimeRatio = FMath::Clamp(CurrentRotationTime / TimeToRotate, 0.0f, 1.0f);
		const float RotationAlpha = OpenCurve.GetRichCurveConst()->Eval(TimeRatio);
		const FRotator CurrentRotation = FMath::Lerp(StartRotation, FinalRotation, RotationAlpha);
		GetOwner()->SetActorRotation(CurrentRotation);

		if (TimeRatio >= 1.0f)
		{
			OnDoorOpen();
			//DoorState = EDoorState::DS_Open;
			//GEngine->AddOnScreenDebugMessage(-1. 3.0f, FColor::Yellow, TEXT("DoorOpened"));
			//OpenedEvent.Broadcast();
		}
	}

	DebugDraw();
}

void UDoorInteractionrComponent::OnDoorOpen()
{
	DoorState = EDoorState::DS_Open;
	UObjectiveComponent* ObjectiveComponent = GetOwner()->FindComponentByClass<UObjectiveComponent>();
	if (ObjectiveComponent)
	{
		ObjectiveComponent->SetState(EObjectiveState::OS_Completed);
	}
	//GEngine->AddOnScreenDebugMessage(-1. 3.0f, FColor::Yellow, TEXT("DoorOpened"));

	//tell any listeners that the interaction is successful
	InteractionSuccess.Broadcast();
}

void UDoorInteractionrComponent::OnDebugToggled(IConsoleVariable* Var)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDebugToggled"));
}

void UDoorInteractionrComponent::DebugDraw()
{
	if (CVarToggleDebugDoor->GetBool())
	{
		FVector Offset(FLT_METERS(-0.75f), 0.0f, FLT_METERS(2.5f));
		FVector StartLocation = GetOwner()->GetActorLocation() + Offset;
		FString EnumAsString = TEXT("Door State: ") + UEnum::GetDisplayValueAsText(DoorState).ToString();
		DrawDebugString(GetWorld(), Offset, EnumAsString, GetOwner(), FColor::Blue, 0.0f);
	}
}

