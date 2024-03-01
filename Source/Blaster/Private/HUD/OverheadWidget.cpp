// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();
}

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (!InPawn) return;
	
	FString Role = FString();

	const ENetRole RemoteRole = InPawn->GetRemoteRole();
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = TEXT("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = TEXT("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = TEXT("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = TEXT("None");
		break;
	default:
		break;
	}

	const FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	SetDisplayText(RemoteRoleString);
}
