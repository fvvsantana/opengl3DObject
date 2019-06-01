#ifndef GRAPHICSLIB_HPP
#define GRAPHICSLIB_HPP


#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace ml{
    template<class T>
        class matrix;
}

namespace graphicslib {
    struct Vertex {
        float position[3];
        float color[3];
        float texcoord[2];
    };

    struct ModelCoordinates{
        float position[3];
        float rotation[3];
        float scale[3];
    };


    class Window {
    private:
        int mWindowWidth;
        int mWindowHeight;
        GLFWwindow* mWindow;
        GLuint mCoreProgram;

        ModelCoordinates modelCoord;

        // perspective type
        bool mOrthogonalProjection;
        bool mPReleased;

        // timing
        float mDeltaTime;
        float mLastFrame;


        //callback function to execute when the window is resized
        static void framebufferResizeCallback(GLFWwindow* window, int fbWidth, int fbHeight);

        void updateInput(GLFWwindow *window);

        static void glfwErrorCallback(int error, const char* description);

    public:

        //init glfw stuff
        Window(int windowWidth, int windowHeight);

        //destroy everything
        ~Window();

        //create the window, load glad, load shaders
        void createWindow();

        //main loop
        void run(char *filepath);
    };
}


#endif
