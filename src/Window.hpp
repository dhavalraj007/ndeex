#pragma once

#include <Windows.h>
#include "Vulkan.hpp"
#include <GLFW/glfw3.h>
#include "Exception.hpp"

namespace Core
{
    struct WindowCreateInfo
    {
        uint32_t width;
        uint32_t height;
        std::string title;
    };
    class Window
    {
    public:
        Window(WindowCreateInfo wci) : windowCreateInfo(wci)
        {
            if (!glfwInit())
            {
                char const *errString;
                int errCode = glfwGetError(&errString);
                throw std::runtime_error{errString};
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window = glfwCreateWindow(windowCreateInfo.width, windowCreateInfo.height, windowCreateInfo.title.c_str(), nullptr, nullptr);
        }

        ~Window()
        {
            glfwDestroyWindow(window);
            glfwTerminate();
        }

        bool closeRequested()
        {
            return glfwWindowShouldClose(window);
        }

        void clearScreen()
        {
        }

        void pollAndrocessEvents() const
        {
            glfwPollEvents();
        }

    private:
        WindowCreateInfo windowCreateInfo;
        GLFWwindow *window;
    };
}