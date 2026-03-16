
#include "MagicSquareHpWidget.h"
#include "Components/ProgressBar.h"

void UMagicSquareHpWidget::UpdateHp(float CurrentHp, float MaxHp)
{
	// 프로그레스 바 유효성 및 0 나누기 방지 검사
	if (PB_HpBar && MaxHp > 0.0f)
	{
		// 0.0 ~ 1.0 비율 값 적용
		PB_HpBar->SetPercent(CurrentHp / MaxHp);
	}
}
