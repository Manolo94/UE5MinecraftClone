// Fill out your copyright notice in the Description page of Project Settings.

#include "Chunk.h"
#include "Async/Async.h"

// Creating a standard root object.
AChunk::AChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = mesh;
	// New in UE 4.17, multi-threaded PhysX cooking.
	mesh->bUseAsyncCooking = true;
	// Sets default values

	UMaterial* m = LoadObject<UMaterial>(NULL, TEXT("/Game/Textures/TileTextures_Mat"));

	for(int i = 0; i < 16; i++)
		mesh->SetMaterial(i, m);


	UE_LOG(LogTemp, Log, TEXT("Generated %d"), this);
}

void AChunk::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	UE_LOG(LogTemp, Log, TEXT("Duplicated to %d"), this);
}

void AChunk::BeginDestroy()
{
	Super::BeginDestroy();

	UE_LOG(LogTemp, Log, TEXT("Destroying %d"), this);
}

// This is called when actor is spawned (at runtime or when you drop it into the world in editor)
void AChunk::PostActorCreated()
{
	Super::PostActorCreated();

	UE_LOG(LogTemp, Log, TEXT("PostActorCreated Blocks Size %d %d %d"), mBlocks.Num(), &mBlocks, this);
}

// This is called when actor is already in level and map is opened
void AChunk::PostLoad()
{
	Super::PostLoad();

	UE_LOG(LogTemp, Log, TEXT("PostLoad Chunk Mem Size %d"), sizeof(mBlocks));
}

// Each chunk it's a stack of sections, one on top of another
// For a 16x16x256 chunk, the sectionSideWidth will be 16, and the number of Sections is 16, 16*16 = 256
TArray<BlockType> AChunk::GenerateChunkData(int chunkI, int chunkJ, int sectionSideWidth, int numberOfSections,
	int& sideWidth, int& sectionCount,
	TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock)
{
	// Setup the chunk size
	sideWidth = sectionSideWidth; sectionCount = numberOfSections;

	// Setup the block array
	TArray<BlockType> blocks;
	blocks.SetNum(numberOfSections * sectionSideWidth * sectionSideWidth * sectionSideWidth);

	int I, J, K; 
	for (int i = 0; i < numberOfSections * sectionSideWidth * sectionSideWidth * sectionSideWidth; i++)
	{
		GetIJKFromPositionInTArray(i, sectionSideWidth, I, J, K);
		blocks[i] = PopulateBlock(I, J, K);
	}

	return blocks;
}

void AChunk::CreateVoxelChunk(TArray<BlockType> blocks, int sectionSide, int sectionCount)
{
	mBlocks = TArray<BlockType>(blocks);
	mSectionSide = sectionSide; mSectionCount = sectionCount;

	UE_LOG(LogTemp, Log, TEXT("Blocks Size %d %d %d"), mBlocks.Num(), &mBlocks, this);

	// Generate the mesh data by sections, each of sectionSide*sectionSide, start at the bottom

	for (int32 section = 0; section < mSectionCount; section++)
	{
		// Get mesh data for this section only
		MeshData* d = GetMeshData(0,0, section, mSectionCount, mBlocks, mSectionSide);

		// Create the section
		mesh->CreateMeshSection_LinearColor(section, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);

		delete(d); d = NULL;
	}

	// Enable collision data
	mesh->ContainsPhysicsTriMeshData(true);
}

// Gets the mesh information for a given section of the chunk
// Note that any TArrays passed in will be overwritten
TArray<MeshData*> AChunk::GetMeshDataForChunk(int32 chunkI, int32 chunkJ, 
	int32 sectionCount, TArray<BlockType> blocks, int32 sectionSide)
{
	TArray<MeshData*> chunkMeshData; chunkMeshData.SetNum(sectionCount);

	// Get mesh data for all sections
	for (int32 section = 0; section < sectionCount; section++)
	{
		chunkMeshData[section] = GetMeshData(chunkI, chunkJ, section, sectionCount, blocks, sectionSide);
	}

	return chunkMeshData;
}

