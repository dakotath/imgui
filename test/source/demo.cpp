// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

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

// Wii Remote.
ir_t ir;

// Global variables.
bool quitapp;
bool isKBInit;
bool imGuiReady;
s8 _input_init = 0;
u32 _kbd_pressed = 0;
bool kbd_should_quit = false;

// Threads.
lwp_t buttonThread;
lwp_t renderThread;
lwp_t kbd_handle;
void* buttonThreadFunc(void* arg);
void *kbd_thread (void *arg);

// Counters.
int keyCount;
int prvCount;
char *keyBuf;

// SDL2 Init result
typedef struct {
    int             rC;
    SDL_Window*     window;
    SDL_Renderer*   renderer;
} SDL2_Initresult_t;

// Init Systems
SDL2_Initresult_t Wii_Init(void);
SDL2_Initresult_t game;

extern ImGuiKey ImGui_ImplSDL2_KeyEventToImGuiKey(SDL_Keycode keycode, SDL_Scancode scancode);
extern void ImGui_ImplSDL2_UpdateKeyModifiers(SDL_Keymod sdl_key_mods);

// Functions
void KBEventHandler(USBKeyboard_event event);
void log_to_file(void *userdata, int category, SDL_LogPriority priority, const char *message);

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

    // Color format: ABGR
    //style.Colors[ImGuiCol_WindowBg] = ImVec4(1.00f, 0.06f, 0.06f, 0.06f);
    //style.Colors[ImGuiCol_PopupBg]  = ImVec4(0.85f, 0.3f, 0.3f, 0.3f);

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
                ImGui::MenuItem("Exit", NULL, &done);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::Text("Test Window");
        ImGui::Text("Keys: %s", keyBuf);
        ImGui::ProgressBar(0.5f, ImVec2(0.0f, 0.0f));
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
    //keyBuf = (char*)malloc(1);
    //keyBuf[0] = '\0';
    game = Wii_Init();
    if(game.rC != 0) {
        SDL_Log("ERROR: %d, FAILURE\n", game.rC);
        return -1;
    }

    // Start external input watcher thread.
    if (LWP_CreateThread(&renderThread, render_thread, NULL, NULL, 0, 80) != 0) {
        SDL_Log("Failed to create button handler thread.\n");
        return -1;
    }

    while(true) {
        usleep(800);
    }

    // Play BGM.
    //PlayOgg(bgm_ogg, bgm_ogg_size, 0, OGG_INFINITE_TIME);
}

void kp(char sym) {
    usleep(1000);
    return;
}

