// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// Bins clusterer objects to the clusterer.
class ClusterBinning : public RendererObject
{
public:
	ClusterBinning();

	~ClusterBinning();

	Error init();

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	/// It will populate the clusters and the rest of the objects (lights, probes etc) in an async job. Needs to be
	/// called after the render queue is finalized.
	void writeClusterBuffersAsync();

	const RebarGpuMemoryToken& getClusteredUniformsRebarToken() const
	{
		return m_runCtx.m_clusteredShadingUniformsToken;
	}

	const RebarGpuMemoryToken& getClustersRebarToken() const
	{
		return m_runCtx.m_clustersToken;
	}

	const BufferHandle& getClustersRenderGraphHandle() const
	{
		return m_runCtx.m_rebarHandle;
	}

private:
	ShaderProgramResourcePtr m_prog;
	ShaderProgramPtr m_grProg;

	U32 m_tileCount = 0;
	U32 m_clusterCount = 0;

	class
	{
	public:
		RenderingContext* m_ctx = nullptr;
		RebarGpuMemoryToken m_clusteredShadingUniformsToken;
		RebarGpuMemoryToken m_clustersToken;
		BufferHandle m_rebarHandle; ///< For dependency tracking.
	} m_runCtx;

	void writeClustererBuffers(RenderingContext& ctx);
	void writeClustererBuffersTask();
};
/// @}

} // end namespace anki