// Gets the mesh information for a given section of the chunk
// Note that any TArrays passed in will be overwritten
MeshData* AChunk::GetMeshData(int32 chunkI, int32 chunkJ, int32 sectionID, int32 sectionCount, TArray<BlockType> blocks, int32 sectionSide)
{
	MeshData* result = new MeshData();

	if (sectionID >= sectionCount)
	{
		UE_LOG(LogTemp, Error, TEXT("Attempting to GetMeshData for %d but the max count is %d"), sectionID, sectionCount);
		return result;
	}

	// Clear the arrays
	result->vertices.Empty(); result->Triangles.Empty(); result->normals.Empty();
	result->UV0.Empty(); result->tangents.Empty(); result->vertexColors.Empty();

	result->sectionCount = sectionCount; result->sectionSide = sectionSide;
	result->blocks = blocks;
	result->chunkI = chunkI; result->chunkJ = chunkJ;

	int initialI = sectionID * sectionSide, lastI = (sectionID + 1) * sectionSide;

	for (int i = initialI; i < lastI; i++)
	{
		for (int j = 0; j < sectionSide; j++)
		{
			for (int k = 0; k < sectionSide; k++)
			{
				if (blocks[GetPositionInTArray(i,j,k,sectionSide)] == BlockType::AIR) continue;
				for (int d = 0; d < MeshData::Direction::SIZE; d++)
				{
					if (CheckIfNeighboorIsAir(MeshData::Direction(d), blocks, i, j, k, sectionCount, sectionSide, *result))
					{
						AddVoxelFace(MeshData::Direction(d), blocks[GetPositionInTArray(i, j, k, sectionSide)],
							result, i, j, k);
					}
				}
			}
		}
	}

	return result;
}

bool AChunk::CheckIfNeighboorIsAir(MeshData::Direction direction, TArray<BlockType>& blocks, int i, int j, int k,
	int sectionCount, int sectionSide, MeshData& data)
{
	FVector offset = data.NORMALS[direction];
	int newI = i + offset.Z, newJ = j + offset.Y, newK = k + offset.X;

	// TODO: Handle interchunk check
	if (newI >= sectionCount * sectionSide || newI < 0) return false;
	if (newJ >= sectionSide || newJ < 0) return false;
	if (newK >= sectionSide || newK < 0) return false;

	//UE_LOG(LogTemp, Log, TEXT("%d %d %d -> %d %d %d: %d: %s"),
	//	i, j, k, newI, newJ, newK,
	//	direction,
	//	(blocks[GetPositionInTArray(newI, newJ, newK)] == BlockType::AIR) ? TEXT("TRUE") : TEXT("FALSE"))

	return blocks[GetPositionInTArray(newI,newJ,newK, sectionSide)] == BlockType::AIR;
}

void AChunk::AddVoxel(FVector insidePoint, BlockType blockTypeToAdd)
{
	int k = int(insidePoint.X) / BlockSize;
	int j = int(insidePoint.Y) / BlockSize;
	int i = int(insidePoint.Z) / BlockSize;

	int positionInTArray = GetPositionInTArray(i, j, k, mSectionSide);

	// TODO: Handle interchunk check
	if (i >= mSectionCount * mSectionSide || i < 0) return;
	if (j >= mSectionSide || j < 0) return;
	if (k >= mSectionSide || k < 0) return;

	UE_LOG(LogTemp, Log, TEXT("Blocks Size %d %d %d"), mBlocks.Num(), &mBlocks, this);

	mBlocks[positionInTArray] = blockTypeToAdd;

	// TODO: Handle interchunk compute
	// Reconstruct the current section
	int32 section = i / mSectionSide;

	// Check if section above or below needs update (current i is right at the edge)
	if (i % mSectionSide == 0 || (i + 1) % mSectionSide == 0)
	{
		int32 extraSection = i % mSectionSide == 0 ? section - 1 : section + 1;

		MeshData* d = GetMeshData(0, 0, extraSection, mSectionCount, mBlocks, mSectionSide);
		mesh->ClearMeshSection(extraSection);
		mesh->CreateMeshSection_LinearColor(extraSection, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);

		delete(d);

		UE_LOG(LogTemp, Log, TEXT("Extra section updated i: %d, extraSection: %d"), i, extraSection);
	}

	MeshData* d = GetMeshData(0, 0, section, mSectionCount, mBlocks, mSectionSide);
	mesh->ClearMeshSection(section);
	mesh->CreateMeshSection_LinearColor(section, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);
	delete(d);
}

