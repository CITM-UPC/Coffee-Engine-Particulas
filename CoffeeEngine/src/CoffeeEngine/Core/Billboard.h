#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/Material.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Coffee
{
    enum class BillboardType
    {
        SCREEN_ALIGNED,
        WORLD_ALIGNED,
        AXIS_ALIGNED
    };

    class Billboard
    {
      public:
        static Ref<Billboard> Create(BillboardType type = BillboardType::SCREEN_ALIGNED);

        Billboard(BillboardType type = BillboardType::SCREEN_ALIGNED);
        ~Billboard() = default;

        void SetPosition(const glm::vec3& position) { m_Position = position; }
        void SetScale(const glm::vec3& scale) { m_Scale = scale; }
        void SetType(BillboardType type) { m_Type = type; }
        void SetMaterial(const Ref<Material>& material) { m_Material = material; }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetScale() const { return m_Scale; }
        BillboardType GetType() const { return m_Type; }
        const Ref<Material>& GetMaterial() const { return m_Material; }

        glm::mat4 CalculateTransform(const glm::vec3& cameraPosition, const glm::vec3& cameraUp);

      private:
        glm::mat4 CalculateScreenAligned(const glm::vec3& cameraPosition, const glm::vec3& cameraUp);
        glm::mat4 CalculateWorldAligned(const glm::vec3& cameraPosition, const glm::vec3& cameraUp);
        glm::mat4 CalculateAxisAligned(const glm::vec3& cameraPosition);

        BillboardType m_Type;
        glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
        glm::vec3 m_Scale = {1.0f, 1.0f, 1.0f};
        Ref<Material> m_Material;
    };

} // namespace Coffee