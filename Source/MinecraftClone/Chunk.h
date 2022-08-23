// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Engine/Engine.h"
#include "Containers/Map.h"
#include "Chunk.generated.h"

UENUM(BlueprintType)
enum class BlockType : uint8
{
	AIR UMETA(DisplayName = "AIR"),
	GRASS UMETA(DisplayName = "GRASS"),
	DIRT UMETA(DisplayName = "DIRT"),
	STONE UMETA(DisplayName = "STONE"),
	WOOD UMETA(DisplayName = "WOOD"),
	LEAVES UMETA(DisplayName = "LEAVES")
};

class MeshData
{
public:
	TArray<FVector> vertices;
	TArray<int32> Triangles;
	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> tangents;
	TArray<FLinearColor> vertexColors;
	TArray<BlockType> blocks;
	int32 sectionSide;
	int32 sectionCount;
	int32 chunkI;
	int32 chunkJ;

	static enum Direction
	{
		UP = 0,
		DOWN = 1,
		LEFT = 2,
		RIGHT = 3,
		FORWARD = 4,
		BACK = 5,
		SIZE = 6
	};

	const FString DirectionImage[Direction::SIZE][8] =
	{
		"UP", "DOWN", "LEFT", "RIGHT", "FORWARD", "BACK"
	};


	const int32 ATLAS_SIZE = 4;
	// Top1, Side1, Bottom1, Top2, Side2, Bottom2, Top3, Side3, Bottom3, ...
	// Needs to be BLOCKTYPE::SIZE * 3
	const int32 BlockTypeTextureIndex[6][3] =
	{
		{-1, -1, -1},// AIR
		{0,  3,  2},// GRASS
		{2,  2,  2},// DIRT
		{1,  1,  1},// STONE
		{5,  4,  5},// WOOD
		{6,  6,  6}// LEAVES
	};

	const FVector NORMALS[Direction::SIZE] =
	{ FVector(0,0,1), // up
	  FVector(0,0,-1), // down
	  FVector(-1,0,0), // left
	  FVector(1,0,0), // right
	  FVector(0,-1,0), // forward
	  FVector(0,1,0) // back 
	};

	const FProcMeshTangent TANGENTS[Direction::SIZE] =
	{ FProcMeshTangent(1, 0, 0), // up
	  FProcMeshTangent(1, 0, 0), // down
	  FProcMeshTangent(0, 0, 1), // left
	  FProcMeshTangent(0, 0, 1), // right
	  FProcMeshTangent(0, 0, 1), // forward
	  FProcMeshTangent(0, 0, 1) // back
	};

	const FVector VERTICES[Direction::SIZE][4] =
	{
		{ FVector(1, 0, 1), FVector(0, 0, 1), FVector(1, 1, 1), FVector(0, 1, 1) },
		{ FVector(0, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(1, 1, 0) },
		{ FVector(0, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0, 1, 1) },
		{ FVector(1, 1, 0), FVector(1, 0, 0), FVector(1, 1, 1), FVector(1, 0, 1) },
		{ FVector(0, 0, 0), FVector(0, 0, 1), FVector(1, 0, 0), FVector(1, 0, 1) },
		{ FVector(0, 1, 0), FVector(1, 1, 0), FVector(0, 1, 1), FVector(1, 1, 1) }
	};

	const FVector2D UVS[Direction::SIZE][4] =
	{
		{ FVector2D(0.9, 0.1), FVector2D(0.1, 0.1), FVector2D(0.9, 0.9), FVector2D(0.1, 0.9) }, // up
		{ FVector2D(0.01, 0.01), FVector2D(0.99, 0.01), FVector2D(0.01, 0.99), FVector2D(0.99, 0.99) }, // down
		{ FVector2D(0.01, 0.01), FVector2D(0.99, 0.01), FVector2D(0.01, 0.99), FVector2D(0.99, 0.99) }, // left
		{ FVector2D(0.99, 0.01), FVector2D(0.01, 0.01), FVector2D(0.99, 0.99), FVector2D(0.01, 0.99) }, // right
		{ FVector2D(0.01, 0.01), FVector2D(0.01, 0.99), FVector2D(0.99, 0.01), FVector2D(0.99, 0.99) }, // forward
		{ FVector2D(0.01, 0.01), FVector2D(0.99, 0.01), FVector2D(0.01, 0.99), FVector2D(0.99, 0.99) }  // back
	};

	const FVector2D UVS_Inverted[Direction::SIZE][4] =
	{
		{ FVector2D(0, 1), FVector2D(1, 1), FVector2D(0, 0), FVector2D(1, 0) }, // up
		{ FVector2D(1, 1), FVector2D(0, 1), FVector2D(1, 0), FVector2D(0, 0) }, // down
		{ FVector2D(1, 1), FVector2D(0, 1), FVector2D(1, 0), FVector2D(0, 0) }, // left
		{ FVector2D(0, 1), FVector2D(1, 1), FVector2D(0, 0), FVector2D(1, 0) }, // right
		{ FVector2D(1, 1), FVector2D(1, 0), FVector2D(0, 1), FVector2D(0, 0) }, // forward
		{ FVector2D(1, 1), FVector2D(0, 1), FVector2D(1, 0), FVector2D(0, 0) }  // back
	};
};

UCLASS(Blueprintable)
class MINECRAFTCLONE_API AChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChunk();