void AChunk::RemoveVoxel(FVector insidePoint)
{
	int k = int(insidePoint.X) / BlockSize;
	int j = int(insidePoint.Y) / BlockSize;
	int i = int(insidePoint.Z) / BlockSize;

	int positionInTArray = GetPositionInTArray(i, j, k, mSectionSide);
	// TODO: Handle interchunk check
	if (i >= mSectionCount*mSectionSide || i < 0) return;
	if (j >= mSectionSide || j < 0) return;
	if (k >= mSectionSide || k < 0) return;

	//UE_LOG(LogTemp, Log, TEXT("Blocks Size %d %d %d"), mBlocks.Num(), &mBlocks, this);

	mBlocks[positionInTArray] = BlockType::AIR;

	// TODO: Handle interchunk compute
	// Reconstruct the current section
	int32 section = i / mSectionSide;

	UE_LOG(LogTemp, Log, TEXT("Removing voxel %d %d %d, at section: %d"), i, j, k, section)

	// Check if section above or below needs update (current i is right at the edge)
	if (i % mSectionSide == 0 || (i + 1) % mSectionSide == 0)
	{
		int32 extraSection = i % mSectionSide == 0 ? section - 1 : section + 1;

		MeshData* d = GetMeshData(0,0, extraSection, mSectionCount, mBlocks, mSectionSide);
		mesh->ClearMeshSection(extraSection);
		mesh->CreateMeshSection_LinearColor(extraSection, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);

		delete(d);

		UE_LOG(LogTemp, Log, TEXT("Extra section updated i: %d, extraSection: %d"), i, extraSection);
	}

	MeshData* d = GetMeshData(0,0,section, mSectionCount, mBlocks, mSectionSide);
	mesh->ClearMeshSection(section);
	mesh->CreateMeshSection_LinearColor(section, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);

	delete(d);
}

void AChunk::AddVoxelFace(MeshData::Direction direction,
	BlockType currentBlockType,
	MeshData* data,
	int i, int j, int k)
{
	FDateTime start = FDateTime::UtcNow();

	const int numVertices = 4;
	int lastNumVertices = data->vertices.Num();
	FVector position = FVector(k * BlockSize, j * BlockSize, i * BlockSize);

	// VERTICES
	for (int v = 0; v < numVertices; v++) data->vertices.Add(data->VERTICES[direction][v]*100 + position);

	// TRIANGLES
	data->Triangles.Add(lastNumVertices + 0);
	data->Triangles.Add(lastNumVertices + 1);
	data->Triangles.Add(lastNumVertices + 2);
	data->Triangles.Add(lastNumVertices + 3);
	data->Triangles.Add(lastNumVertices + 2);
	data->Triangles.Add(lastNumVertices + 1);

	// NORMALS
	for (int v = 0; v < numVertices; v++) data->normals.Add(data->NORMALS[direction]);

	auto getUVIndices = [data](int32 index, int32& outUVI, int32& outUVJ)
	{
		outUVI = index / data->ATLAS_SIZE;
		outUVJ = index % data->ATLAS_SIZE;
	};

	// UVI -> rows -> V, UVJ -> columns -> U
	int UVI = 0;
	int UVJ = 0;
	float UVSize = 1.0f / data->ATLAS_SIZE;
	switch (direction)
	{
	case MeshData::UP:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][0], UVI, UVJ);
		break;
	case MeshData::DOWN:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][2], UVI, UVJ);
		break;
	case MeshData::RIGHT:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][1], UVI, UVJ);
		break;
	case MeshData::LEFT:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][1], UVI, UVJ);
		break;
	case MeshData::FORWARD:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][1], UVI, UVJ);
		break;
	case MeshData::BACK:
		getUVIndices(data->BlockTypeTextureIndex[int(currentBlockType)][1], UVI, UVJ);
		break;
	}
		
	// UVs
	// TODO: Make the texture mapping to use RHI
	// TODO: Make the lighting more efficient
	data->UV0.Add(data->UVS_Inverted[direction][0] * UVSize + FVector2D(UVJ * UVSize, UVI * UVSize));
	data->UV0.Add(data->UVS_Inverted[direction][1] * UVSize + FVector2D(UVJ * UVSize, UVI * UVSize));
	data->UV0.Add(data->UVS_Inverted[direction][2] * UVSize + FVector2D(UVJ * UVSize, UVI * UVSize));
	data->UV0.Add(data->UVS_Inverted[direction][3] * UVSize + FVector2D(UVJ * UVSize, UVI * UVSize));

	//TANGENTS
	for (int v = 0; v < numVertices; v++) data->tangents.Add(data->TANGENTS[direction]);

	// VERTEX COLOR
	data->vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	data->vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	data->vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	data->vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
}

