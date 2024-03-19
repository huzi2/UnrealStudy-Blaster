// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"
#include "Blaster/Blaster.h"
#include "Weapon/Projectile.h"

ULagCompensationComponent::ULagCompensationComponent()
	: MaxRecordTime(4.0)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character) return;
	// ������ ������ ������ �ϸ��
	if (!Character->HasAuthority()) return;

	SaveFramePackage();
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime, AWeapon* DamageCauser) const
{
	if (!HitCharacter) return;
	if (!DamageCauser) return;
	if (!Character) return;

	// ���� �ǰ��� ��û
	const FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	// ���� �ǰ��� ��� Ÿ���� ����
	if (Confirm.bHitConfirmed)
	{
		// Ÿ���� �������� �������� �ش�.
		UGameplayStatics::ApplyDamage(HitCharacter, DamageCauser->GetDamage(), Character->Controller, DamageCauser, UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ServerProjectileScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime, AProjectile* DamageCauser) const
{
	if (!HitCharacter) return;
	if (!DamageCauser) return;
	if (!Character) return;

	// ���� �ǰ��� ��û
	const FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	// ���� �ǰ��� ��� Ÿ���� ����
	if (Confirm.bHitConfirmed)
	{
		// Ÿ���� �������� �������� �ش�.
		UGameplayStatics::ApplyDamage(HitCharacter, DamageCauser->GetDamage(), Character->Controller, DamageCauser, UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ServerShotgunScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime, AWeapon* DamageCauser) const
{
	if (!DamageCauser) return;
	if (!Character) return;

	const FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		if (!HitCharacter) continue;

		float TotalDamage = 0.f;
		// ��弦 ������
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			TotalDamage += Confirm.HeadShots[HitCharacter] * DamageCauser->GetDamage();
		}
		// �ٵ� ������
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			TotalDamage += Confirm.BodyShots[HitCharacter] * DamageCauser->GetDamage();
		}

		UGameplayStatics::ApplyDamage(HitCharacter, TotalDamage, Character->Controller, DamageCauser, UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::Init()
{
	if (!Character)
	{
		Character = Cast<ABlasterCharacter>(GetOwner());
	}
}

void ULagCompensationComponent::SaveFramePackage()
{
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

		// �����Ӻ��� ����Ǵ� ��Ű���� ����׷� ������
		//ShowFramePackage(ThisFrame, FColor::Red);
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

		Package.Character = Character;
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
	// ������ �ǰ��Ƽ� Ȯ���ؾ��� ������ ��Ű��
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	FrameToCheck.Character = HitCharacter;
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, double HitTime) const
{
	if (!HitCharacter) return FFramePackage();
	if (!HitCharacter->GetLagCompensation()) return FFramePackage();
	if (!HitCharacter->GetLagCompensation()->FrameHistory.GetHead()) return FFramePackage();
	if (!HitCharacter->GetLagCompensation()->FrameHistory.GetTail()) return FFramePackage();

	// ���� ĳ������ ������ �����丮
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const double OldestHistoryTime = History.GetTail()->GetValue().Time;
	// ���� �ð��� ��ϵ� �ð����� �� �����̶� ���� �ǵ����Ⱑ �Ұ���
	if (OldestHistoryTime > HitTime) return FFramePackage();

	// ���� ������ �Ͱ� ������ �װ� Ȯ��
	if (OldestHistoryTime == HitTime) return History.GetTail()->GetValue();

	// ���� �ֱ� �Ͱ� ���ų� ��ϵ� �ð����� ���߽ð��� ������ ���� �ֱ� ��ϵ� ���� Ȯ��
	const double NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (NewestHistoryTime <= HitTime) return History.GetHead()->GetValue();

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

	if (Older->GetValue().Time == HitTime) return Older->GetValue();

	// Older�� Younger ������ ������ ����
	return InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
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
	if (UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[TEXT("head")])
	{
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// ���߿� �� �κ� �ƿ� ������ �� �����صδ°� ��� Ȯ��
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

		FHitResult ConfirmHitResult;
		// �浹üũ�� ���� ���� ���� ��ġ���� ���� �� �ø�
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		// ��弦 ��Ʈ!
		if (ConfirmHitResult.bBlockingHit)
		{
			DrawBox(ConfirmHitResult, FColor::Red);

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
				BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

				GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
				// �ٸ� ���� ��Ʈ!
				if (ConfirmHitResult.bBlockingHit)
				{
					DrawBox(ConfirmHitResult, FColor::Blue);

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

void ULagCompensationComponent::DrawBox(const FHitResult& ConfirmHitResult, const FColor& Color) const
{
	if (ConfirmHitResult.Component.IsValid())
	{
		if (UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component))
		{
			DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), Color, false, 8.f);
		}
	}
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime) const
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	FrameToCheck.Character = HitCharacter;
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity) const
{
	if (!HitCharacter) return FServerSideRewindResult();
	if (!GetWorld()) return FServerSideRewindResult();

	// �߻�ü�� ���� �ǰ���� ���� ��Ʈ��ĵ ���� �ǰ���� �ʹݺδ� ������

	// ���� ĳ������ ���� ��ġ ��Ʈ�ڽ� ������ ����(�ǰ��� �Ŀ� �ٽ� ������� ���� �� ���)
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);

	// ĳ������ ��Ʈ�ڽ��� ��Ű�� ������� ����(�ǰ���)
	MoveBoxes(HitCharacter, Package);

	// �޽��� �浹ó���� ����.
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// head���� Ȱ��ȭ���Ѽ� �¾Ҵ��� Ȯ��
	if (UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[TEXT("head")])
	{
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// ���߿� �� �κ� �ƿ� ������ �� �����صδ°� ��� Ȯ��
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	// ���⼭���� �߻�ü�� ����Ʈ���̽��� �ƴ� �߻�ü �������� �浹 ó���� Ȯ��
	FPredictProjectilePathParams PathParams;
	// Ʈ���̽� ä���� ���
	PathParams.bTraceWithChannel = true;
	PathParams.TraceChannel = ECC_HitBox;
	// Ʈ���̽��� �浹ü�� �浹
	PathParams.bTraceWithCollision = true;
	// �߻�ü�� ���ư��� �ð�. �ִ� ��ȭ �ð����� �� �ʿ�� ����
	PathParams.MaxSimTime = MaxRecordTime;
	// �󸶳� �ùķ��̼��� ���ΰ�? 10 ~ 30. 30�� ���� �ڼ��ϰ� �ù�
	PathParams.SimFrequency = 15.f;
	// �߻�ü ���� ��ġ
	PathParams.StartLocation = TraceStart;
	// �߻�ü�� ����� �ӵ�
	PathParams.LaunchVelocity = InitialVelocity;
	// �߻�ü�� ũ��
	PathParams.ProjectileRadius = 5.f;
	// �ӽ� �߻�ü�� ������ ����
	PathParams.ActorsToIgnore.Add(GetOwner());

	// �߻�ü ������ ����׷� ǥ��
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	// �߻�ü ���� ����
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// ��弦 ��Ʈ!
	if (PathResult.HitResult.bBlockingHit)
	{
		DrawBox(PathResult.HitResult, FColor::Red);

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
			BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

			// �߻�ü ���� ����
			UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
			// �ٸ� ���� ��Ʈ!
			if (PathResult.HitResult.bBlockingHit)
			{
				DrawBox(PathResult.HitResult, FColor::Blue);

				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				// ��弦�� false
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// ���� �ʾҴ�.
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime) const
{
	TArray<FFramePackage> FramesToCheck;
	FramesToCheck.Reserve(HitCharacters.Num());

	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		if (!HitCharacter) continue;

		FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
		FrameToCheck.Character = HitCharacter;
		FramesToCheck.Add(FrameToCheck);
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& Packages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations) const
{
	if (!GetWorld()) return FShotgunServerSideRewindResult();

	FShotgunServerSideRewindResult ShotResult;

	TArray<FFramePackage> CurrentFrames;
	CurrentFrames.Reserve(Packages.Num());

	// ��� ĳ���͵��� ���� ��ġ �������� ������, ���� ������ �������� �ű��.
	for (const FFramePackage& Frame : Packages)
	{
		if (!Frame.Character) continue;

		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);

		CurrentFrames.Add(CurrentFrame);
	}

	// ���� �������� �ű� �����ӵ��� ��� �浹üũ Ȱ��ȭ
	for (const FFramePackage& Frame : CurrentFrames)
	{
		if (!Frame.Character) continue;

		if (UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[TEXT("head")])
		{
			HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
		}
	}

	// ��弦 Ȯ��
	for (const FVector_NetQuantize& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{
			DrawBox(ConfirmHitResult, FColor::Red);

			ShotResult.HeadShots.Contains(BlasterCharacter) ? ++ShotResult.HeadShots[BlasterCharacter] : ShotResult.HeadShots.Emplace(BlasterCharacter, 1);
		}
	}

	// �ٸ� ���� �浹üũ�� Ű�� ��� �浹üũ�� ��
	for (const FFramePackage& Frame : CurrentFrames)
	{
		if (!Frame.Character) continue;

		for (const auto& BoxPair : Frame.Character->GetHitCollisionBoxes())
		{
			if (!BoxPair.Value) continue;

			BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
		}

		if (UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[TEXT("head")])
		{
			HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// �ٵ� Ȯ��
	for (const FVector_NetQuantize& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{
			DrawBox(ConfirmHitResult, FColor::Blue);

			ShotResult.BodyShots.Contains(BlasterCharacter) ? ++ShotResult.BodyShots[BlasterCharacter] : ShotResult.HeadShots.Emplace(BlasterCharacter, 1);
		}
	}

	// ��Ʈ�ڽ� ��ġ�� �浹üũ ����ȭ
	for (const FFramePackage& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotResult;
}
