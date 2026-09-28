// WorldHeightNoise.h
// Self-contained Perlin-based height sampler for UE5.
// Feed it world-space X/Y (e.g. from an Actor's location) and it returns a
// single height value — useful for procedural terrain, placing objects at
// "ground level" without a physical landscape, foliage scatter, etc.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldHeightNoise.generated.h"

USTRUCT(BlueprintType)
struct FWorldHeightNoiseSettings
{
	GENERATED_BODY()

	// Random seed. Same seed + same settings = same terrain every time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise")
	int32 Seed = 1337;

	// World units per noise "cell". Bigger = broader, smoother hills.
	// Smaller = tighter, bumpier terrain. Try 1000-5000 for landscape-scale.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise", meta = (ClampMin = "0.01"))
	float Scale = 2000.0f;

	// Number of noise layers summed together. More = finer detail, slower.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise", meta = (ClampMin = "1", ClampMax = "8"))
	int32 Octaves = 4;

	// Amplitude falloff per octave. Lower = smoother, higher = rougher/noisier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Persistence = 0.5f;

	// Frequency multiplier per octave. 2.0 is the standard choice.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise", meta = (ClampMin = "1.0"))
	float Lacunarity = 2.0f;

	// Final output is remapped to [BaseHeight - HeightScale, BaseHeight + HeightScale].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise")
	float HeightScale = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height Noise")
	float BaseHeight = 0.0f;
};

UCLASS()
class MINECRAFTCLONE_API UWorldHeightNoise : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns a single world-space height (Z) for the given world X/Y,
	 * remapped into [BaseHeight - HeightScale, BaseHeight + HeightScale].
	 */
	UFUNCTION(BlueprintCallable, Category = "Height Noise")
	static float GetHeightAtWorldLocation(float WorldX, float WorldY, const FWorldHeightNoiseSettings& Settings);

	/** Convenience overload taking a full world location; only X/Y are used. */
	UFUNCTION(BlueprintCallable, Category = "Height Noise")
	static float GetHeightAtLocation(const FVector& WorldLocation, const FWorldHeightNoiseSettings& Settings);

private:
	static float RawPerlin2D(float X, float Y, int32 Seed);
	static float FractalPerlin2D(float X, float Y, const FWorldHeightNoiseSettings& Settings);

	static float Fade(float T);
	static float Lerp(float T, float A, float B);
	static float Grad(int32 Hash, float X, float Y);
	static void BuildPermutation(int32 Seed, uint8 OutTable[512]);
};
