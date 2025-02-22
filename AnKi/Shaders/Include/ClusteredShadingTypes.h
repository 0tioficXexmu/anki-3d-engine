// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Shaders/Include/GpuSceneTypes.h>

#define ANKI_CLUSTERED_SHADING_USE_64BIT ANKI_SUPPORTS_64BIT_TYPES

ANKI_BEGIN_NAMESPACE

// Enum of clusterer object types
enum class ClusteredObjectType : U32
{
	kPointLight,
	kSpotLight,
	kDecal,
	kFogDensityVolume,
	kReflectionProbe,
	kGlobalIlluminationProbe,

	kCount,
	kFirst = 0
};
ANKI_ENUM_ALLOW_NUMERIC_OPERATIONS(ClusteredObjectType)

// Limits
#if ANKI_CLUSTERED_SHADING_USE_64BIT
constexpr U32 kMaxVisiblePointLights = 64u;
constexpr U32 kMaxVisibleSpotLights = 64u;
constexpr U32 kMaxVisibleDecals = 64u;
#else
constexpr U32 kMaxVisiblePointLights = 32u;
constexpr U32 kMaxVisibleSpotLights = 32u;
constexpr U32 kMaxVisibleDecals = 32u;
#endif
constexpr U32 kMaxVisibleFogDensityVolumes = 16u;
constexpr U32 kMaxVisibleReflectionProbes = 16u;
constexpr U32 kMaxVisibleGlobalIlluminationProbes = 8u;

// Other consts
constexpr RF32 kClusterObjectFrustumNearPlane = 0.1f / 4.0f; ///< Near plane of all clusterer object frustums.
constexpr RF32 kSubsurfaceMin = 0.01f;
constexpr U32 kMaxZsplitCount = 128u;

/// Point light.
struct PointLight
{
	Vec3 m_position; ///< Position in world space.
	RF32 m_radius; ///< Radius

	RVec3 m_diffuseColor;
	RF32 m_squareRadiusOverOne; ///< 1/(radius^2).

	Vec2 m_padding0;
	U32 m_shadowLayer; ///< Shadow layer used in RT shadows. Also used to show that it doesn't cast shadow.
	F32 m_shadowAtlasTileScale; ///< UV scale for all tiles.

	Vec4 m_shadowAtlasTileOffsets[6u]; ///< It's a array of Vec2 but because of padding round it up.
};
constexpr U32 kSizeof_PointLight = 9u * sizeof(Vec4);
static_assert(sizeof(PointLight) == kSizeof_PointLight);

/// Spot light.
struct SpotLight
{
	Vec3 m_position;
	F32 m_padding0;

	Vec4 m_edgePoints[4u]; ///< Edge points in world space.

	RVec3 m_diffuseColor;
	RF32 m_radius; ///< Max distance.

	RVec3 m_direction; ///< Light direction.
	RF32 m_squareRadiusOverOne; ///< 1/(radius^2).

	U32 m_shadowLayer; ///< Shadow layer used in RT shadows. Also used to show that it doesn't cast shadow.
	RF32 m_outerCos;
	RF32 m_innerCos;
	U32 m_padding1;

	Mat4 m_textureMatrix;
};
constexpr U32 kSizeof_SpotLight = 12u * sizeof(Vec4);
static_assert(sizeof(SpotLight) == kSizeof_SpotLight);

/// Spot light for binning. This is the same structure as SpotLight (same signature) but it's used for binning.
struct SpotLightBinning
{
	Vec4 m_edgePoints[5u]; ///< Edge points in world space. Point 0 is the eye pos.

	RVec3 m_diffuseColor;
	RF32 m_radius; ///< Max distance.

	RVec3 m_direction; ///< Light direction.
	RF32 m_squareRadiusOverOne; ///< 1/(radius^2).

	U32 m_shadowLayer; ///< Shadow layer used in RT shadows. Also used to show that it doesn't cast shadow.
	RF32 m_outerCos;
	RF32 m_innerCos;
	U32 m_padding0;

	Mat4 m_textureMatrix;
};
constexpr U32 kSizeof_SpotLightBinning = kSizeof_SpotLight;
static_assert(sizeof(SpotLightBinning) == kSizeof_SpotLightBinning);
static_assert(sizeof(SpotLight) == sizeof(SpotLightBinning));

/// Directional light (sun).
struct DirectionalLight
{
	RVec3 m_diffuseColor;
	U32 m_shadowCascadeCount; ///< If it's zero then it doesn't cast shadow.

	RVec3 m_direction;
	U32 m_active;

	Vec4 m_shadowCascadeDistances;

	Vec3 m_padding0;
	U32 m_shadowLayer; ///< Shadow layer used in RT shadows. Also used to show that it doesn't cast shadow.

	Mat4 m_textureMatrices[kMaxShadowCascades];
};
constexpr U32 kSizeof_DirectionalLight = 4u * sizeof(Vec4) + kMaxShadowCascades * sizeof(Mat4);
static_assert(sizeof(DirectionalLight) == kSizeof_DirectionalLight);
static_assert(kMaxShadowCascades == 4u); // Because m_shadowCascadeDistances is a Vec4

/// Representation of a reflection probe.
typedef GpuSceneReflectionProbe ReflectionProbe;

/// Decal.
typedef GpuSceneDecal Decal;

