#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Billboard.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Renderer/VertexArray.h"
#include "CoffeeEngine/Renderer/Material.h"
#include <glm/fwd.hpp>

namespace Coffee
{
    struct BillboardRenderCommand
    {
        Ref<Billboard> billboard;
        Ref<Material> material;
        uint32_t entityID;
    };
    static void DrawBillboard(
    const Ref<Billboard>& billboard, 
    const Ref<Texture2D>& texture, 
    const glm::vec4& color = glm::vec4(1.0f)
);
    class BillboardRenderer
    {
      public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition, const glm::vec3& cameraUp);
        static void EndScene();

        static void DrawBillboard(const Ref<Billboard>& billboard, const Ref<Texture2D>& texture, const glm::vec4& color = glm::vec4(1.0f));

      private:
        struct BillboardRendererData
        {
            Ref<VertexArray> QuadVertexArray;
            Ref<VertexBuffer> QuadVertexBuffer;
            Ref<Shader> BillboardShader;
            std::vector<BillboardRenderCommand> billboardQueue;

            glm::mat4 ViewProjection = glm::mat4(1.0f);
            glm::vec3 CameraPosition = {0.0f, 0.0f, 0.0f};
            glm::vec3 CameraUp = {0.0f, 1.0f, 0.0f};
        };
        static BillboardRendererData s_Data;

      public:
        /**
         * @brief Submits a billboard render command.
         * @param command The billboard render command.
         */
        static void Submit(const BillboardRenderCommand& command);

        /**
         * @brief Submits a billboard with the specified material and entity ID.
         * @param billboard The billboard.
         * @param material The material.
         * @param entityID The entity ID.
         */
        static void Submit(const Ref<Billboard>& billboard, const Ref<Material>& material, uint32_t entityID = 4294967295);
    };

} // namespace Coffee