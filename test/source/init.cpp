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

#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>
#include <wiikeyboard/keyboard.h>
#include <wiikeyboard/usbkeyboard.h>

s8 _input_init = 0;
u32 _kbd_pressed = 0;
bool kbd_should_quit = false;

// Wii Remote.
ir_t ir;

// Global variables.
bool quitapp;
bool isKBInit;
bool imGuiReady;

// Counters.
int keyCount;
int prvCount;
char *keyBuf;

// Threads
lwp_t kbd_handle;
lwp_t buttonThread;

// Game
SDL2_Initresult_t game;

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

    // TODO: Log to file for Wii debug.
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(result.renderer, &info) == 0) {
        // Log renderer information
        SDL_Log("Renderer: %s", info.name);
        SDL_Log("Max Texture Size: %dx%d", info.max_texture_width, info.max_texture_height);
        SDL_Log("Supported Texture Formats: %d", info.num_texture_formats);
        for (Uint32 i = 0; i < info.num_texture_formats; ++i) {
            SDL_Log(" - Format %d: %s", i, SDL_GetPixelFormatName(info.texture_formats[i]));
        }
    } else {
        SDL_Log("Failed to get renderer info: %s", SDL_GetError());
    }

    /*
        Threads
    */
    
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
            SDL_Log("%s(): Error, Unknown UsageCode(0x%04x)\n", __FUNCTION__, usageID);
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