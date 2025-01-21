#include "CoffeeEngine/Renderer/BillboardRenderer.h"
#include "CoffeeEngine/Renderer/Buffer.h"
#include <tracy/Tracy.hpp>

namespace Coffee
{
    BillboardRenderer::BillboardRendererData BillboardRenderer::s_Data;

    void BillboardRenderer::Init()
    {
        ZoneScoped;

        // Create quad vertices
        float vertices[] = {// positions        // texture coords
                            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 
                            0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 
                            -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};

        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        s_Data.QuadVertexArray = VertexArray::Create();
        s_Data.QuadVertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));

        BufferLayout layout = {{ShaderDataType::Vec3, "a_Position"}, {ShaderDataType::Vec2, "a_TexCoord"}};

        s_Data.QuadVertexBuffer->SetLayout(layout);
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
        s_Data.QuadVertexArray->SetIndexBuffer(indexBuffer);

        if (!std::filesystem::exists("assets/shaders/Billboard.glsl"))
        {
            COFFEE_CORE_ERROR("Shader file not found: assets/shaders/Billboard.glsl");
        }
        s_Data.BillboardShader = Shader::Create("assets/shaders/Billboard.glsl");
    }

    void BillboardRenderer::Shutdown()
    {
        ZoneScoped;
        s_Data.QuadVertexArray.reset();
        s_Data.QuadVertexBuffer.reset();
        s_Data.BillboardShader.reset();
    }

    void BillboardRenderer::BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition, const glm::vec3& cameraUp)
    {
        ZoneScoped;
        s_Data.ViewProjection = viewProjection;
        s_Data.CameraPosition = cameraPosition;
        s_Data.CameraUp = cameraUp;
    }

    void BillboardRenderer::EndScene()
    {
        ZoneScoped;

        for (const auto& command : s_Data.billboardQueue)
        {
            Material* material = command.material.get();
            if (material == nullptr)
            {
                // There should be access to the default material here
                continue;
            }

            material->Use();
            const Ref<Shader>& shader = material->GetShader();
            shader->Bind();

            glm::mat4 transform = command.billboard->CalculateTransform(s_Data.CameraPosition, s_Data.CameraUp);

            shader->setMat4("model", transform);
            shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(transform))));

            // Conversión de entityID
            uint32_t r = (command.entityID & 0x000000FF) >> 0;
            uint32_t g = (command.entityID & 0x0000FF00) >> 8;
            uint32_t b = (command.entityID & 0x00FF0000) >> 16;
            glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
            shader->setVec3("entityID", entityIDVec3);

            s_Data.QuadVertexArray->Bind();
            glDrawElements(GL_TRIANGLES, s_Data.QuadVertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
            s_Data.QuadVertexArray->Unbind();
        }

        s_Data.billboardQueue.clear();
    }

    void BillboardRenderer::DrawBillboard(const Ref<Billboard>& billboard, const Ref<Texture2D>& texture, const glm::vec4& color)
    {
        ZoneScoped;

        s_Data.BillboardShader->Bind();
        s_Data.BillboardShader->setMat4("u_ViewProjection", s_Data.ViewProjection);
        s_Data.BillboardShader->setMat4("u_Transform",
                                        billboard->CalculateTransform(s_Data.CameraPosition, s_Data.CameraUp));
        s_Data.BillboardShader->setVec4("u_Color", color);

        texture->Bind(0);
        s_Data.BillboardShader->setInt("u_Texture", 0);

        s_Data.QuadVertexArray->Bind();
        glDrawElements(GL_TRIANGLES, s_Data.QuadVertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        s_Data.QuadVertexArray->Unbind();
    }

    void BillboardRenderer::Submit(const BillboardRenderCommand& command)
    {
        s_Data.billboardQueue.push_back(command);
    }

    void BillboardRenderer::Submit(const Ref<Billboard>& billboard, const Ref<Material>& material, uint32_t entityID)
    {
        BillboardRenderCommand command;
        command.billboard = billboard;
        command.material = material;
        command.entityID = entityID;
        Submit(command);
    }

} // namespace Coffee