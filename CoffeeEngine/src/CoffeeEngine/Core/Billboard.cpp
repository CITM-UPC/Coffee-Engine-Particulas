#include "Billboard.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Coffee
{
    Ref<Billboard> Billboard::Create(BillboardType type)
    {
        return CreateRef<Billboard>(type);
    }

    Billboard::Billboard(BillboardType type) : m_Type(type) {}

    glm::mat4 Billboard::CalculateTransform(const glm::vec3& cameraPosition, const glm::vec3& cameraUp)
    {
        switch (m_Type)
        {
        case BillboardType::SCREEN_ALIGNED:
            return CalculateScreenAligned(cameraPosition, cameraUp);
        case BillboardType::WORLD_ALIGNED:
            return CalculateWorldAligned(cameraPosition, cameraUp);
        case BillboardType::AXIS_ALIGNED:
            return CalculateAxisAligned(cameraPosition);
        }
        return glm::mat4(1.0f);
    }

    glm::mat4 Billboard::CalculateScreenAligned(const glm::vec3& cameraPosition, const glm::vec3& cameraUp)
    {
        glm::vec3 N = glm::normalize(cameraPosition - m_Position);
        glm::vec3 U = cameraUp;
        glm::vec3 R = glm::cross(U, N);

        glm::mat4 transform = glm::mat4(glm::vec4(R * m_Scale.x, 0.0f), glm::vec4(U * m_Scale.y, 0.0f), glm::vec4(N * m_Scale.z, 0.0f), glm::vec4(m_Position, 1.0f));

        return transform;
    }

    glm::mat4 Billboard::CalculateWorldAligned(const glm::vec3& cameraPosition, const glm::vec3& cameraUp)
    {
        glm::vec3 N = glm::normalize(cameraPosition - m_Position);
        glm::vec3 U = cameraUp;
        glm::vec3 R = glm::normalize(glm::cross(U, N));
        U = glm::cross(N, R);

        glm::mat4 transform = glm::mat4(glm::vec4(R * m_Scale.x, 0.0f), glm::vec4(U * m_Scale.y, 0.0f), glm::vec4(N * m_Scale.z, 0.0f), glm::vec4(m_Position, 1.0f));

        return transform;
    }

    glm::mat4 Billboard::CalculateAxisAligned(const glm::vec3& cameraPosition)
    {
        glm::vec3 dirToCamera = cameraPosition - m_Position;
        dirToCamera.y = 0.0f;
        dirToCamera = glm::normalize(dirToCamera);

        glm::vec3 R = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), dirToCamera);

        glm::mat4 transform = glm::mat4(glm::vec4(R * m_Scale.x, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) * m_Scale.y, glm::vec4(dirToCamera * m_Scale.z, 0.0f), glm::vec4(m_Position, 1.0f));

        return transform;
    }

} // namespace Coffee