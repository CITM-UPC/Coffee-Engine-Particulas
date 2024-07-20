#include "Framebuffer.h"
#include "TeaEngine/Renderer/Texture.h"

#include <glad/glad.h>

namespace Tea {

    static const uint32_t s_MaxFramebufferSize = 8192;

    Framebuffer::Framebuffer(uint32_t width, uint32_t height, std::initializer_list<ImageFormat> attachments)
        : m_Width(width), m_Height(height), m_Attachments(attachments)
    {
        glCreateFramebuffers(1, &m_fboID);

        Invalidate();
    }

    Framebuffer::~Framebuffer()
    {
        glDeleteFramebuffers(1, &m_fboID);
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height)
    {
        if(width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
        {
            TEA_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
            return;
        }

        m_Width = width;
        m_Height = height;

        /* for (auto& texture : m_ColorTextures)
        {
            texture->SetData(nullptr, m_Width * m_Height * 4);
        }

        m_DepthTexture->SetData(nullptr, m_Width * m_Height * 4); */

        Invalidate();
    }

    void Framebuffer::Invalidate()
    {
        if(m_fboID)
        {
            glDeleteFramebuffers(1, &m_fboID);
            for (auto& texture : m_ColorTextures)
            {
                if(texture != nullptr)
                {
                    delete texture;
                }
            }
            if(m_DepthTexture != nullptr)
            {
                delete m_DepthTexture;
            }

            glCreateFramebuffers(1, &m_fboID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
            
            for (size_t i = 0; i < m_Attachments.size(); i++)
            {
                ImageFormat imageFormat = m_Attachments[i];

                if(imageFormat == ImageFormat::DEPTH24STENCIL8)
                {
                    //m_DepthTexture = new Texture(m_Width, m_Height, imageFormat);
                    Texture* depthTexture = new Texture(m_Width, m_Height, imageFormat);
                    AttachDepthTexture(depthTexture);
                }
                else
                {
                    //m_ColorTextures.push_back(new Texture(m_Width, m_Height, imageFormat));
                    Texture* colorTexture = new Texture(m_Width, m_Height, imageFormat);
                    AttachColorTexture(colorTexture);
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void Framebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    }

    void Framebuffer::UnBind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::AttachColorTexture(Texture* texture)
    {
        m_ColorTextures.push_back(texture);
        glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT0 + m_ColorTextures.size() - 1, texture->GetID(), 0);
    }

    void Framebuffer::AttachDepthTexture(Texture* texture)
    {
        m_DepthTexture = texture;
        glNamedFramebufferTexture(m_fboID, GL_DEPTH_STENCIL_ATTACHMENT, texture->GetID(), 0);
    }

    Ref<Framebuffer> Framebuffer::Create(uint32_t width, uint32_t height, std::initializer_list<ImageFormat> attachments)
    {
        return CreateRef<Framebuffer>(width, height, attachments);
    }

}