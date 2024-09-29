#pragma once

#include "CoffeeEngine/Core/KeyCodes.h"
#include "CoffeeEngine/Core/MouseCodes.h"
#include "CoffeeEngine/Events/Event.h"

#include <functional>
#include <glm/glm.hpp>

namespace Coffee {
    /**
     * @defgroup core Core
     * @brief Core components of the CoffeeEngine.
     * @{
     */
	class Input
	{
	public:
		/**
		 * Checks if a specific key is currently being pressed.
		 *
		 * @param key The key code of the key to check.
		 * @return True if the key is currently being pressed, false otherwise.
		 */
		static bool IsKeyPressed(const KeyCode key);

		/**
		 * Checks if a mouse button is currently pressed.
		 *
		 * @param button The mouse button to check.
		 * @return True if the mouse button is pressed, false otherwise.
		 */
		static bool IsMouseButtonPressed(const MouseCode button);
		/**
		 * Retrieves the current position of the mouse.
		 *
		 * @return The current position of the mouse as a 2D vector.
		 */
		static glm::vec2 GetMousePosition();
		/**
		 * @brief Retrieves the current x-coordinate of the mouse cursor.
		 *
		 * @return The x-coordinate of the mouse cursor.
		 */
		static float GetMouseX();
		/**
		 * @brief Retrieves the current y-coordinate of the mouse cursor.
		 *
		 * @return The y-coordinate of the mouse cursor.
		 */
		static float GetMouseY();

				using EventCallbackFn = std::function<void(Event&)>; ///< Type definition for event callback function.
		/**
         * @brief Polls and processes events.
         * 
         * This function retrieves and handles events such as input from the keyboard,
         * mouse, window, and other devices.
         */
        static void ProcessEvents();

		/**
         * @brief Sets the event callback function.
         * @param callback The event callback function.
         */
        static void SetEventCallback(const EventCallbackFn& callback) { m_EventCallback = callback; }
	private:
		static EventCallbackFn m_EventCallback; ///< The event callback function.
	};
    /** @} */
}