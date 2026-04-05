
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
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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

	// 카메라 설정값 저장 및 나이아가라/Spline 생성

	// 원래 카메라 설정값 저장
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
				// 마법진 평면 깊이 (원하는 거리에 따라 조절 가능)
				const float DrawingPlaneDepth = 150.0f;
				FVector DrawPoint = HeadLocation + (MouseWorldDirection * DrawingPlaneDepth);

				NiagaraComponent->SetVariablePosition(TEXT("DrawPosition"), DrawPoint);
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

	// 카메라 사이드 설정 (목표 값으로 변경)
	CameraBoom->TargetArmLength = DrawingCameraTargetArmLength;
	CameraBoom->SocketOffset = DrawingCameraSocketOffset;

	// 마우스 커서 활성화 및 입력 모드 변경 (LockAlways 사용 권장)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
	}

	// 나이아가라 활성화 및 Spline 초기화
	if (DrawingEffect)
	{
		NiagaraComponent->SetAsset(DrawingEffect);
		NiagaraComponent->Activate();
	}
	DrawSplineComponent->ClearSplinePoints(true);
}

void AMagicSquareCharacter::StopDrawing(const FInputActionValue& Value)
{
	if (!bIsDrawingMode) return;
	bIsDrawingMode = false;

	// 시간 정상으로 복구
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

	// 카메라 설정 복구
	CameraBoom->TargetArmLength = DefaultCameraTargetArmLength;
	CameraBoom->SocketOffset = DefaultCameraSocketOffset;

	// 마우스 커서 비활성화 및 입력 모드 복구
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	// 나이아가라 비활성화 및 Spline 제거
	NiagaraComponent->Deactivate();
	DrawSplineComponent->ClearSplinePoints(true);
}
