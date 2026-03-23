
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Roll.generated.h"

/**
 * 
 */
UCLASS()
class MAGICSQUARE_API UGA_Roll : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Roll();

	// 어빌리티 활성화 시 호출
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 몽타주 재생 종료 시 호출
	UFUNCTION()
	void OnMontageEnded();

	// 구르기 애니메이션 몽타주 자산
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> RollMontage;
};
