#pragma once

#include "TeaEngine/Core/Base.h"
#include "TeaEngine/Events/Event.h"
#include "TeaEngine/Renderer/GraphicsContext.h"

#include <cstdint>
#include <functional>
#include <sstream>

struct GLFWwindow;

namespace Tea {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Tea Engine",
			        uint32_t width = 1600,
			        uint32_t height = 900)
			: Title(title), Width(width), Height(height)
		{
		}
	};

    class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(const WindowProps& props);
		virtual ~Window();

		void OnUpdate();

		unsigned int GetWidth() const { return m_Data.Width; }
		unsigned int GetHeight() const { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) {m_Data.EventCallback = callback;}
		void SetVSync(bool enabled);
		bool IsVSync() const;

		void SetTitle(const std::string& title);
		const std::string& GetTitle() const { return m_Data.Title; }

		void SetIcon(const std::string& path);

		virtual void* GetNativeWindow() const { return m_Window; }

        static Scope<Window> Create(const WindowProps& props = WindowProps()) {return CreateScope<Window>(props);}
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}