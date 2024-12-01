// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "init.h"
#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_sdl2.h>
#include <ImGui/backends/imgui_impl_sdlrenderer2.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>
#include <wiikeyboard/keyboard.h>
#include <wiikeyboard/usbkeyboard.h>

// SDL2 Fix
#undef main

// External Factors
extern SDL2_Initresult_t game;
extern ir_t ir;
extern bool imGuiReady;

// Threads.
lwp_t renderThread;

// Rendering
void* render_thread(void* arg) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.MouseDrawCursor = true;

    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Our Style.
    style.WindowRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;

    // Convert colors
    // Normal: RGBA
    // Modify: ABGR
    for(int i=0; i<ImGuiCol_COUNT; i++) {
        ImVec4 newColor = ImVec4(style.Colors[i].w,style.Colors[i].z,style.Colors[i].x,style.Colors[i].y);
        style.Colors[i] = newColor;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(game.window, game.renderer);
    ImGui_ImplSDLRenderer2_Init(game.renderer);
    imGuiReady=true;

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;

    float* my_color = (float*)malloc(sizeof(float)*4);
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    // Main loop
    bool done = false;
    while (!done)
    {
        WPAD_IR(WPAD_CHAN_0, &ir);

        // Pressed keys.
        s32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
        if(pressed & WPAD_BUTTON_HOME) exit(0);

        // Update Mouse Position
        if(imGuiReady) {
            ImVec2 nMousePos;
            nMousePos.x = ir.x;
            nMousePos.y = ir.y;
            ImGui::SetMousePosExternal(nMousePos);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        //    if (event.type == SDL_QUIT)
        //        done = true;
        //    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(game.window))
        //        done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Create a Window
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_MenuBar;

        ImGui::SetNextWindowSize(ImVec2(500, 380), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImVec2((640/2)-(500/2),(480/2)-(380/2)), ImGuiCond_Appearing);
        ImGui::Begin("Wii Beans", NULL, window_flags);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                ImGui::SeparatorText("Test Menu Bar");
                ImGui::MenuItem("Assets Browser", NULL, &show_demo_window);
                ImGui::MenuItem("A Sample Tool", NULL, &show_another_window);
                ImGui::MenuItem("Exit", NULL, &done);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::Text("Test Window");
        ImGui::ProgressBar(0.5f, ImVec2(0.0f, 0.0f));
        ImGui::End();

        // Create a window called "My First Tool", with a menu bar.
        ImGui::Begin("My First Tool", &show_another_window, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
                if (ImGui::MenuItem("Close", "Ctrl+W"))  { show_another_window = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Edit a color stored as 4 floats
        ImGui::ColorEdit4("Color", my_color);

        // Generate samples and plot them
        float samples[100];
        for (int n = 0; n < 100; n++)
            samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
        ImGui::PlotLines("Samples", samples, 100);

        // Display contents in a scrolling region
        ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");
        ImGui::BeginChild("Scrolling");
        for (int n = 0; n < 50; n++)
            ImGui::Text("%04d: Some text", n);
        ImGui::EndChild();
        ImGui::End();

        // Demo
        if(show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(game.renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(game.renderer,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255)
        );
        SDL_RenderClear(game.renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), game.renderer);
        SDL_RenderPresent(game.renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();

    return NULL;
}

// Main code
int main(int, char**)
{
    // Initialize the platform.
    game = Wii_Init();
    if(game.rC != 0) {
        SDL_Log("ERROR: %d, FAILURE\n", game.rC);
        return -1;
    }

    // Start the render thread.
    if (LWP_CreateThread(&renderThread, render_thread, NULL, NULL, 0, 80) != 0) {
        SDL_Log("Failed to create button handler thread.\n");
        return -1;
    }

    // Main Thread (Do nothing)
    while(true) {
        usleep(800);
    }
}