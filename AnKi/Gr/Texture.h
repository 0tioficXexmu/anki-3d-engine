// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/GrObject.h>

namespace anki {

/// @addtogroup graphics
/// @{

/// Texture initializer.
class alignas(4) TextureInitInfo : public GrBaseInitInfo
{
public:
	U32 m_width = 0;
	U32 m_height = 0;
	U32 m_depth = 1; ///< Relevant only for 3D textures.
	U32 m_layerCount = 1; ///< Relevant only for texture arrays.

	Format m_format = Format::kNone;

	TextureUsageBit m_usage = TextureUsageBit::kNone; ///< How the texture will be used.
	TextureType m_type = TextureType::k2D;

	U8 m_mipmapCount = 1;

	U8 m_samples = 1;

	U8 _m_padding[1] = {0};

	TextureInitInfo() = default;

	TextureInitInfo(CString name)
		: GrBaseInitInfo(name)
	{
	}

	U64 computeHash() const
	{
		const U8* first = reinterpret_cast<const U8*>(&m_width);
		const U8* last = reinterpret_cast<const U8*>(&m_samples) + sizeof(m_samples);
		const U size = U(last - first);
		ANKI_ASSERT(size
					== sizeof(m_width) + sizeof(m_height) + sizeof(m_depth) + sizeof(m_layerCount) + sizeof(m_format)
						   + sizeof(m_usage) + sizeof(m_type) + sizeof(m_mipmapCount) + sizeof(m_samples));
		return anki::computeHash(first, size);
	}

	Bool isValid() const
	{
#define ANKI_CHECK_VAL_VALIDITY(x) \
	do \
	{ \
		if(!(x)) \
		{ \
			return false; \
		} \
	} while(0)

		ANKI_CHECK_VAL_VALIDITY(m_format != Format::kNone);
		ANKI_CHECK_VAL_VALIDITY(m_usage != TextureUsageBit::kNone);
		ANKI_CHECK_VAL_VALIDITY(m_mipmapCount > 0);
		ANKI_CHECK_VAL_VALIDITY(m_width > 0);
		ANKI_CHECK_VAL_VALIDITY(m_height > 0);
		switch(m_type)
		{
		case TextureType::k2D:
			ANKI_CHECK_VAL_VALIDITY(m_depth == 1);
			ANKI_CHECK_VAL_VALIDITY(m_layerCount == 1);
			break;
		case TextureType::kCube:
			ANKI_CHECK_VAL_VALIDITY(m_depth == 1);
			ANKI_CHECK_VAL_VALIDITY(m_layerCount == 1);
			break;
		case TextureType::k3D:
			ANKI_CHECK_VAL_VALIDITY(m_depth > 0);
			ANKI_CHECK_VAL_VALIDITY(m_layerCount == 1);
			break;
		case TextureType::k2DArray:
		case TextureType::kCubeArray:
			ANKI_CHECK_VAL_VALIDITY(m_depth == 1);
			ANKI_CHECK_VAL_VALIDITY(m_layerCount > 0);
			break;
		default:
			ANKI_CHECK_VAL_VALIDITY(0);
		};

		return true;
#undef ANKI_CHECK_VAL_VALIDITY
	}
};

/// GPU texture.
class Texture : public GrObject
{
	ANKI_GR_OBJECT

public:
	static constexpr GrObjectType kClassType = GrObjectType::kTexture;

	U32 getWidth() const
	{
		ANKI_ASSERT(m_width);
		return m_width;
	}

	U32 getHeight() const
	{
		ANKI_ASSERT(m_height);
		return m_height;
	}

	U32 getDepth() const
	{
		ANKI_ASSERT(m_depth);
		return m_depth;
	}

	U32 getLayerCount() const
	{
		ANKI_ASSERT(m_layerCount);
		return m_layerCount;
	}

	U32 getMipmapCount() const
	{
		ANKI_ASSERT(m_mipCount);
		return m_mipCount;
	}

	TextureType getTextureType() const
	{
		ANKI_ASSERT(m_texType != TextureType::kCount);
		return m_texType;
	}

	TextureUsageBit getTextureUsage() const
	{
		ANKI_ASSERT(!!m_usage);
		return m_usage;
	}

	Format getFormat() const
	{
		ANKI_ASSERT(m_format != Format::kNone);
		return m_format;
	}

	DepthStencilAspectBit getDepthStencilAspect() const
	{
		return m_aspect;
	}

