#pragma once

#include "CoffeeEngine/Events/Event.h"
#include "CoffeeEngine/Events/MouseEvent.h"
#include <CoffeeEngine/Renderer/Camera.h>

#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief The EditorCamera class is responsible for managing the editor camera's view and projection matrices.
     */
    class EditorCamera : public Camera
    {
    public:
        /**
         * @brief Enum class representing the state of the camera.
         */
        enum class CameraState {
            NONE,   ///< No camera state.
            ORBIT,  ///< Camera is in orbit mode.
            FLY     ///< Camera is in fly mode.
        };

        /**
         * @brief Default constructor for the EditorCamera class.
         */
        EditorCamera() = default;

        /**
         * @brief Constructs an EditorCamera object with the specified parameters.
         * @param fov The field of view for the perspective projection.
         * @param projection The type of projection (perspective or orthographic).
         * @param aspectRatio The aspect ratio of the viewport.
         * @param nearClip The near clipping plane distance.
         * @param farClip The far clipping plane distance.
         */
        EditorCamera(float fov, ProjectionType projection = ProjectionType::PERSPECTIVE, float aspectRatio = 1.778, float nearClip = 0.1f, float farClip = 1000.0f);

        /**
         * @brief Updates the camera's view matrix.
         */
        void OnUpdate(float dt);

        /**
         * @brief Handles events for the camera.
         * @param event The event to handle.
         */
        void OnEvent(Event& event);

        /**
         * @brief Sets the focal point of the camera.
         * @param focusPoint The focal point.
         */
        inline void SetFocusPoint(glm::vec3 focusPoint) { m_FocalPoint = focusPoint; }

        /**
         * @brief Gets the view matrix of the camera.
         * @return The view matrix.
         */
        const glm::mat4 GetViewMatrix() const { return m_ViewMatrix; };

        /**
         * @brief Gets the up direction of the camera.
         * @return The up direction.
         */
        glm::vec3 GetUpDirection() const;

        /**
         * @brief Gets the right direction of the camera.
         * @return The right direction.
         */
        glm::vec3 GetRightDirection() const;

        /**
         * @brief Gets the forward direction of the camera.
         * @return The forward direction.
         */
        glm::vec3 GetForwardDirection() const;

        /**
         * @brief Gets the position of the camera.
         * @return The position.
         */
        const glm::vec3& GetPosition() const { return m_Position; }

        /**
         * @brief Gets the orientation of the camera.
         * @return The orientation.
         */
        glm::quat GetOrientation() const;

        /**
         * @brief Gets the current state of the camera.
         * @return The current state of the camera.
         */
        const CameraState& GetState() const { return m_CurrentState; }

        const float& GetFlySpeed() const { return m_CurrentSpeed; }

        const float& GetOrbitZoom() const { return m_Distance; }

        /**
         * @brief Gets the view projection matrix of the camera.
         * @return The view projection matrix.
         */
        glm::mat4 GetViewProjection() const { return GetProjection() * GetViewMatrix(); }

    private:
        /**
         * @brief Updates the view matrix based on the current position and orientation.
         */
        void UpdateView();

        /**
         * @brief Handles mouse scroll events.
         *
         * This function calculates the increment speed using the following formulas:
         *
         * \f[
         * \text{incrementSpeed} = e^{(0.1 \times \text{m\_CameraSpeed})} - 1
         * \f]
         * \f[
         * \text{m\_CameraSpeed} = \text{m\_CameraSpeed} + (\delta \times \text{incrementSpeed})
         * \f]
         *
         * @param event The mouse scroll event.
         * @return True if the event was handled, false otherwise.
         */
        bool OnMouseScroll(MouseScrolledEvent& event);

        /**
         * @brief Calculates the position of the camera based on the focal point and distance.
         * @return The calculated position.
         */
        glm::vec3 CalculatePosition() const;

        /**
         * @brief Rotates the camera based on mouse movement.
         * @param delta The mouse movement delta.
         */
        void MouseRotate(const glm::vec2& delta);

        /**
         * @brief Pans the camera based on mouse movement.
         * @param delta The mouse movement delta.
         */
        void MousePan(const glm::vec2& delta);

        /**
         * @brief Zooms the camera based on mouse scroll.
         * @param delta The mouse scroll delta.
         */
        void MouseZoom(float delta);

        void Fly(const glm::vec2& mouseDelta);

    private:
        glm::mat4 m_ViewMatrix; ///< The view matrix of the camera.

        glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f); ///< The position of the camera.
        glm::vec3 m_FocalPoint = glm::vec3(0.0f, 0.0f, 0.0f); ///< The focal point of the camera.

        float m_Distance = 10.0f; ///< The distance from the focal point.
        float m_BaseSpeed = 0.1f; ///< The base speed of the camera.
        float m_CurrentSpeed = m_BaseSpeed; ///< The speed of the camera.
        float m_Pitch = 0.0f, m_Yaw = 0.0f; ///< The pitch and yaw angles of the camera.

        glm::vec2 m_InitialMousePosition = glm::vec2(0.0f, 0.0f); ///< The initial mouse position.

        CameraState m_CurrentState = CameraState::NONE; ///< The current state of the camera.
    };

    /** @} */
}