// Init everything.
SDL2_Initresult_t Wii_Init(void) {
    SDL2_Initresult_t result;
    result.rC = 0;

    /*
        Hardware
    */

    // Audio
    ASND_Init();

    // FAT (SD Card)
    fatInitDefault();

    // Open a log file for writing
    FILE *logfile = fopen("sdl_log.txt", "w");
    if (!logfile) {
        printf("Failed to open log file: %s", SDL_GetError());
        //result.rC = -1;
        //return result;
    }

    // Controller
	WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

	//USB_Initialize();	
    if (KEYBOARD_Init(kp) == 0) {
        isKBInit=true;
    }
    //USBKeyboard_Initialize();

	// Even if the thread fails to start,
	kbd_should_quit = false;

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(logfile, "Error: %s\n", SDL_GetError());
        result.rC = -1;
        return result;
    }

    //KEYBOARD_Deinit();
	if (USB_Initialize() != IPC_OK) {
        result.rC = -1;
		return result;
    }

	if (USBKeyboard_Initialize() != IPC_OK) {
        result.rC = -1;
		return result;
	}

    // Set custom log function to write logs to the file
    SDL_LogSetOutputFunction(log_to_file, (void *)logfile);

    // SDL2 ttf init
    if (TTF_Init() != 0)
    {
        SDL_Log("TTF_Init Error: %s\n", SDL_GetError());
        result.rC = -1;
        return result;
    }

    // SDL2 image init
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
    {
        SDL_Log("IMG_Init Error: %s\n", SDL_GetError());
        result.rC = -1;
        return result;
    }

    // Window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_FULLSCREEN);
    result.window = SDL_CreateWindow("Dear ImGui Wii", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, window_flags);
    if (result.window == nullptr)
    {
        SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        result.rC = -1;
        return result;
    }

    // Renderer
    result.renderer = SDL_CreateRenderer(result.window, -1, SDL_RENDERER_PRESENTVSYNC); // SDL_RENDERER_ACCELERATED
    if (result.renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        result.rC = -1;
        return result;
    }

    // sdl2 mixer init
    /*
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        SDL_Log("Mixer Error: %s\n", SDL_GetError());
        result.rC = -1;
        return result;
    }
    */

    // TODO: Log to file for Wii debug.
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(result.renderer, &info) == 0) {
        // Log renderer information
        SDL_Log("Renderer: %s", info.name);
        SDL_Log("Max Texture Size: %dx%d", info.max_texture_width, info.max_texture_height);
        SDL_Log("Supported Texture Formats: %d", info.num_texture_formats);
        for (int i = 0; i < info.num_texture_formats; ++i) {
            SDL_Log(" - Format %d: %s", i, SDL_GetPixelFormatName(info.texture_formats[i]));
        }
    } else {
        SDL_Log("Failed to get renderer info: %s", SDL_GetError());
    }

    /*
        Threads
    */

    //gettimeofday(&_time_init, NULL);
    
    // Start external input watcher thread.
    if (LWP_CreateThread(&buttonThread, buttonThreadFunc, NULL, NULL, 0, 80) != 0) {
        SDL_Log("Failed to create button handler thread.\n");
        result.rC = -1;
        return result;
    }

    return result;
}

// Button Watcher Thread Function.
void* buttonThreadFunc(void* arg) {
    // Counters
    prvCount=0;
    keyCount=0;

    // Thread loop
    while(true) {
        // Scan for pad changes, and update IR.
        //WPAD_ScanPads();

        if(!kbd_should_quit) {
            if(!USBKeyboard_IsConnected())
            {
                USBKeyboard_Open(KBEventHandler);
                //wake up the keyboard by sending it a command.
                //im looking at you, you bastard LINQ keyboard.
                USBKeyboard_SetLed(USBKEYBOARD_LEDCAPS, false);
            }
            USBKeyboard_Scan();
        }

        // Thread wait, and increment counter.
        usleep(800);
        prvCount++;
    }
}

