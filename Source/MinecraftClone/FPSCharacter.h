// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Chunk.h"
#include "FPSCharacter.generated.h"

UCLASS()
class MINECRAFTCLONE_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFPSCharacter();

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float weaponRange{ 1000 };

	UPROPERTY(EditAnywhere, Category = "ChunkGeneration")
	int32 CHUNK_RENDER_DISTANCE { 10 };

	UFUNCTION(BlueprintImplementableEvent, Category = "ChunkGeneration")
	BlockType BlueprintPopulateBlock(int32 i, int32 j, int32 k);

	TFunction<BlockType(int32 i, int32 j, int32 k)> PopulateBlockFunction = NULL;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PrimaryFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SecondaryFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ChangeBlockInHand(BlockType newBlockType);

	TQueue<TArray<MeshData*>> chunkRenderQueue;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	FHitResult InstantShot();

public:	
	BlockType blockInHand = BlockType::DIRT;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	int32 LastChunkX = 1000;
	int32 LastChunkY = 1000;
};
