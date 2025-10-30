#include "OS/Window/Window.h"
#include "Rendering/Samples/HelloTriangle.h"

int main() {
    Window::InitGLFW();

    //TODO: Console Argument parsing for paramaters
    {
        HelloTriangle app = HelloTriangle(1920, 1080);
        app.Start();
    }

    Window::TerminateGLFW();
    return 0;
}
