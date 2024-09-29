#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

struct Platform {
    float x, y, width, height;
    ImVec4 color;
};

std::vector<std::vector<Platform>> levels;
int currentLevel = 0;
int MAX_LEVELS = 10;
int currPlatformWidth = 0;
int currPlatformHeight = 0;
ImVec4 currPlatformColor = {0, 0, 0, 0};

void SaveLevels(const char* filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "MAX_LEVELS = " << MAX_LEVELS << ";\n";
        for (int i = 0; i < levels.size(); i++) {
            file << "Level " << i + 1 << ":\n";
            for (const auto& platform : levels[i]) {
                // Save platform's properties including RGBA values
                file << platform.x << " " << platform.y << " " 
                     << platform.width << " " << platform.height << " "
                     << platform.color.x << " " << platform.color.y << " "
                     << platform.color.z << " " << platform.color.w << "\n";
            }
        }
        file.close();
    } else {
        std::cerr << "Error: Could not open file for saving levels.\n";
    }
}


void LoadLevels(const char* filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        levels.clear();
        int levelIndex = -1;
        
        while (std::getline(file, line)) {
            // Parse the MAX_LEVELS line
            if (line.find("MAX_LEVELS") != std::string::npos) {
                sscanf(line.c_str(), "MAX_LEVELS = %d;", &MAX_LEVELS);
                levels.resize(MAX_LEVELS);
            } 
            // Detect when a new level starts
            else if (line.find("Level") != std::string::npos) {
                levelIndex++;
            } 
            // Parse platform data
            else if (!line.empty() && levelIndex >= 0) {
                Platform p;
                sscanf(line.c_str(), "%f %f %f %f %f %f %f %f", 
                    &p.x, &p.y, &p.width, &p.height, 
                    &p.color.x, &p.color.y, &p.color.z, &p.color.w);
                levels[levelIndex].push_back(p);
            }
        }
        file.close();
    } else {
        std::cerr << "Error: Could not open file for loading levels.\n";
    }
}



static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(800, 600, "2D Platformer Level Editor", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    levels.resize(MAX_LEVELS);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec4 platformColor = ImVec4(0, 1, 0, 1);
    bool dragging = false;
    int draggedPlatform = -1;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Level Editor");

        if (ImGui::Button("Save Levels"))
            SaveLevels("../../levels.txt");

        ImGui::SameLine();

        if (ImGui::Button("Load Levels"))
            LoadLevels("../../levels.txt");
        if (ImGui::Button("Green Color"))
            platformColor = ImVec4(0, 1, 0, 1);
        if (ImGui::Button("Blue Color"))
            platformColor = ImVec4(0, 0, 1, 1);
        if (ImGui::Button("Red Color"))
            platformColor = ImVec4(1, 0, 0, 1);

        ImGui::SliderInt("MAX LEVELS", &MAX_LEVELS, 0, 200);
        ImGui::SliderInt("Current Level", &currentLevel, 0, MAX_LEVELS - 1);
        ImGui::SliderInt("PLATFORM WIDTH", &currPlatformWidth, 0, 800);
        ImGui::SliderInt("PLATFORM HEIGHT", &currPlatformHeight, 0, 600);

        
    
        if (ImGui::Button("Add Platform"))
            levels[currentLevel].push_back({100, 100, (float)currPlatformWidth, (float)currPlatformHeight, platformColor});

        ImGui::Text("Drag platforms to move them. Right-click to delete.");

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImVec2(800, 600);
        ImGui::InvisibleButton("canvas", canvasSize);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 rectMin = canvasPos;
        ImVec2 rectMax = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        draw_list->AddRect(rectMin, rectMax, IM_COL32(255, 255, 255, 255), 0.0f, ImDrawFlags_None, 2.0f);

        for (int i = 0; i < levels[currentLevel].size(); i++) {
            auto& platform = levels[currentLevel][i];
            ImVec2 rectMin = ImVec2(canvasPos.x + platform.x, canvasPos.y + platform.y);
            ImVec2 rectMax = ImVec2(rectMin.x + platform.width, rectMin.y + platform.height);
            draw_list->AddRectFilled(rectMin, rectMax, ImColor(platform.color));

            if (ImGui::IsMouseHoveringRect(rectMin, rectMax)) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    dragging = true;
                    draggedPlatform = i;
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    levels[currentLevel].erase(levels[currentLevel].begin() + i);
                    i--;
                }
            }
        }

        if (dragging && draggedPlatform != -1) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            levels[currentLevel][draggedPlatform].x += delta.x;
            levels[currentLevel][draggedPlatform].y += delta.y;

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                dragging = false;
                draggedPlatform = -1;
            }
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w,
                     clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w,
                     clear_color.w);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
