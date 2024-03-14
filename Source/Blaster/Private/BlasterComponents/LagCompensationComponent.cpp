// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

ULagCompensationComponent::ULagCompensationComponent()
	: MaxRecordTime(4.0)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// �������� �ƿ������� �ϳ��� �����ϰ� �ð� ��� ����
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	// �������� �ִٸ� ������ �ð������� �����
	else
	{
		// �� �հ� �� �� ����� �ð� ���̰� �� ������ִ� �ð�
		double HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		// ������ �ð����� ������ ������ ��� ����
		while (HistoryLength > MaxRecordTime)
		{
			// ������ �������� ������ �׿������Ƿ� ������ ����
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		// �� �������� ��忡 ����
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color) const
{
	for (const auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color, false, 4.f);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime) const
{
	if (!HitCharacter) return FServerSideRewindResult();
	if (!HitCharacter->GetLagCompensation()) return FServerSideRewindResult();
	if (!HitCharacter->GetLagCompensation()->FrameHistory.GetHead()) return FServerSideRewindResult();
	if (!HitCharacter->GetLagCompensation()->FrameHistory.GetTail()) return FServerSideRewindResult();

	// ���� ĳ������ ������ �����丮
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const double OldestHistoryTime = History.GetTail()->GetValue().Time;
	// ���� �ð��� ��ϵ� �ð����� �� �����̶� ���� �ǵ����Ⱑ �Ұ���
	if (OldestHistoryTime > HitTime) return FServerSideRewindResult();
	
	// ������ �ǰ��Ƽ� Ȯ���ؾ��� ������ ��Ű��
	FFramePackage FrameToCheck;
	bool bCheck = false;

	// ���� ������ �Ͱ� ������ �װ� Ȯ��
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bCheck = true;
	}

	if (!bCheck)
	{
		// ���� �ֱ� �Ͱ� ���ų� ��ϵ� �ð����� ���߽ð��� ������ ���� �ֱ� ��ϵ� ���� Ȯ��
		const double NewestHistoryTime = History.GetHead()->GetValue().Time;
		if (NewestHistoryTime <= HitTime)
		{
			FrameToCheck = History.GetHead()->GetValue();
			bCheck = true;
		}

		if (!bCheck)
		{
			TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = History.GetHead();
			TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();

			// �����丮 �� HitTime�� �� �� �ð��� Ȯ��
			while (Older->GetValue().Time > HitTime)
			{
				if (!Older->GetNextNode()) break;
				
				Older = Older->GetNextNode();
				if (Older->GetValue().Time > HitTime)
				{
					Younger = Older;
				}
			}

			if (Older->GetValue().Time == HitTime)
			{
				FrameToCheck = Older->GetValue();
				bCheck = true;
			}

			if (!bCheck)
			{
				// Older�� Younger ������ ������ ����
				FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
				bCheck = true;
			}
		}
	}

	// ������ ��ġ�� ������ ������ ã�Ҵ�.
	if (bCheck)
	{
		// �ش� �����ӿ��� �¾Ҵ��� Ȯ��. ��弦�� Ȯ��
		return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
	}
	return FServerSideRewindResult();
}

