
#include "MagicSquareCharacter.h"

// Sets default values
AMagicSquareCharacter::AMagicSquareCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMagicSquareCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMagicSquareCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMagicSquareCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

