// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/Common.h>
#include <AnKi/Resource/Forward.h>
#include <AnKi/Renderer/Renderer.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// MainRenderer statistics.
class MainRendererStats
{
public:
	Second m_renderingCpuTime ANKI_DEBUG_CODE(= -1.0);
	Second m_renderingGpuTime ANKI_DEBUG_CODE(= -1.0);
	Second m_renderingGpuSubmitTimestamp ANKI_DEBUG_CODE(= -1.0);
};

class MainRendererInitInfo
{
public:
	UVec2 m_swapchainSize = UVec2(0u);

	AllocAlignedCallback m_allocCallback = nullptr;
	void* m_allocCallbackUserData = nullptr;
};

/// Main onscreen renderer
class MainRenderer : public MakeSingleton<MainRenderer>
{
	template<typename>
	friend class MakeSingleton;

public:
	Error init(const MainRendererInitInfo& inf);

	Error render(RenderQueue& rqueue, TexturePtr presentTex);

	Dbg& getDbg();

	F32 getAspectRatio() const;

	const Renderer& getOffscreenRenderer() const
	{
		return *m_r;
	}

	Renderer& getOffscreenRenderer()
	{
		return *m_r;
	}

	void setStatsEnabled(Bool enabled)
	{
		m_statsEnabled = enabled;
	}

	const MainRendererStats& getStats() const
	{
		return m_stats;
	}

private:
	StackMemoryPool m_framePool;

	Renderer* m_r = nullptr;
	Bool m_rDrawToDefaultFb = false;

	ShaderProgramResourcePtr m_blitProg;
	ShaderProgramPtr m_blitGrProg;

	UVec2 m_swapchainResolution = UVec2(0u);

	RenderGraphPtr m_rgraph;
	RenderTargetDescription m_tmpRtDesc;
	FramebufferDescription m_fbDescr;

	MainRendererStats m_stats;
	Bool m_statsEnabled = false;

	class
	{
	public:
		const RenderingContext* m_ctx = nullptr;
		Atomic<U32> m_secondaryTaskId = {0};
	} m_runCtx;

	MainRenderer();

	~MainRenderer();
};
/// @}

} // end namespace anki
