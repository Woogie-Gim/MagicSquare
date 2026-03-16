

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MagicSquareHpWidget.generated.h"

UCLASS()
class MAGICSQUARE_API UMagicSquareHpWidget : public UUserWidget
{
	GENERATED_BODY()
	
	public:
	// Hp 바 UI 갱신
	void UpdateHp(float CurrentHp, float MaxHp);

protected:
	// 블루프린트 UProgressBar 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UProgressBar> PB_HpBar;
};