void ULagCompensationComponent::Init()
{
	if (!Character)
	{
		Character = Cast<ABlasterCharacter>(GetOwner());
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	if (!GetWorld()) return;

	Init();

	// ĳ������ ��� ��Ʈ�ڽ��� �����ð��� ���缭 ��ġ, ȸ��, ũ�⸦ �����Ѵ�.
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();

		Package.HitBoxInfo.Reserve(Character->GetHitCollisionBoxes().Num());
		for (const auto& BoxPair : Character->GetHitCollisionBoxes())
		{
			if (!BoxPair.Value) continue;

			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, double HitTime) const
{
	const double Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = static_cast<float>(FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0, 1.0));

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	InterpFramePackage.HitBoxInfo.Reserve(OlderFrame.HitBoxInfo.Num());
	for (const auto& OlderPair : OlderFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = OlderPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		// ��ġ�� ȸ������ �� ������ ������ ������ ������ �����Ѵ�.
		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = OlderBox.BoxExtent;
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}
	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation) const
{
	if (!HitCharacter) return FServerSideRewindResult();
	if (!GetWorld()) return FServerSideRewindResult();

	// ���� ĳ������ ���� ��ġ ��Ʈ�ڽ� ������ ����(�ǰ��� �Ŀ� �ٽ� ������� ���� �� ���)
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	
	// ĳ������ ��Ʈ�ڽ��� ��Ű�� ������� ����(�ǰ���)
	MoveBoxes(HitCharacter, Package);

	// �޽��� �浹ó���� ����.
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);
	
	// head���� Ȱ��ȭ���Ѽ� �¾Ҵ��� Ȯ��
	UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[TEXT("head")];
	if (HeadBox)
	{
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// ���߿� �� �κ� �ƿ� ������ �� �����صδ°� ��� Ȯ��
		HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

		FHitResult ConfirmHitResult;
		// �浹üũ�� ���� ���� ���� ��ġ���� ���� �� �ø�
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
		// ��弦 ��Ʈ!
		if (ConfirmHitResult.bBlockingHit)
		{
			// ���� ĳ������ ��Ʈ�ڽ� ������ ������� �ǵ�����.
			ResetHitBoxes(HitCharacter, CurrentFrame);
			// �޽��� �浹ó���� �ٽ� Ų��.
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			// ���� ������ ��弦 ������ true�� ����
			return FServerSideRewindResult{ true, true };
		}
		// ��忡 ���� �ʾ����Ƿ� �ٸ� ������ ��Ʈ�ڽ� Ȯ��
		else
		{
			for (const auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
			{
				if (!BoxPair.Value) continue;

				BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				BoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

				GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
				// �ٸ� ���� ��Ʈ!
				if (ConfirmHitResult.bBlockingHit)
				{
					ResetHitBoxes(HitCharacter, CurrentFrame);
					EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
					// ��弦�� false
					return FServerSideRewindResult{ true, false };
				}
			}
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// ���� �ʾҴ�.
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) const
{
	// ���� ĳ������ ���� ��Ʈ�ڽ� ������ �����´�.
	if (!HitCharacter) return;

	OutFramePackage.HitBoxInfo.Reserve(HitCharacter->GetHitCollisionBoxes().Num());
	for (const auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (!BoxPair.Value) continue;

		FBoxInformation BoxInformation;
		BoxInformation.Location = BoxPair.Value->GetComponentLocation();
		BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
		BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

		OutFramePackage.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const
{
	// ���� ĳ������ ��Ʈ�ڽ��� ������ ������ ���� �����ð���� �ǰ���
	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (!BoxPair.Value) continue;

		BoxPair.Value->SetWorldLocation(Package.HitBoxInfo[BoxPair.Key].Location);
		BoxPair.Value->SetWorldRotation(Package.HitBoxInfo[BoxPair.Key].Rotation);
		BoxPair.Value->SetBoxExtent(Package.HitBoxInfo[BoxPair.Key].BoxExtent);
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const
{
	// ���� ĳ������ ��Ʈ�ڽ��� ������ ������ ���� ������� �ǵ���
	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (!BoxPair.Value) continue;

		BoxPair.Value->SetWorldLocation(Package.HitBoxInfo[BoxPair.Key].Location);
		BoxPair.Value->SetWorldRotation(Package.HitBoxInfo[BoxPair.Key].Rotation);
		BoxPair.Value->SetBoxExtent(Package.HitBoxInfo[BoxPair.Key].BoxExtent);
		// �浹����
		BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled) const
{
	if (!HitCharacter) return;
	if (!HitCharacter->GetMesh()) return;

	HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}
