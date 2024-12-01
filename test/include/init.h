#pragma once

#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_sdl2.h>
#include <ImGui/backends/imgui_impl_sdlrenderer2.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <gccore.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>
#include <wiikeyboard/keyboard.h>
#include <wiikeyboard/usbkeyboard.h>

#ifndef INIT_H
#define INIT_H

// SDL2 Init result
typedef struct {
    int             rC;
    SDL_Window*     window;
    SDL_Renderer*   renderer;
} SDL2_Initresult_t;

// Thread Functions
void* buttonThreadFunc(void* arg);
void *kbd_thread (void *arg);

// Init Systems
SDL2_Initresult_t Wii_Init(void);

extern ImGuiKey ImGui_ImplSDL2_KeyEventToImGuiKey(SDL_Keycode keycode, SDL_Scancode scancode);
extern void ImGui_ImplSDL2_UpdateKeyModifiers(SDL_Keymod sdl_key_mods);

// Functions
void KBEventHandler(USBKeyboard_event event);
void log_to_file(void *userdata, int category, SDL_LogPriority priority, const char *message);

#endif // INIT_H