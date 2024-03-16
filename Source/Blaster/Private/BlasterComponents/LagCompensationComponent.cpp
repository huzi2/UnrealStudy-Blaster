// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"

ULagCompensationComponent::ULagCompensationComponent()
	: MaxRecordTime(4.0)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character) return;
	// 프레임 저장은 서버만 하면됨
	if (!Character->HasAuthority()) return;

	SaveFramePackage();
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime, AWeapon* DamageCauser) const
{
	if (!HitCharacter) return;
	if (!DamageCauser) return;
	if (!Character) return;

	// 서버 되감기 요청
	const FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	// 서버 되감기 결과 타겟을 맞춤
	if (Confirm.bHitConfirmed)
	{
		// 타겟을 맞췄으니 데미지를 준다.
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
		// 헤드샷 데미지
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			TotalDamage += Confirm.HeadShots[HitCharacter] * DamageCauser->GetDamage();
		}
		// 바디샷 데미지
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
	// 프레임이 아예없으면 하나를 저장하고 시간 기록 시작
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	// 프레임이 있다면 지정된 시간까지만 기록함
	else
	{
		// 맨 앞과 맨 뒤 노드의 시간 차이가 총 저장되있는 시간
		double HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		// 설정한 시간보다 적어질 때까지 노드 제거
		while (HistoryLength > MaxRecordTime)
		{
			// 오래된 프레임은 꼬리에 쌓여있으므로 꼬리를 삭제
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		// 새 프레임은 헤드에 저장
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		// 프레임별로 저장되는 패키지를 디버그로 보여줌
		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	if (!GetWorld()) return;

	Init();

	// 캐릭터의 모든 히트박스를 서버시간에 맞춰서 위치, 회전, 크기를 저장한다.
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
	// 서버가 되감아서 확인해야할 프레임 패키지
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

	// 맞은 캐릭터의 프레임 히스토리
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const double OldestHistoryTime = History.GetTail()->GetValue().Time;
	// 맞은 시간이 기록된 시간보다 더 이전이라서 서버 되돌리기가 불가능
	if (OldestHistoryTime > HitTime) return FFramePackage();

	// 가장 오래된 것과 같으면 그걸 확인
	if (OldestHistoryTime == HitTime) return History.GetTail()->GetValue();

	// 가장 최근 것과 같거나 기록된 시간보다 적중시간이 빠르면 가장 최근 기록된 것을 확인
	const double NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (NewestHistoryTime <= HitTime) return History.GetHead()->GetValue();

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();

	// 히스토리 중 HitTime의 앞 뒤 시간을 확인
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

	// Older와 Younger 사이의 값으로 보간
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

		// 위치와 회전값을 두 프레임 사이의 적절한 값으로 보간한다.
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

	// 맞은 캐릭터의 현재 위치 히트박스 정보를 얻어옴(되감기 후에 다시 원래대로 돌릴 때 사용)
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	
	// 캐릭터의 히트박스를 패키지 정보대로 수정(되감기)
	MoveBoxes(HitCharacter, Package);

	// 메시의 충돌처리는 끈다.
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);
	
	// head부터 활성화시켜서 맞았는지 확인
	if (UBoxComponent* HeadBox = HitCharacter->GetHitCollisionBoxes()[TEXT("head")])
	{
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// 나중에 이 부분 아예 생성할 때 세팅해두는건 어떤지 확인
		HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

		FHitResult ConfirmHitResult;
		// 충돌체크를 위해 원래 맞은 위치보다 조금 더 늘림
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
		// 헤드샷 히트!
		if (ConfirmHitResult.bBlockingHit)
		{
			// 맞은 캐릭터의 히트박스 정보를 원래대로 되돌린다.
			ResetHitBoxes(HitCharacter, CurrentFrame);
			// 메시의 충돌처리도 다시 킨다.
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			// 맞은 정보와 헤드샷 정보를 true로 리턴
			return FServerSideRewindResult{ true, true };
		}
		// 헤드에 맞지 않았으므로 다른 부위의 히트박스 확인
		else
		{
			for (const auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
			{
				if (!BoxPair.Value) continue;

				BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				BoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

				GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
				// 다른 부위 히트!
				if (ConfirmHitResult.bBlockingHit)
				{
					ResetHitBoxes(HitCharacter, CurrentFrame);
					EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
					// 헤드샷만 false
					return FServerSideRewindResult{ true, false };
				}
			}
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// 맞지 않았다.
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) const
{
	// 맞은 캐릭터의 현재 히트박스 정보를 가져온다.
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
	// 맞은 캐릭터의 히트박스를 프레임 정보를 통해 서버시간대로 되감기
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
	// 맞은 캐릭터의 히트박스를 프레임 정보를 통해 원래대로 되돌림
	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (!BoxPair.Value) continue;

		BoxPair.Value->SetWorldLocation(Package.HitBoxInfo[BoxPair.Key].Location);
		BoxPair.Value->SetWorldRotation(Package.HitBoxInfo[BoxPair.Key].Rotation);
		BoxPair.Value->SetBoxExtent(Package.HitBoxInfo[BoxPair.Key].BoxExtent);
		// 충돌끄기
		BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled) const
{
	if (!HitCharacter) return;
	if (!HitCharacter->GetMesh()) return;

	HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
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

	// 모든 캐릭터들의 현재 위치 프레임을 얻어오고, 서버 프레임 기준으로 옮긴다.
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

	// 서버 기준으로 옮긴 프레임들의 헤드 충돌체크 활성화
	for (const FFramePackage& Frame : CurrentFrames)
	{
		if (!Frame.Character) continue;

		if (UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[TEXT("head")])
		{
			HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		}
	}

	// 헤드샷 확인
	for (const FVector_NetQuantize& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);

		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{
			ShotResult.HeadShots.Contains(BlasterCharacter) ? ++ShotResult.HeadShots[BlasterCharacter] : ShotResult.HeadShots.Emplace(BlasterCharacter, 1);
		}
	}

	// 다른 부위 충돌체크를 키고 헤드 충돌체크는 끔
	for (const FFramePackage& Frame : CurrentFrames)
	{
		if (!Frame.Character) continue;

		for (const auto& BoxPair : Frame.Character->GetHitCollisionBoxes())
		{
			if (!BoxPair.Value) continue;

			BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			BoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		}

		if (UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[TEXT("head")])
		{
			HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// 바디샷 확인
	for (const FVector_NetQuantize& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);

		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{
			ShotResult.BodyShots.Contains(BlasterCharacter) ? ++ShotResult.BodyShots[BlasterCharacter] : ShotResult.HeadShots.Emplace(BlasterCharacter, 1);
		}
	}

	// 히트박스 위치와 충돌체크 정상화
	for (const FFramePackage& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotResult;
}
