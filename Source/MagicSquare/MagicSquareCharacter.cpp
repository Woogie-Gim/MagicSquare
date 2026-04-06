#include "MagicSquareCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "MagicSquareHpWidget.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"

// Sets default values
AMagicSquareCharacter::AMagicSquareCharacter()
{
	// Set this character to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// 컨트롤러 회전에 따른 캐릭터 회전 비활성화
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 캐릭터 자동 회전 활성화
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// 카메라 붐 생성 및 초기 세팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CamreaBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// 메인 카메라 생성 및 카메라 붐에 부착
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 어빌리티 시스템 컴포넌트 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// --- 카메라 설정값 저장 및 나이아가라/Spline 생성 ---

	// 원래 카메라 설정값 저장 (나중에 드로잉 모드 종료 시 복구용)
	DefaultCameraTargetArmLength = CameraBoom->TargetArmLength;
	DefaultCameraSocketOffset = CameraBoom->SocketOffset;

	// 나이아가라 컴포넌트 생성 및 머리 위치에 부착 (마법진이 그려지는 중심점 역할을 위함)
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(GetMesh(), TEXT("head"));
	NiagaraComponent->bAutoActivate = false; // 기본적으로 꺼둠

	// Spline 컴포넌트 생성 및 루트 부착 (궤적 데이터 저장용)
	DrawSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("DrawSplineComponent"));
	DrawSplineComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AMagicSquareCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 체력 초기화
	CurrentHp = MaxHp;

	// UI 위젯 생성 및 화면 출력
	if (HpWidgetClass)
	{
		HpWidgetInstance = CreateWidget<UMagicSquareHpWidget>(GetWorld(), HpWidgetClass);
		if (HpWidgetInstance)
		{
			HpWidgetInstance->AddToViewport();
			HpWidgetInstance->UpdateHp(CurrentHp, MaxHp);
		}
	}

	// Enhanced Input Local Player Subsystem 획득 및 매핑 컨텍스트 추가
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (AbilitySystemComponent != nullptr)
	{
		// 어빌리티 시스템 컴포넌트에게 오너와 아바타가 자신(this)임을 알려줌 (필수 초기화)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (RollAbilityClass != nullptr && HasAuthority())
		{
			FGameplayAbilitySpec Spec(RollAbilityClass, 1, INDEX_NONE, this);
			AbilitySystemComponent->GiveAbility(Spec);
		}
	}
}

// Called every frame
void AMagicSquareCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 드로잉 모드일 때만 궤적 업데이트 실행
	if (bIsDrawingMode)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FVector MouseWorldLocation, MouseWorldDirection;
			// 마우스 화면 좌표를 월드 좌표로 변환
			if (PC->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection))
			{
				// 캐릭터 머리 위치에서 마우스 방향으로 일정 거리 떨어진 지점 (마법진이 그려지는 평면) 계산
				const FVector HeadLocation = GetMesh()->GetSocketLocation(TEXT("head"));
				// 마법진 평면 깊이
				const float DrawingPlaneDepth = 150.0f;
				FVector DrawPoint = HeadLocation + (MouseWorldDirection * DrawingPlaneDepth);

				// 시각화용 나이아가라 위치 업데이트 (DrawPosition 파라미터에 좌표 전달)
				NiagaraComponent->SetVariablePosition(TEXT("DrawPosition"), DrawPoint);

				// 판별용 스플라인 데이터 저장
				DrawSplineComponent->AddSplinePoint(DrawPoint, ESplineCoordinateSpace::World);
			}
		}
	}
}

// Called to bind functionality to input
void AMagicSquareCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// EnhancedInputComponetn로 캐스팅 후 액션 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagicSquareCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagicSquareCharacter::Look);
		// 구르기 입력 액션 바인딩
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AMagicSquareCharacter::Dodge);

		// 우클릭 시작과 종료 바인딩
		EnhancedInputComponent->BindAction(DrawAction, ETriggerEvent::Started, this, &AMagicSquareCharacter::StartDrawing);
		EnhancedInputComponent->BindAction(DrawAction, ETriggerEvent::Completed, this, &AMagicSquareCharacter::StopDrawing);
	}
}

float AMagicSquareCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 부모 클래스 데미지 처리 수행
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 체력 감소 및 범위 제한
	CurrentHp = FMath::Clamp(CurrentHp - ActualDamage, 0.0f, MaxHp);

	// UI 갱신
	if (HpWidgetInstance)
	{
		HpWidgetInstance->UpdateHp(CurrentHp, MaxHp);
	}

	return ActualDamage;
}

void AMagicSquareCharacter::Dodge()
{
	if (AbilitySystemComponent != nullptr && RollAbilityClass != nullptr)
	{
		// 부여된 어빌리티들 중 해당 클래스를 찾아 실행 시도
		AbilitySystemComponent->TryActivateAbilityByClass(RollAbilityClass);
	}
}

void AMagicSquareCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 컨트롤러의 회전값을 기준으로 전방 및 우측 벡터 계산
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 입력 벡터 기반으로 이동 적용
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMagicSquareCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 마우스 입력값을 컨트롤러의 Yaw, Pitch에 적용
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMagicSquareCharacter::StartDrawing(const FInputActionValue& Value)
{
	if (bIsDrawingMode) return;
	bIsDrawingMode = true;

	// 시간 느리게 설정
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeSlowFactor);

	// 캐릭터 어깨 너머로 더 멀리서, 위에서 내려다보게 설정하여 드로잉 영역 확보
	CameraBoom->TargetArmLength = 450.0f; // 기존 150 -> 450 (멀어짐)
	CameraBoom->SocketOffset = FVector(0.0f, 120.0f, 100.0f); // 약간 우측, 높게 (시야 개선)

	// 마우스 커서 활성화 및 입력 모드 변경 (LockAlways 사용 권장)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
	}

	// 나이아가라 활성화
	if (DrawingEffect)
	{
		NiagaraComponent->SetAsset(DrawingEffect);
		NiagaraComponent->Activate();
	}
	// 스플라인 궤적 초기화
	DrawSplineComponent->ClearSplinePoints(true);
}

void AMagicSquareCharacter::StopDrawing(const FInputActionValue& Value)
{
	if (!bIsDrawingMode) return;
	bIsDrawingMode = false;

	// 시간 정상으로 복구
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

	// 카메라 설정 복구 (원래 값으로)
	CameraBoom->TargetArmLength = DefaultCameraTargetArmLength;
	CameraBoom->SocketOffset = DefaultCameraSocketOffset;

	// 마우스 커서 비활성화 및 입력 모드 복구
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	// 나이아가라 비활성화
	NiagaraComponent->Deactivate();

	// 스플라인 초기화 전 마법진 패턴 분석 실행
	AnalyzeDrawing();

	DrawSplineComponent->ClearSplinePoints(true);
}

void AMagicSquareCharacter::AnalyzeDrawing()
{
	// 유효 길이 검사 (너무 짧으면 무시)
	float SplineLength = DrawSplineComponent->GetSplineLength();
	if (SplineLength < 100.0f) return;

	// 데이터 재샘플링 (동일 간격 점 획득)
	int32 SampleCount = 30; // 40 -> 30으로 축소하여 계산 최적화 및 구불거림 무시
	float Step = SplineLength / SampleCount;
	TArray<FVector> SampledPoints;

	for (int32 i = 0; i <= SampleCount; ++i)
	{
		FVector Point = DrawSplineComponent->GetLocationAtDistanceAlongSpline(Step * i, ESplineCoordinateSpace::World);
		SampledPoints.Add(Point);
	}

	// 도형 닫힘 확인 (시작점과 끝점의 거리)
	float DistanceStartEnd = FVector::Distance(SampledPoints[0], SampledPoints.Last());
	bool bIsClosed = DistanceStartEnd < (SplineLength * 0.3f); // 0.25 -> 0.3으로 약간 루즈하게 설정

	// 도형 판별을 위한 수치 계산
	int32 SharpCornerCount = 0; // 예각으로 날카롭게 꺾인 곳
	int32 SmoothCornerCount = 0; // 둔각으로 부드럽게 꺾인 곳
	float TotalCurvature = 0.0f; // 전체 곡률 합

	for (int32 i = 1; i < SampledPoints.Num() - 1; ++i)
	{
		FVector Dir1 = (SampledPoints[i] - SampledPoints[i - 1]).GetSafeNormal();
		FVector Dir2 = (SampledPoints[i + 1] - SampledPoints[i]).GetSafeNormal();

		// 내적을 통한 꺾임 각도 계산
		float DotProduct = FVector::DotProduct(Dir1, Dir2);
		float Curvature = FMath::Acos(DotProduct); // 곡률(라디안)
		TotalCurvature += Curvature;

		// 각도 임계값 설정
		if (DotProduct < 0.1f) // 날카롭게 꺾임 (약 84도 이상)
		{
			SharpCornerCount++;
		}
		else if (DotProduct < 0.85f) // 부드럽게 곡선 (약 32도 이상)
		{
			SmoothCornerCount++;
		}
	}

	FString ShapeName = TEXT("Unknown");
	FColor LogColor = FColor::Red;

	// 도형 판별 로직
	if (bIsClosed)
	{
		// 전체 곡률 합 기반 원 판별 (2파이(6.28) 내외)
		// 둔각 꺾임이 많고 날카로운 꺾임이 거의 없을 것
		if (TotalCurvature >= 5.0f && TotalCurvature <= 8.5f && SharpCornerCount <= 2)
		{
			ShapeName = TEXT("Circle (Circle)");
			LogColor = FColor::Cyan;
		}
		// 날카로운 꼭짓점 개수 기반 판별
		else if (SharpCornerCount >= 2 && SharpCornerCount <= 4)
		{
			ShapeName = TEXT("Triangle (Triangle)");
			LogColor = FColor::Yellow;
		}
		else if (SharpCornerCount >= 4 && SharpCornerCount <= 6)
		{
			ShapeName = TEXT("Square (Square)");
			LogColor = FColor::Green;
		}
	}

	// 결과 출력
	UE_LOG(LogTemp, Warning, TEXT("Analyze: Shape=%s, SharpCorners=%d, TotalCurvature=%.2f, Closed=%d"), *ShapeName, SharpCornerCount, TotalCurvature, bIsClosed);

	if (GEngine)
	{
		if (ShapeName != TEXT("Unknown"))
		{
			FString Message = FString::Printf(TEXT("Activated Magic Square: %s!"), *ShapeName);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, LogColor, Message);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Magic Square Failed: Unknown Pattern"));
		}
	}
}