	void PostDuplicate(EDuplicateMode::Type DuplicateMode);

	void BeginDestroy();

	static TArray<BlockType> GenerateChunkData(int chunkI, int chunkJ, int sectionSideWidth, int numberOfSections,
		int& sideWidth, int& sectionCount,
		TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock);

	UFUNCTION(BlueprintCallable, Category = "VoxelChunk")
	void CreateVoxelChunk(TArray<BlockType> blocks, int sectionSide, int sectionCount);

	void AddVoxel(FVector insidePoint, BlockType blockTypeToAdd);

	void RemoveVoxel(FVector insidePoint);

	static void PlayerMovedToAnotherChunk(int newChunkX, int newChunkY, TQueue<TArray<MeshData*>>& chunkLoaderQueue,
		TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock,
		int32 ChunkRenderDistance);

	static TMap<int32, AChunk*> ChunkMap;
	const static int BlockSize = 100;

	static TMap <int32, TFuture<TArray<MeshData*>>*> chunkResults;

	static MeshData* GetMeshData(int32 chunkI, int32 chunkJ, 
		int32 sectionID, int32 sectionCount, TArray<BlockType> blocks, int32 sectionSide);
	static TArray<MeshData*> GetMeshDataForChunk(int32 chunkI, int32 chunkJ, 
		int32 sectionCount, TArray<BlockType> blocks, int32 sectionSide);
	static TArray<MeshData*> GetMeshDataForChunk(int32 ChunkX, int32 ChunkY,
		TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock);

	void static CreateChunk(int32 ChunkX, int32 ChunkY, UWorld* World);
	void static CreateChunk(int32 ChunkX, int32 ChunkY, UWorld* World, TArray<MeshData*> chunkData);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* mesh;

	UPROPERTY(VisibleAnywhere)
	int mSectionSide = 10;

	UPROPERTY(VisibleAnywhere)
	int mSectionCount = 10;

	UPROPERTY(VisibleAnywhere)
	TArray<BlockType> mBlocks;

	void PostActorCreated();

	void PostLoad();

	static bool CheckIfNeighboorIsAir(MeshData::Direction direction,
		TArray<BlockType>& blocks, int i, int j, int k, 
		int sectionCount, int sectionSide, MeshData& data);

	static void AddVoxelFace(MeshData::Direction direction, 
		BlockType currentBlockType,
		MeshData* data,
		int i, int j, int k);

	static int GetPositionInTArray(int i, int j, int k, int sectionSide);

	static void GetIJKFromPositionInTArray(int pos, int sectionSide, int &i, int &j, int &k);

	static int GetHashFromChunkPosition(int ChunkX, int ChunkY)
	{
		uint32 half16bit = 1 << 15;
		return uint32(uint32(ChunkX + half16bit) << 16) + uint16(ChunkY + half16bit);
	}

	void CreateTriangle();
};

class FChunkCreateTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FChunkCreateTask>;

public:
	FChunkCreateTask(int32 chunkX, int32 chunkY, UWorld* world) :
		ChunkX(chunkX),
		ChunkY(chunkY),
		World(world)
	{}

protected:
	int32 ChunkX;
	int32 ChunkY;
	UWorld* World;

	void DoWork();

	// This next section of code needs to be here.  Not important as to why.

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMyTaskName, STATGROUP_ThreadPoolAsyncTasks);
	}
};
