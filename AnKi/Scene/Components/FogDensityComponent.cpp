// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Scene/Components/FogDensityComponent.h>
#include <AnKi/Scene/SceneNode.h>
#include <AnKi/Scene/SceneGraph.h>

namespace anki {

FogDensityComponent::FogDensityComponent(SceneNode* node)
	: SceneComponent(node, getStaticClassId())
	, m_spatial(this)
{
	m_gpuSceneIndex = SceneGraph::getSingleton().getAllGpuSceneContiguousArrays().allocate(
		GpuSceneContiguousArrayType::kFogDensityVolumes);
}

FogDensityComponent ::~FogDensityComponent()
{
	m_spatial.removeFromOctree(SceneGraph::getSingleton().getOctree());

	SceneGraph::getSingleton().getAllGpuSceneContiguousArrays().deferredFree(
		GpuSceneContiguousArrayType::kFogDensityVolumes, m_gpuSceneIndex);
}

Error FogDensityComponent::update(SceneComponentUpdateInfo& info, Bool& updated)
{
	updated = m_dirty || info.m_node->movedThisFrame();

	if(updated)
	{
		m_dirty = false;

		m_worldPos = info.m_node->getWorldTransform().getOrigin().xyz();

		if(m_isBox)
		{
			const Aabb aabb(m_aabbMin + m_worldPos, m_aabbMax + m_worldPos);
			m_spatial.setBoundingShape(aabb);
		}
		else
		{
			const Sphere sphere(m_worldPos, m_sphereRadius);
			m_spatial.setBoundingShape(sphere);
		}

		// Upload to the GPU scene
		GpuSceneFogDensityVolume gpuVolume;
		if(m_isBox)
		{
			gpuVolume.m_aabbMinOrSphereCenter = m_aabbMin.xyz();
			gpuVolume.m_aabbMaxOrSphereRadiusSquared = m_aabbMax.xyz();
		}
		else
		{
			gpuVolume.m_aabbMaxOrSphereRadiusSquared = Vec3(m_sphereRadius * m_sphereRadius);
			gpuVolume.m_aabbMinOrSphereCenter = m_worldPos.xyz();
		}
		gpuVolume.m_isBox = m_isBox;
		gpuVolume.m_density = m_density;

		const PtrSize offset = m_gpuSceneIndex * sizeof(GpuSceneFogDensityVolume)
							   + SceneGraph::getSingleton().getAllGpuSceneContiguousArrays().getArrayBase(
								   GpuSceneContiguousArrayType::kFogDensityVolumes);
		GpuSceneMicroPatcher::getSingleton().newCopy(*info.m_framePool, offset, sizeof(gpuVolume), &gpuVolume);
	}

	const Bool spatialUpdated = m_spatial.update(SceneGraph::getSingleton().getOctree());
	updated = updated || spatialUpdated;

	return Error::kNone;
}

} // end namespace anki