// Function to map USB HID usage ID to SDL_Keycode
SDL_Keycode USBKeyToSDLKeyCode(u32 usageID) {
    switch (usageID) {
        // Alphabet keys
        case 0x04: return SDLK_a;   // Keyboard a and A
        case 0x05: return SDLK_b;   // Keyboard b and B
        case 0x06: return SDLK_c;   // Keyboard c and C
        case 0x07: return SDLK_d;   // Keyboard d and D
        case 0x08: return SDLK_e;   // Keyboard e and E
        case 0x09: return SDLK_f;   // Keyboard f and F
        case 0x0A: return SDLK_g;   // Keyboard g and G
        case 0x0B: return SDLK_h;   // Keyboard h and H
        case 0x0C: return SDLK_i;   // Keyboard i and I
        case 0x0D: return SDLK_j;   // Keyboard j and J
        case 0x0E: return SDLK_k;   // Keyboard k and K
        case 0x0F: return SDLK_l;   // Keyboard l and L
        case 0x10: return SDLK_m;   // Keyboard m and M
        case 0x11: return SDLK_n;   // Keyboard n and N
        case 0x12: return SDLK_o;   // Keyboard o and O
        case 0x13: return SDLK_p;   // Keyboard p and P
        case 0x14: return SDLK_q;   // Keyboard q and Q
        case 0x15: return SDLK_r;   // Keyboard r and R
        case 0x16: return SDLK_s;   // Keyboard s and S
        case 0x17: return SDLK_t;   // Keyboard t and T
        case 0x18: return SDLK_u;   // Keyboard u and U
        case 0x19: return SDLK_v;   // Keyboard v and V
        case 0x1A: return SDLK_w;   // Keyboard w and W
        case 0x1B: return SDLK_x;   // Keyboard x and X
        case 0x1C: return SDLK_y;   // Keyboard y and Y
        case 0x1D: return SDLK_z;   // Keyboard z and Z

        // Number keys (1 to 0)
        case 0x1E: return SDLK_1;   // Keyboard 1 and !
        case 0x1F: return SDLK_2;   // Keyboard 2 and @
        case 0x20: return SDLK_3;   // Keyboard 3 and #
        case 0x21: return SDLK_4;   // Keyboard 4 and $
        case 0x22: return SDLK_5;   // Keyboard 5 and %
        case 0x23: return SDLK_6;   // Keyboard 6 and ^
        case 0x24: return SDLK_7;   // Keyboard 7 and &
        case 0x25: return SDLK_8;   // Keyboard 8 and *
        case 0x26: return SDLK_9;   // Keyboard 9 and (
        case 0x27: return SDLK_0;   // Keyboard 0 and )

        // Special keys
        case 0x28: return SDLK_RETURN;  // Keyboard Return (ENTER)
        case 0x29: return SDLK_ESCAPE;  // Keyboard ESCAPE
        case 0x2A: return SDLK_BACKSPACE; // Keyboard DELETE (Backspace)
        case 0x2B: return SDLK_TAB;  // Keyboard Tab
        case 0x2C: return SDLK_SPACE;  // Keyboard Spacebar
        case 0x2D: return SDLK_MINUS;  // Keyboard - and (underscore)
        case 0x2E: return SDLK_EQUALS; // Keyboard = and +
        case 0x2F: return SDLK_LEFTBRACKET; // Keyboard [ and {
        case 0x30: return SDLK_RIGHTBRACKET; // Keyboard ] and }
        case 0x31: return SDLK_BACKSLASH; // Keyboard \ and |
        case 0x33: return SDLK_SEMICOLON; // Keyboard ; and :
        case 0x34: return SDLK_QUOTE;  // Keyboard ‘ and “
        //case 0x35: return SDLK_GRAVE;  // Keyboard Grave Accent and Tilde
        case 0x36: return SDLK_COMMA; // Keyboard , and <
        case 0x37: return SDLK_PERIOD; // Keyboard . and >
        case 0x38: return SDLK_SLASH;  // Keyboard / and ?
        case 0x39: return SDLK_CAPSLOCK;  // Keyboard Caps Lock

        // Function keys
        case 0x3A: return SDLK_F1;  // Keyboard F1
        case 0x3B: return SDLK_F2;  // Keyboard F2
        case 0x3C: return SDLK_F3;  // Keyboard F3
        case 0x3D: return SDLK_F4;  // Keyboard F4
        case 0x3E: return SDLK_F5;  // Keyboard F5
        case 0x3F: return SDLK_F6;  // Keyboard F6
        case 0x40: return SDLK_F7;  // Keyboard F7
        case 0x41: return SDLK_F8;  // Keyboard F8
        case 0x42: return SDLK_F9;  // Keyboard F9
        case 0x43: return SDLK_F10;  // Keyboard F10
        case 0x44: return SDLK_F11;  // Keyboard F11
        case 0x45: return SDLK_F12;  // Keyboard F12

        // Arrow keys and navigation keys
        case 0x50: return SDLK_LEFT;  // Keyboard LeftArrow
        case 0x51: return SDLK_DOWN;  // Keyboard DownArrow
        case 0x52: return SDLK_UP;    // Keyboard UpArrow
        case 0x53: return SDLK_RIGHT; // Keyboard RightArrow

        // Keypad keys
        case 0x54: return SDLK_KP_DIVIDE; // Keypad /
        case 0x55: return SDLK_KP_MULTIPLY; // Keypad *
        case 0x56: return SDLK_KP_MINUS;  // Keypad -
        case 0x57: return SDLK_KP_PLUS;   // Keypad +
        case 0x58: return SDLK_KP_ENTER;  // Keypad ENTER
        case 0x59: return SDLK_KP_1;      // Keypad 1
        case 0x5A: return SDLK_KP_2;      // Keypad 2
        case 0x5B: return SDLK_KP_3;      // Keypad 3
        case 0x5C: return SDLK_KP_4;      // Keypad 4
        case 0x5D: return SDLK_KP_5;      // Keypad 5
        case 0x5E: return SDLK_KP_6;      // Keypad 6
        case 0x5F: return SDLK_KP_7;      // Keypad 7
        case 0x60: return SDLK_KP_8;      // Keypad 8
        case 0x61: return SDLK_KP_9;      // Keypad 9
        case 0x62: return SDLK_KP_0;      // Keypad 0
        case 0x63: return SDLK_KP_PERIOD; // Keypad .

        // Modifier keys
        case 0xE0: return SDLK_LCTRL;   // Keyboard LeftControl
        case 0xE1: return SDLK_LSHIFT;  // Keyboard LeftShift
        case 0xE2: return SDLK_LALT;    // Keyboard LeftAlt
        case 0xE4: return SDLK_RCTRL;   // Keyboard RightControl
        case 0xE5: return SDLK_RSHIFT;  // Keyboard RightShift
        case 0xE6: return SDLK_RALT;    // Keyboard RightAlt

        // Default case for unmapped keys
        default:
            SDL_Log("%s(): Error, Unknown UsageCode(0x%04x)\n", usageID);
            return SDLK_UNKNOWN;
    }
}