/// Fog density volume.
typedef GpuSceneFogDensityVolume FogDensityVolume;

/// Global illumination probe
typedef GpuSceneGlobalIlluminationProbe GlobalIlluminationProbe;

/// Common matrices.
struct CommonMatrices
{
	Mat3x4 m_cameraTransform;
	Mat3x4 m_view;
	Mat4 m_projection;
	Mat4 m_viewProjection;

	Mat4 m_jitter;
	Mat4 m_projectionJitter;
	Mat4 m_viewProjectionJitter;

	Mat4 m_invertedViewProjectionJitter; ///< To unproject in world space.
	Mat4 m_invertedViewProjection;
	Mat4 m_invertedProjectionJitter; ///< To unproject in view space.

	/// It's being used to reproject a clip space position of the current frame to the previous frame. Its value should
	/// be m_jitter * m_prevFrame.m_viewProjection * m_invertedViewProjectionJitter. At first it unprojects the current
	/// position to world space, all fine here. Then it projects to the previous frame as if the previous frame was
	/// using the current frame's jitter matrix.
	Mat4 m_reprojection;

	/// To unproject to view space. Jitter not considered.
	/// @code
	/// F32 z = m_unprojectionParameters.z / (m_unprojectionParameters.w + depth);
	/// Vec2 xy = ndc * m_unprojectionParameters.xy * z;
	/// pos = Vec3(xy, z);
	/// @endcode
	Vec4 m_unprojectionParameters;
};
constexpr U32 kSizeof_CommonMatrices = 43u * sizeof(Vec4);
static_assert(sizeof(CommonMatrices) == kSizeof_CommonMatrices);

/// Common uniforms for light shading passes.
struct ClusteredShadingUniforms
{
	Vec2 m_renderingSize;
	F32 m_time;
	U32 m_frame;

	Vec4 m_nearPlaneWSpace;

	Vec3 m_cameraPosition;
	F32 m_reflectionProbesMipCount;

	UVec2 m_tileCounts;
	U32 m_zSplitCount;
	F32 m_zSplitCountOverFrustumLength; ///< m_zSplitCount/(far-near)

	Vec2 m_zSplitMagic; ///< It's the "a" and "b" of computeZSplitClusterIndex(). See there for details.
	U32 m_tileSize;
	U32 m_lightVolumeLastZSplit;

	UVec2 m_padding0;
	F32 m_near;
	F32 m_far;

	DirectionalLight m_directionalLight;

	CommonMatrices m_matrices;
	CommonMatrices m_previousMatrices;

	/// This are some additive counts used to map a flat index to the index of the specific object
#if defined(__cplusplus)
	Array<UVec4, U32(ClusteredObjectType::kCount)> m_objectCountsUpTo;
#else
	UVec4 m_objectCountsUpTo[(U32)ClusteredObjectType::kCount];
#endif
};
constexpr U32 kSizeof_ClusteredShadingUniforms =
	(6u + (U32)ClusteredObjectType::kCount) * sizeof(Vec4) + 2u * sizeof(CommonMatrices) + sizeof(DirectionalLight);
static_assert(sizeof(ClusteredShadingUniforms) == kSizeof_ClusteredShadingUniforms);

// Define the type of some cluster object masks
#if ANKI_GLSL
#	if ANKI_CLUSTERED_SHADING_USE_64BIT
#		define ExtendedClusterObjectMask U64
#	else
#		define ExtendedClusterObjectMask U32
#	endif
#else
#	if ANKI_CLUSTERED_SHADING_USE_64BIT
typedef U64 ExtendedClusterObjectMask;
#	else
typedef U32 ExtendedClusterObjectMask;
#	endif
#endif

/// Information that a tile or a Z-split will contain.
struct Cluster
{
	ExtendedClusterObjectMask m_pointLightsMask;
	ExtendedClusterObjectMask m_spotLightsMask;
	ExtendedClusterObjectMask m_decalsMask;
	U32 m_fogDensityVolumesMask;
	U32 m_reflectionProbesMask;
	U32 m_giProbesMask;

	// Pad to 16byte
#if ANKI_CLUSTERED_SHADING_USE_64BIT
	U32 m_padding0;
	U32 m_padding1;
	U32 m_padding2;
#else
	U32 m_padding0;
	U32 m_padding1;
#endif
};

#if ANKI_CLUSTERED_SHADING_USE_64BIT
constexpr U32 kSizeof_Cluster = 3u * sizeof(Vec4);
static_assert(sizeof(Cluster) == kSizeof_Cluster);
#else
constexpr U32 kSizeof_Cluster = 2u * sizeof(Vec4);
static_assert(sizeof(Cluster) == kSizeof_Cluster);
#endif

constexpr ANKI_ARRAY(U32, ClusteredObjectType::kCount, kClusteredObjectSizes) = {
	sizeof(PointLight),       sizeof(SpotLight),       sizeof(Decal),
	sizeof(FogDensityVolume), sizeof(ReflectionProbe), sizeof(GlobalIlluminationProbe)};

constexpr ANKI_ARRAY(U32, ClusteredObjectType::kCount, kMaxVisibleClusteredObjects) = {
#if ANKI_CLUSTERED_SHADING_USE_64BIT
	64, 64, 64,
#else
	32, 32, 32,
#endif
	16, 16, 16};

ANKI_END_NAMESPACE
