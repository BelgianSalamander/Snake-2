//
// Created by Anatol on 25/06/2022.
//

#ifndef SNAKE_IMGUIRENDERER_H
#define SNAKE_IMGUIRENDERER_H

#include "imgui.h"

#include <map>

struct GLFWwindow;

class ImGuiRenderer {
public:
    ImGuiRenderer();
    virtual ~ImGuiRenderer();

    bool init();
    void renderFrame();
    void cleanup();

    void mainloop();
protected:
    virtual void render() = 0;
private:
    enum State {
        NOT_INITIALIZED,
        INITIALIZED,
        CLEANED_UP
    } state;

    std::map<float, ImFont*> fonts;
    GLFWwindow* window;

    float ref;

    ImFont* getFont(float dpiScale);
};


#endif //SNAKE_IMGUIRENDERER_H