// Check if usageID is is a char
bool USBKeyIsChar(u32 usageID) {
    switch(usageID) {
        // Alphabet keys
        case 0x04: return true;   // Keyboard a and A
        case 0x05: return true;   // Keyboard b and B
        case 0x06: return true;   // Keyboard c and C
        case 0x07: return true;   // Keyboard d and D
        case 0x08: return true;   // Keyboard e and E
        case 0x09: return true;   // Keyboard f and F
        case 0x0A: return true;   // Keyboard g and G
        case 0x0B: return true;   // Keyboard h and H
        case 0x0C: return true;   // Keyboard i and I
        case 0x0D: return true;   // Keyboard j and J
        case 0x0E: return true;   // Keyboard k and K
        case 0x0F: return true;   // Keyboard l and L
        case 0x10: return true;   // Keyboard m and M
        case 0x11: return true;   // Keyboard n and N
        case 0x12: return true;   // Keyboard o and O
        case 0x13: return true;   // Keyboard p and P
        case 0x14: return true;   // Keyboard q and Q
        case 0x15: return true;   // Keyboard r and R
        case 0x16: return true;   // Keyboard s and S
        case 0x17: return true;   // Keyboard t and T
        case 0x18: return true;   // Keyboard u and U
        case 0x19: return true;   // Keyboard v and V
        case 0x1A: return true;   // Keyboard w and W
        case 0x1B: return true;   // Keyboard x and X
        case 0x1C: return true;   // Keyboard y and Y
        case 0x1D: return true;   // Keyboard z and Z

        // Number keys (1 to 0)
        case 0x1E: return true;   // Keyboard 1 and !
        case 0x1F: return true;   // Keyboard 2 and @
        case 0x20: return true;   // Keyboard 3 and #
        case 0x21: return true;   // Keyboard 4 and $
        case 0x22: return true;   // Keyboard 5 and %
        case 0x23: return true;   // Keyboard 6 and ^
        case 0x24: return true;   // Keyboard 7 and &
        case 0x25: return true;   // Keyboard 8 and *
        case 0x26: return true;   // Keyboard 9 and (
        case 0x27: return true;   // Keyboard 0 and )

        // Special keys
        //case 0x2A: return true; // Keyboard DELETE (Backspace)
        case 0x2C: return true;  // Keyboard Spacebar
        case 0x2D: return true;  // Keyboard - and (underscore)
        case 0x2E: return true; // Keyboard = and +
        case 0x2F: return true; // Keyboard [ and {
        case 0x30: return true; // Keyboard ] and }
        case 0x31: return true; // Keyboard \ and |
        case 0x33: return true; // Keyboard ; and :
        case 0x34: return true;  // Keyboard ‘ and “
        case 0x36: return true; // Keyboard , and <
        case 0x37: return true; // Keyboard . and >
        case 0x38: return true;  // Keyboard / and ?

        // Keypad keys
        case 0x54: return true; // Keypad /
        case 0x55: return true; // Keypad *
        case 0x56: return true;  // Keypad -
        case 0x57: return true;   // Keypad +
        case 0x58: return true;  // Keypad ENTER
        case 0x59: return true;      // Keypad 1
        case 0x5A: return true;      // Keypad 2
        case 0x5B: return true;      // Keypad 3
        case 0x5C: return true;      // Keypad 4
        case 0x5D: return true;      // Keypad 5
        case 0x5E: return true;      // Keypad 6
        case 0x5F: return true;      // Keypad 7
        case 0x60: return true;      // Keypad 8
        case 0x61: return true;      // Keypad 9
        case 0x62: return true;      // Keypad 0
        case 0x63: return true; // Keypad .
        default: return false;
    }
}