// Given an i, j, k position, return the position in the resulting block array
int AChunk::GetPositionInTArray(int i, int j, int k, int sectionSide)
{
	return i * sectionSide*sectionSide + j * sectionSide + k;
}

// Given the position in the block array, return its corresponding i, j, k position
void AChunk::GetIJKFromPositionInTArray(int pos, int sectionSide, int& i, int& j, int& k)
{
	i = pos / (sectionSide * sectionSide);
	j = (pos % (sectionSide * sectionSide)) / sectionSide;
	k = (pos % (sectionSide * sectionSide)) % sectionSide;
}

void AChunk::CreateTriangle()
{
	TArray<FVector> vertices;
	vertices.Add(FVector(0, 0, 0));
	vertices.Add(FVector(0, 100, 0));
	vertices.Add(FVector(0, 0, 100));

	TArray<int32> Triangles;
	Triangles.Add(0);
	Triangles.Add(1);
	Triangles.Add(2);

	TArray<FVector> normals;
	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));
	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	UV0.Add(FVector2D(0, 0));
	UV0.Add(FVector2D(1, 0));
	UV0.Add(FVector2D(0, 1));


	TArray<FProcMeshTangent> tangents;
	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));
	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	mesh->CreateMeshSection_LinearColor(0, vertices, Triangles, normals, UV0, vertexColors, tangents, true);

	// Enable collision data
	mesh->ContainsPhysicsTriMeshData(true);
}

// Called when the game starts or when spawned
void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

TArray<MeshData*> AChunk::GetMeshDataForChunk(int32 ChunkX, int32 ChunkY, 
	TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock)
{
	int dummy;
	TArray<BlockType> blocks = AChunk::GenerateChunkData(ChunkX, ChunkY, 16, 16, dummy, dummy, PopulateBlock);

	return AChunk::GetMeshDataForChunk(ChunkX, ChunkY, 16, blocks, 16);
}

void AChunk::CreateChunk(int32 ChunkX, int32 ChunkY, UWorld* World)
{
	//FVector location = FVector(ChunkX * 1600, ChunkY * 1600, -400);
	//FRotator rotation = FRotator();
	//const FTransform transform = FTransform(location);

	//if (GEngine)
	//{
	//	const int32 AlwaysAddKey = -1; // Passing -1 means that we will not try and overwrite an   
	//								   // existing message, just add a new one  
	//	if (World)
	//	{
	//		AChunk* const newChunk = World->SpawnActor<AChunk>(AChunk::StaticClass(), transform);

	//		TArray<BlockType> blocks; int32 dummy;
	//		newChunk->GenerateChunkData(16, 16, 100, blocks, dummy, dummy);
	//		newChunk->CreateVoxelChunk(blocks, 16, 16);
	//	}

	//	GEngine->AddOnScreenDebugMessage(AlwaysAddKey, 0.5f, FColor::Yellow, TEXT("Chunk created"));

	//	//AChunk::ChunkMap.Add(FVector2D(ChunkX, ChunkY), newChunk);
	//}
}

