// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Shaders/Include/MiscRendererTypes.h>
#include <AnKi/Shaders/PackFunctions.hlsl>

constexpr F32 kRtShadowsMaxHistoryLength = 16.0;

UVec4 packRtShadows(RF32 shadowFactors[kMaxRtShadowLayers])
{
	const U32 a = newPackUnorm4x8(Vec4(shadowFactors[0], shadowFactors[1], shadowFactors[2], shadowFactors[3]));
	const U32 b = newPackUnorm4x8(Vec4(shadowFactors[4], shadowFactors[5], shadowFactors[6], shadowFactors[7]));
	return UVec4(a, b, 0, 0);
}

void unpackRtShadows(UVec4 packed, out RF32 shadowFactors[kMaxRtShadowLayers])
{
	const Vec4 a = newUnpackUnorm4x8(packed.x);
	const Vec4 b = newUnpackUnorm4x8(packed.y);
	shadowFactors[0] = a[0];
	shadowFactors[1] = a[1];
	shadowFactors[2] = a[2];
	shadowFactors[3] = a[3];
	shadowFactors[4] = b[0];
	shadowFactors[5] = b[1];
	shadowFactors[6] = b[2];
	shadowFactors[7] = b[3];
}

void zeroRtShadowLayers(out RF32 shadowFactors[kMaxRtShadowLayers])
{
	[unroll] for(U32 i = 0u; i < kMaxRtShadowLayers; ++i)
	{
		shadowFactors[i] = 0.0;
	}
}

struct [raypayload] RayPayload
{
	F32 m_shadowFactor : write(caller, anyhit, miss): read(caller);
};

struct Barycentrics
{
	Vec2 m_value;
};
