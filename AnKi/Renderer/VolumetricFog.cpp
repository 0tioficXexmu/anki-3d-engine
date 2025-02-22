// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/VolumetricFog.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/DepthDownscale.h>
#include <AnKi/Renderer/ShadowMapping.h>
#include <AnKi/Renderer/LightShading.h>
#include <AnKi/Renderer/RenderQueue.h>
#include <AnKi/Renderer/VolumetricLightingAccumulation.h>
#include <AnKi/Core/ConfigSet.h>

namespace anki {

Error VolumetricFog::init()
{
	// Misc
	const F32 qualityXY = ConfigSet::getSingleton().getRVolumetricLightingAccumulationQualityXY();
	const F32 qualityZ = ConfigSet::getSingleton().getRVolumetricLightingAccumulationQualityZ();
	m_finalZSplit = min(getRenderer().getZSplitCount() - 1,
						ConfigSet::getSingleton().getRVolumetricLightingAccumulationFinalZSplit());

	m_volumeSize[0] = U32(F32(getRenderer().getTileCounts().x()) * qualityXY);
	m_volumeSize[1] = U32(F32(getRenderer().getTileCounts().y()) * qualityXY);
	m_volumeSize[2] = U32(F32(m_finalZSplit + 1) * qualityZ);

	ANKI_R_LOGV("Initializing volumetric fog. Resolution %ux%ux%u", m_volumeSize[0], m_volumeSize[1], m_volumeSize[2]);

	// Shaders
	ANKI_CHECK(
		ResourceManager::getSingleton().loadResource("ShaderBinaries/VolumetricFogAccumulation.ankiprogbin", m_prog));

	ShaderProgramResourceVariantInitInfo variantInitInfo(m_prog);
	const ShaderProgramResourceVariant* variant;
	m_prog->getOrCreateVariant(variantInitInfo, variant);
	m_grProg = variant->getProgram();
	m_workgroupSize[0] = variant->getWorkgroupSizes()[0];
	m_workgroupSize[1] = variant->getWorkgroupSizes()[1];

	// RT descr
	m_rtDescr = getRenderer().create2DRenderTargetDescription(m_volumeSize[0], m_volumeSize[1],
															  Format::kR16G16B16A16_Sfloat, "Fog");
	m_rtDescr.m_depth = m_volumeSize[2];
	m_rtDescr.m_type = TextureType::k3D;
	m_rtDescr.bake();

	return Error::kNone;
}

void VolumetricFog::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	m_runCtx.m_rt = rgraph.newRenderTarget(m_rtDescr);

	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("Vol fog");

	pass.newTextureDependency(m_runCtx.m_rt, TextureUsageBit::kImageComputeWrite);
	pass.newTextureDependency(getRenderer().getVolumetricLightingAccumulation().getRt(),
							  TextureUsageBit::kSampledCompute);

	pass.setWork([this, &ctx](RenderPassWorkContext& rgraphCtx) -> void {
		CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;

		cmdb->bindShaderProgram(m_grProg);

		cmdb->bindSampler(0, 0, getRenderer().getSamplers().m_trilinearClamp);
		rgraphCtx.bindColorTexture(0, 1, getRenderer().getVolumetricLightingAccumulation().getRt());

		rgraphCtx.bindImage(0, 2, m_runCtx.m_rt, TextureSubresourceInfo());

		VolumetricFogUniforms regs;
		const SkyboxQueueElement& el = ctx.m_renderQueue->m_skybox;
		regs.m_fogDiffuse = el.m_fog.m_diffuseColor;
		regs.m_fogScatteringCoeff = el.m_fog.m_scatteringCoeff;
		regs.m_fogAbsorptionCoeff = el.m_fog.m_absorptionCoeff;
		regs.m_near = ctx.m_renderQueue->m_cameraNear;
		regs.m_far = ctx.m_renderQueue->m_cameraFar;
		regs.m_zSplitCountf = F32(getRenderer().getZSplitCount());
		regs.m_volumeSize = UVec3(m_volumeSize);
		regs.m_maxZSplitsToProcessf = F32(m_finalZSplit + 1);

		cmdb->setPushConstants(&regs, sizeof(regs));

		dispatchPPCompute(cmdb, m_workgroupSize[0], m_workgroupSize[1], m_volumeSize[0], m_volumeSize[1]);
	});
}

} // end namespace anki
