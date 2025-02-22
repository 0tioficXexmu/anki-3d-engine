// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/MotionVectors.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Renderer/RenderQueue.h>
#include <AnKi/Core/ConfigSet.h>

namespace anki {

Error MotionVectors::init()
{
	const Error err = initInternal();
	if(err)
	{
		ANKI_R_LOGE("Failed to initialize motion vectors");
	}
	return err;
}

Error MotionVectors::initInternal()
{
	ANKI_R_LOGV("Initializing motion vectors");

	// Prog
	ANKI_CHECK(ResourceManager::getSingleton().loadResource((ConfigSet::getSingleton().getRPreferCompute())
																? "ShaderBinaries/MotionVectorsCompute.ankiprogbin"
																: "ShaderBinaries/MotionVectorsRaster.ankiprogbin",
															m_prog));
	ShaderProgramResourceVariantInitInfo variantInitInfo(m_prog);
	variantInitInfo.addConstant("kFramebufferSize", UVec2(getRenderer().getInternalResolution().x(),
														  getRenderer().getInternalResolution().y()));
	const ShaderProgramResourceVariant* variant;
	m_prog->getOrCreateVariant(variantInitInfo, variant);
	m_grProg = variant->getProgram();

	// RTs
	m_motionVectorsRtDescr = getRenderer().create2DRenderTargetDescription(getRenderer().getInternalResolution().x(),
																		   getRenderer().getInternalResolution().y(),
																		   Format::kR16G16_Sfloat, "MotionVectors");
	m_motionVectorsRtDescr.bake();

	TextureUsageBit historyLengthUsage = TextureUsageBit::kAllSampled;
	if(ConfigSet::getSingleton().getRPreferCompute())
	{
		historyLengthUsage |= TextureUsageBit::kImageComputeWrite;
	}
	else
	{
		historyLengthUsage |= TextureUsageBit::kFramebufferWrite;
	}

	TextureInitInfo historyLengthTexInit = getRenderer().create2DRenderTargetInitInfo(
		getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y(), Format::kR8_Unorm,
		historyLengthUsage, "MotionVectorsHistoryLen#1");
	m_historyLengthTextures[0] =
		getRenderer().createAndClearRenderTarget(historyLengthTexInit, TextureUsageBit::kAllSampled);
	historyLengthTexInit.setName("MotionVectorsHistoryLen#2");
	m_historyLengthTextures[1] =
		getRenderer().createAndClearRenderTarget(historyLengthTexInit, TextureUsageBit::kAllSampled);

	m_fbDescr.m_colorAttachmentCount = 2;
	m_fbDescr.bake();

	return Error::kNone;
}

void MotionVectors::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	m_runCtx.m_motionVectorsRtHandle = rgraph.newRenderTarget(m_motionVectorsRtDescr);

	const U32 writeHistoryLenTexIdx = getRenderer().getFrameCount() & 1;
	const U32 readHistoryLenTexIdx = !writeHistoryLenTexIdx;

	if(m_historyLengthTexturesImportedOnce) [[likely]]
	{
		m_runCtx.m_historyLengthWriteRtHandle =
			rgraph.importRenderTarget(m_historyLengthTextures[writeHistoryLenTexIdx]);
		m_runCtx.m_historyLengthReadRtHandle = rgraph.importRenderTarget(m_historyLengthTextures[readHistoryLenTexIdx]);
	}
	else
	{
		m_runCtx.m_historyLengthWriteRtHandle =
			rgraph.importRenderTarget(m_historyLengthTextures[writeHistoryLenTexIdx], TextureUsageBit::kAllSampled);
		m_runCtx.m_historyLengthReadRtHandle =
			rgraph.importRenderTarget(m_historyLengthTextures[readHistoryLenTexIdx], TextureUsageBit::kAllSampled);

		m_historyLengthTexturesImportedOnce = true;
	}

	RenderPassDescriptionBase* ppass;
	TextureUsageBit readUsage;
	TextureUsageBit writeUsage;
	if(ConfigSet::getSingleton().getRPreferCompute())
	{
		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("MotionVectors");

		readUsage = TextureUsageBit::kSampledCompute;
		writeUsage = TextureUsageBit::kImageComputeWrite;
		ppass = &pass;
	}
	else
	{
		GraphicsRenderPassDescription& pass = rgraph.newGraphicsRenderPass("MotionVectors");
		pass.setFramebufferInfo(m_fbDescr, {m_runCtx.m_motionVectorsRtHandle, m_runCtx.m_historyLengthWriteRtHandle});

		readUsage = TextureUsageBit::kSampledFragment;
		writeUsage = TextureUsageBit::kFramebufferWrite;
		ppass = &pass;
	}

	ppass->setWork([this, &ctx](RenderPassWorkContext& rgraphCtx) -> void {
		run(ctx, rgraphCtx);
	});

	ppass->newTextureDependency(m_runCtx.m_motionVectorsRtHandle, writeUsage);
	ppass->newTextureDependency(m_runCtx.m_historyLengthWriteRtHandle, writeUsage);
	ppass->newTextureDependency(m_runCtx.m_historyLengthReadRtHandle, readUsage);
	ppass->newTextureDependency(getRenderer().getGBuffer().getColorRt(3), readUsage);
	ppass->newTextureDependency(getRenderer().getGBuffer().getDepthRt(), readUsage);
	ppass->newTextureDependency(getRenderer().getGBuffer().getPreviousFrameDepthRt(), readUsage);
}

void MotionVectors::run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx)
{
	CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;

	cmdb->bindShaderProgram(m_grProg);

	cmdb->bindSampler(0, 0, getRenderer().getSamplers().m_trilinearClamp);
	rgraphCtx.bindTexture(0, 1, getRenderer().getGBuffer().getDepthRt(),
						  TextureSubresourceInfo(DepthStencilAspectBit::kDepth));
	rgraphCtx.bindTexture(0, 2, getRenderer().getGBuffer().getPreviousFrameDepthRt(),
						  TextureSubresourceInfo(DepthStencilAspectBit::kDepth));
	rgraphCtx.bindColorTexture(0, 3, getRenderer().getGBuffer().getColorRt(3));
	rgraphCtx.bindColorTexture(0, 4, m_runCtx.m_historyLengthReadRtHandle);

	class Uniforms
	{
	public:
		Mat4 m_reprojectionMat;
		Mat4 m_viewProjectionInvMat;
		Mat4 m_prevViewProjectionInvMat;
	} * pc;
	pc = allocateAndBindUniforms<Uniforms*>(sizeof(*pc), cmdb, 0, 5);

	pc->m_reprojectionMat = ctx.m_matrices.m_reprojection;
	pc->m_viewProjectionInvMat = ctx.m_matrices.m_invertedViewProjectionJitter;
	pc->m_prevViewProjectionInvMat = ctx.m_prevMatrices.m_invertedViewProjectionJitter;

	if(ConfigSet::getSingleton().getRPreferCompute())
	{
		rgraphCtx.bindImage(0, 6, m_runCtx.m_motionVectorsRtHandle, TextureSubresourceInfo());
		rgraphCtx.bindImage(0, 7, m_runCtx.m_historyLengthWriteRtHandle, TextureSubresourceInfo());
	}

	if(ConfigSet::getSingleton().getRPreferCompute())
	{
		dispatchPPCompute(cmdb, 8, 8, getRenderer().getInternalResolution().x(),
						  getRenderer().getInternalResolution().y());
	}
	else
	{
		cmdb->setViewport(0, 0, getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y());

		cmdb->drawArrays(PrimitiveTopology::kTriangles, 3);
	}
}

} // end namespace anki
