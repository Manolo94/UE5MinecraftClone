// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacter.h"
#include "Engine/World.h"
#include "DamageableActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Chunk.h"

// Sets default values
AFPSCharacter::AFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 3rd person camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true; // rotate arm based on controller

	// 3rd person camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // camera follows the arm, not directly

	// 3rd person feel
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // turn speed
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	AChunk::ChunkMap.Empty();

	if (!PopulateBlockFunction)
	{
		PopulateBlockFunction =
			[WeakActor = MakeWeakObjectPtr(this)](int32 i, int32 j, int32 k, FChunkInfo chunkInfo) {
			if (!WeakActor.IsValid()) return BlockType::AIR;
			return WeakActor->BlueprintPopulateBlock(i, j, k, chunkInfo);
			};
	}

	// TODO: Use a heap to give priority to the chunks closest to the player
	TArray<MeshData*> d = AChunk::GetMeshDataForChunk(0, 0, PopulateBlockFunction);
	chunkRenderQueue.Enqueue(d);
}

void AFPSCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AChunk::ClearAllQueuedChunkTasks();
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ChunkMap handling
	// TODO: Stop making these static magic numbers
	// (1000,1000) -> (0,0); (-1000, -1000) -> (-1, -1); (-1601, -1601) -> (-2, -2); (1601, 1601) -> (1, 1)
	int32 chunkX = floor(GetActorLocation().X / 1600.0f);
	int32 chunkY = floor(GetActorLocation().Y / 1600.0f);

	if (LastChunkX != chunkX || LastChunkY != chunkY)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Yellow, FString::Printf(TEXT("Moved to new chunk %d %d"), chunkX, chunkY));

		LastChunkX = chunkX; LastChunkY = chunkY;

		AChunk::PlayerMovedToAnotherChunk(chunkX, chunkY, chunkRenderQueue, 
			PopulateBlockFunction, CHUNK_RENDER_DISTANCE);
	}


	if(!chunkRenderQueue.IsEmpty())
	{
		TArray<MeshData*> chunkData; chunkRenderQueue.Dequeue(chunkData);

		AChunk::CreateChunk(chunkData[0]->chunkI, chunkData[0]->chunkJ, GetWorld(), chunkData);
	}
}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFPSCharacter::PrimaryFire()
{
	FHitResult hit = InstantShot();
	AChunk* hitActor = Cast<AChunk>(hit.GetActor());

	if (hitActor)
	{
		FVector pointInside = hit.ImpactPoint + hit.ImpactNormal - hit.GetActor()->GetActorLocation();

		hitActor->AddVoxel(pointInside, blockInHand);
	}
}

void AFPSCharacter::SecondaryFire()
{
	FHitResult hit = InstantShot();
	AChunk* hitActor = Cast<AChunk>(hit.GetActor());

	if (hitActor)
	{
		FVector pointInside = hit.ImpactPoint - hit.ImpactNormal - hit.GetActor()->GetActorLocation();

		hitActor->RemoveVoxel(pointInside);
	}
}

void AFPSCharacter::ChangeBlockInHand(BlockType newBlockType)
{
	if (newBlockType > BlockType::AIR && newBlockType <= BlockType::LEAVES)
		blockInHand = newBlockType;
}

FHitResult AFPSCharacter::InstantShot()
{
	FVector  rayLocation;
	FRotator rayRotation;
	FVector  endTrace;

	APlayerController* const playerController = GetWorld()->GetFirstPlayerController();
	if (playerController)
	{
		playerController->GetPlayerViewPoint(rayLocation, rayRotation);

		endTrace = rayLocation + rayRotation.Vector() * weaponRange;
	}

	FCollisionQueryParams traceParams(SCENE_QUERY_STAT(InstantShot), true, GetInstigator());
	FHitResult hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(hit, rayLocation, endTrace, ECC_Visibility, traceParams);

	return hit;
}