TMap<int32, AChunk*> AChunk::ChunkMap;
TMap<int32, TFuture<TArray<MeshData*>>*> AChunk::chunkResults;
void AChunk::CreateChunk(int32 ChunkX, int32 ChunkY, UWorld* World, TArray<MeshData*> chunkData)
{
	FVector location = FVector(ChunkX * 1600, ChunkY * 1600, -1000);
	FRotator rotation = FRotator();
	const FTransform transform = FTransform(location);

	if (GEngine)
	{
		const int32 AlwaysAddKey = -1; // Passing -1 means that we will not try and overwrite an   
									   // existing message, just add a new one  
		if (World)
		{
			AChunk* const newChunk = World->SpawnActor<AChunk>(AChunk::StaticClass(), transform);

			newChunk->mBlocks = TArray<BlockType>(chunkData[0]->blocks);
			newChunk->mSectionSide = chunkData[0]->sectionSide; newChunk->mSectionCount = chunkData[0]->sectionCount;

			// Generate the mesh data by sections, each of sectionSide*sectionSide, start at the bottom
			for (int32 section = 0; section < 16; section++)
			{
				MeshData* d = chunkData[section];

				// Create the section
				newChunk->mesh->CreateMeshSection_LinearColor(section, d->vertices, d->Triangles, d->normals, d->UV0, d->vertexColors, d->tangents, true);

				if(chunkData[section]) delete(chunkData[section]); 
			}

			chunkData.Empty();

			// Enable collision data
			newChunk->mesh->ContainsPhysicsTriMeshData(true);

			AChunk::ChunkMap.Add(GetHashFromChunkPosition(ChunkX, ChunkY), newChunk);
		}

		GEngine->AddOnScreenDebugMessage(AlwaysAddKey, 20.0f, FColor::Yellow, FString::Printf(TEXT("Chunk created %d %d %f %f"), ChunkX, ChunkY, location.X, location.Y));
	}
}

void AChunk::PlayerMovedToAnotherChunk(int newChunkX, int newChunkY, TQueue<TArray<MeshData*>> &chunkLoaderQueue, 
	TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock,
	int32 ChunkRenderDistance)
{
	// Go through all the chunks in the ChunkMap, remove the extra chunks
	// TODO: put this back to unload chunks
	//TArray<int32> keys;
	//AChunk::ChunkMap.GetKeys(keys);

	//for (FVector2D k : keys)
	//{
	//	if (abs(k.X - chunkX) > CHUNK_RENDER_DISTANCE || abs(k.Y - chunkY) > CHUNK_RENDER_DISTANCE)
	//		AChunk::ChunkMap.Remove(k);
	//}
	//AChunk::ChunkMap.Compact();

	TFunction<void(int32 i, int32 j, TFunction <BlockType(int32 i, int32 j, int32 k)>)> LoadChunk = [&chunkLoaderQueue](int32 i, int32 j,
		TFunction <BlockType(int32 i, int32 j, int32 k)> PopulateBlock)
	{
		int hash = GetHashFromChunkPosition(i, j);
		if (ChunkMap.Contains(hash) || chunkResults.Contains(hash)) return;

		// Create the chunk
		TFunction<void()> ChunkCallback = [i, j, renderer = &chunkLoaderQueue]()
		{
			TFuture<TArray<MeshData*>>** result = chunkResults.Find(GetHashFromChunkPosition(i, j));
			chunkResults.Remove(GetHashFromChunkPosition(i, j));

			if (result)
			{
				renderer->Enqueue((*result)->Get());
			}
		};
		TFunction<TArray<MeshData*>()> ChunkTask = [i, j, PopulateBlock]()
		{
			return GetMeshDataForChunk(i, j, PopulateBlock);
		};

		TFuture<TArray<MeshData*>>* t = new TFuture<TArray<MeshData*>>(Async(EAsyncExecution::ThreadPool, ChunkTask, ChunkCallback));
		chunkResults.Add(GetHashFromChunkPosition(i, j), t);
	};

	// TODO: Use a heap to give priority to the chunks closest to the player

	// Go through all the chunks that need to be rendered, figure out which need to be added to the chunkmap
	for (int i = newChunkX - ChunkRenderDistance; i <= newChunkX + ChunkRenderDistance; i++)
	{
		for (int j = newChunkY - ChunkRenderDistance; j <= newChunkY + ChunkRenderDistance; j++)
		{
			if (i != 0 || j != 0)
				LoadChunk(i, j, PopulateBlock);
		}
	}
}

//TODO: Is this used??
void FChunkCreateTask::DoWork() {
	AChunk::CreateChunk(ChunkX, ChunkY, World);
}