	Bool isSubresourceValid(const TextureSubresourceInfo& subresource) const
	{
#define ANKI_TEX_SUBRESOURCE_ASSERT(x_) \
	if(!(x_)) \
	{ \
		return false; \
	}
		const TextureType type = m_texType;
		const Bool cube = textureTypeIsCube(type);

		// Mips
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_mipmapCount > 0);
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_firstMipmap + subresource.m_mipmapCount <= m_mipCount);

		// Layers
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_layerCount > 0);
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_firstLayer + subresource.m_layerCount <= m_layerCount);

		// Faces
		const U8 faceCount = (cube) ? 6 : 1;
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_faceCount == 1 || subresource.m_faceCount == 6);
		ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_firstFace + subresource.m_faceCount <= faceCount);

		// Aspect
		ANKI_TEX_SUBRESOURCE_ASSERT((m_aspect & subresource.m_depthStencilAspect) == subresource.m_depthStencilAspect);

		// Misc
		if(type == TextureType::kCubeArray && subresource.m_layerCount > 1)
		{
			// Because of the way surfaces are arranged in cube arrays
			ANKI_TEX_SUBRESOURCE_ASSERT(subresource.m_faceCount == 6);
		}

#undef ANKI_TEX_SUBRESOURCE_ASSERT
		return true;
	}

	/// Mipmap generation requires a specific subresource range.
	Bool isSubresourceGoodForMipmapGeneration(const TextureSubresourceInfo& subresource) const
	{
		ANKI_ASSERT(isSubresourceValid(subresource));
		if(m_texType != TextureType::k3D)
		{
			return subresource.m_firstMipmap == 0 && subresource.m_mipmapCount == m_mipCount
				   && subresource.m_faceCount == 1 && subresource.m_layerCount == 1
				   && subresource.m_depthStencilAspect == m_aspect;
		}
		else
		{
			ANKI_ASSERT(!"TODO");
			return false;
		}
	}

	/// Return true if the subresource is good to be bound for image load store.
	Bool isSubresourceGoodForImageLoadStore(const TextureSubresourceInfo& subresource) const
	{
		ANKI_ASSERT(isSubresourceValid(subresource));
		// One mip and no depth stencil
		return subresource.m_mipmapCount == 1 && !subresource.m_depthStencilAspect;
	}

	/// Return true if the subresource can be bound for sampling.
	Bool isSubresourceGoodForSampling(const TextureSubresourceInfo& subresource) const
	{
		ANKI_ASSERT(isSubresourceValid(subresource));
		/// Can bound only one aspect at a time.
		return subresource.m_depthStencilAspect == DepthStencilAspectBit::kDepth
			   || subresource.m_depthStencilAspect == DepthStencilAspectBit::kStencil
			   || subresource.m_depthStencilAspect == DepthStencilAspectBit::kNone;
	}

	/// Return true if the subresource can be used in CommandBuffer::copyBufferToTextureView.
	Bool isSubresourceGoodForCopyFromBuffer(const TextureSubresourceInfo& subresource) const
	{
		ANKI_ASSERT(isSubresourceValid(subresource));
		return subresource.m_faceCount == 1 && subresource.m_mipmapCount == 1 && subresource.m_layerCount == 1
			   && subresource.m_depthStencilAspect == DepthStencilAspectBit::kNone;
	}

	/// Return true if the subresource can be used as Framebuffer attachment.
	Bool isSubresourceGoodForFramebufferAttachment(const TextureSubresourceInfo& subresource) const
	{
		ANKI_ASSERT(isSubresourceValid(subresource));
		return subresource.m_faceCount == 1 && subresource.m_mipmapCount == 1 && subresource.m_layerCount == 1;
	}

protected:
	U32 m_width = 0;
	U32 m_height = 0;
	U32 m_depth = 0;
	U32 m_layerCount = 0;
	U32 m_mipCount = 0;
	TextureType m_texType = TextureType::kCount;
	TextureUsageBit m_usage = TextureUsageBit::kNone;
	Format m_format = Format::kNone;
	DepthStencilAspectBit m_aspect = DepthStencilAspectBit::kNone;

	/// Construct.
	Texture(CString name)
		: GrObject(kClassType, name)
	{
	}

	/// Destroy.
	~Texture()
	{
	}

private:
	/// Allocate and initialize a new instance.
	[[nodiscard]] static Texture* newInstance(const TextureInitInfo& init);
};
/// @}

} // end namespace anki
