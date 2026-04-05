
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"	// Enhanced Input 값을 받기 위함
#include "NiagaraComponent.h"
#include "Components/SplineComponent.h"
#include "MagicSquareCharacter.generated.h"

UCLASS()
class MAGICSQUARE_API AMagicSquareCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMagicSquareCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 드로잉 모드 시작 / 종료 함수
	void StartDrawing(const FInputActionValue& Value);
	void StopDrawing(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 데미지 처리 오버라이드
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 키보드 입력 시 호출할 구르기 실행 함수
    void Dodge();

protected:
	// 카메라 붐 (캐릭터 뒤에서 카메라를 일정 거리 유지해주는 역할)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;
	
	// 메인 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	// 기본 입력 매핑 컨텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	// 이동 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	// 시점 회전 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> LookAction;

	// 구르기 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> DodgeAction;

	// 이동 입력 처리 함수
	void Move(const FInputActionValue& Value);

	// 시점 회전 입력 처리 함수
	void Look(const FInputActionValue& Value);

	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHp = 100.0f;

	// 현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float CurrentHp;

	// UI 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UMagicSquareHpWidget> HpWidgetClass;

	// 생성된 UI 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<class UMagicSquareHpWidget> HpWidgetInstance;

	// 에디터에서 할당할 구르기 어빌리티 클래스 (BP_GA_Roll)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayAbility> RollAbilityClass;

	// 캐릭터가 가지고 있는 어빌리티 시스템 컴포넌트 포인터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	// 드로잉 입력 액션 (우클릭)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> DrawAction;

	// 시간 느려지는 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Design")
	float TimeSlowFactor = 0.2f;

	// 현재 드로잉 모드 여부
	bool bIsDrawingMode = false;

	// 카메라 사이드 설정 변수
	// 드로잉 모드 스프링 암 목표 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DrawingCameraTargetArmLength = 150.0f; // 캐릭터에게 바짝 다가감

	// 드로잉 모드 소켓 오프셋 (캐릭터의 오른쪽 어깨 너머 시점)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector DrawingCameraSocketOffset = FVector(0.0f, 60.0f, 30.0f); // 약간 우측 상단

	// 원래 카메라 설정값 저장용 (복구용)
	float DefaultCameraTargetArmLength;
	FVector DefaultCameraSocketOffset;

	// 마법진 궤적 시각화 변수
	// 마법진 궤적 시각화 나이아가라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<class UNiagaraComponent> NiagaraComponent;

	// 마법진 궤적 데이터 저장용 Spline 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
	TObjectPtr<class USplineComponent> DrawSplineComponent;

	// 에디터에서 할당할 나이아가라 시스템 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<class UNiagaraSystem> DrawingEffect;
};