Uint16 USBKeyGetMods(u32 usageID) {
    switch(usageID) {
        // Modifier keys
        case 0xE0: return KMOD_CTRL;   // Keyboard LeftControl
        case 0xE1: return KMOD_SHIFT;  // Keyboard LeftShift
        case 0xE2: return KMOD_ALT;    // Keyboard LeftAlt
        case 0xE4: return KMOD_CTRL;   // Keyboard RightControl
        case 0xE5: return KMOD_SHIFT;  // Keyboard RightShift
        case 0xE6: return KMOD_ALT;    // Keyboard RightAlt
        default: return KMOD_NONE;
    }
}

void KBEventHandler(USBKeyboard_event event) {
    if (event.type != USBKEYBOARD_PRESSED && event.type != USBKEYBOARD_RELEASED) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    char keyText;
    Uint16 mod;
    SDL_Event evt;
    SDL_zero(evt); // Initialize the SDL event structure
    if(event.type == USBKEYBOARD_PRESSED)
        SDL_Log("KeyCode: 0x%02x\n", event.keyCode);

    // Map the USB key code to an SDL scancode
    //SDL_Scancode sdlsc; //event.keyCode;
    SDL_Keycode sdlKey = USBKeyToSDLKeyCode(event.keyCode);
    if (sdlKey == SDLK_UNKNOWN) {
        return;  // If the key code is unknown, we do nothing
    }

    mod = USBKeyGetMods(event.keyCode);

    evt.type = (event.type == USBKEYBOARD_PRESSED) ? SDL_KEYDOWN : SDL_KEYUP;
    evt.key.type = (event.type == USBKEYBOARD_PRESSED) ? SDL_KEYDOWN : SDL_KEYUP;
    evt.key.state = (event.type == USBKEYBOARD_PRESSED) ? SDL_PRESSED : SDL_RELEASED;
    evt.key.keysym.sym = sdlKey;  // SDL key symbol (key code)
    evt.key.keysym.scancode = SDL_GetScancodeFromKey(sdlKey); // SDL scancode
    evt.key.keysym.mod = mod;  // You can add modifier keys if needed (SHIFT, CTRL, etc.)
    keyText = (char)sdlKey;

    // Push the event to SDL's event queue
    ImGuiKey key = ImGui_ImplSDL2_KeyEventToImGuiKey(evt.key.keysym.sym, evt.key.keysym.scancode);
    if(event.type == USBKEYBOARD_PRESSED && USBKeyIsChar(event.keyCode)) {
        ImGui_ImplSDL2_UpdateKeyModifiers((SDL_Keymod)evt.key.keysym.mod);
        io.AddInputCharacter(keyText);
    } else {
        io.AddKeyEvent(key, (evt.type == SDL_KEYDOWN));
        io.SetKeyEventNativeData(key, evt.key.keysym.sym, evt.key.keysym.scancode, evt.key.keysym.scancode); // To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_*** as indices to IsKeyXXX() functions.
    }
}

// Log info to SD card from SDL2.
void log_to_file(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    FILE *logfile = (FILE *)userdata;
    if (logfile) {
        fprintf(logfile, "%s\n", message);
        fflush(logfile); // Ensure logs are written immediately
    }
}