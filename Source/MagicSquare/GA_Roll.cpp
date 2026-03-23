
#include "GA_Roll.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"

UGA_Roll::UGA_Roll()
{
	// 액터 당 인스턴스 생성 정책 설정 (태스크 사용을 위함)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Roll::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 자원 및 쿨타임 검증
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 재생 비동기 태스크 생성
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, RollMontage);

	if (MontageTask)
	{
		// 태스크 종료 델리게이트 바인딩
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Roll::OnMontageEnded);
		MontageTask->OnCompleted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Roll::OnMontageEnded);

		// 태스크 활성화
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 태스크 생성 실패 시 어빌리티 종료
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Roll::OnMontageEnded()
{
	// 어빌리티 정상 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

