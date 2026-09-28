// WorldHeightNoise.cpp

#include "WorldHeightNoise.h"
#include "Math/RandomStream.h"

namespace
{
	// Cache the permutation table for the last-used seed so repeated calls
	// (e.g. querying height for many actors per tick) don't rebuild it every time.
	int32 CachedSeed = MIN_int32;
	uint8 CachedTable[512];
}

void UWorldHeightNoise::BuildPermutation(int32 Seed, uint8 OutTable[512])
{
	uint8 P[256];
	for (int32 i = 0; i < 256; ++i)
	{
		P[i] = static_cast<uint8>(i);
	}

	FRandomStream RandomStream(Seed);
	for (int32 i = 255; i > 0; --i)
	{
		const int32 j = RandomStream.RandRange(0, i);
		Swap(P[i], P[j]);
	}

	for (int32 i = 0; i < 256; ++i)
	{
		OutTable[i] = P[i];
		OutTable[i + 256] = P[i];
	}
}

float UWorldHeightNoise::Fade(float T)
{
	return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
}

float UWorldHeightNoise::Lerp(float T, float A, float B)
{
	return A + T * (B - A);
}

float UWorldHeightNoise::Grad(int32 Hash, float X, float Y)
{
	// 2D gradient: 8 directions from the low 3 bits of the hash.
	switch (Hash & 7)
	{
	case 0: return  X + Y;
	case 1: return  X - Y;
	case 2: return -X + Y;
	case 3: return -X - Y;
	case 4: return  X;
	case 5: return -X;
	case 6: return  Y;
	default: return -Y;
	}
}

float UWorldHeightNoise::RawPerlin2D(float X, float Y, int32 Seed)
{
	if (Seed != CachedSeed)
	{
		BuildPermutation(Seed, CachedTable);
		CachedSeed = Seed;
	}
	const uint8* P = CachedTable;

	const int32 XI = FMath::FloorToInt(X) & 255;
	const int32 YI = FMath::FloorToInt(Y) & 255;

	const float XF = X - FMath::FloorToFloat(X);
	const float YF = Y - FMath::FloorToFloat(Y);

	const float U = Fade(XF);
	const float V = Fade(YF);

	const int32 AA = P[P[XI] + YI];
	const int32 AB = P[P[XI] + YI + 1];
	const int32 BA = P[P[XI + 1] + YI];
	const int32 BB = P[P[XI + 1] + YI + 1];

	const float Res = Lerp(V,
		Lerp(U, Grad(AA, XF, YF),
			Grad(BA, XF - 1.0f, YF)),
		Lerp(U, Grad(AB, XF, YF - 1.0f),
			Grad(BB, XF - 1.0f, YF - 1.0f)));

	return Res; // roughly in [-1, 1]
}

float UWorldHeightNoise::FractalPerlin2D(float X, float Y, const FWorldHeightNoiseSettings& Settings)
{
	float Total = 0.0f;
	float Frequency = 1.0f;
	float Amplitude = 1.0f;
	float MaxValue = 0.0f;

	const int32 Octaves = FMath::Clamp(Settings.Octaves, 1, 8);

	for (int32 i = 0; i < Octaves; ++i)
	{
		Total += RawPerlin2D(X * Frequency, Y * Frequency, Settings.Seed) * Amplitude;
		MaxValue += Amplitude;
		Amplitude *= Settings.Persistence;
		Frequency *= Settings.Lacunarity;
	}

	return MaxValue > 0.0f ? Total / MaxValue : 0.0f; // roughly [-1, 1]
}

float UWorldHeightNoise::GetHeightAtWorldLocation(float WorldX, float WorldY, const FWorldHeightNoiseSettings& Settings)
{
	const float Scale = FMath::Max(Settings.Scale, KINDA_SMALL_NUMBER);

	const float NoiseX = WorldX / Scale;
	const float NoiseY = WorldY / Scale;

	const float Noise = FractalPerlin2D(NoiseX, NoiseY, Settings); // [-1, 1]

	return Settings.BaseHeight + Noise * Settings.HeightScale;
}

float UWorldHeightNoise::GetHeightAtLocation(const FVector& WorldLocation, const FWorldHeightNoiseSettings& Settings)
{
	return GetHeightAtWorldLocation(WorldLocation.X, WorldLocation.Y, Settings);
}