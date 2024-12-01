#ifndef RENDERER_H
#define RENDERER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Mingw windows multithreading library
//#include <windows.h>

// SDL2 librarys
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

typedef struct {
    char* file;
    int size;
    TTF_Font* font;
} Font;
typedef struct {
    SDL_Renderer* ren;
    SDL_Window* win;
} Window;
typedef struct
{
    int x;
    int y;
} Point;
typedef struct
{
    int w;
    int h;
} Size;
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;
typedef struct
{
    int length;
    Point pnt;
    char* str;

    SDL_Color color;
    Font font;
} Text;
typedef struct
{
    Point pnt;
    Size size;
    
    SDL_Color color;
} Rect;
typedef struct {
    Point pnt;
    Size size;
    char* file;

    SDL_Texture* texture;
} Sprite;
typedef struct {
    Point pnt;
    Size size;
    Color color;
} Circle;
typedef struct {
    Point pnt;
    Size size;
    Color color;
    Color textColor;
    int length;
    char* str;
    void (*func)(int, char**);
    Font font;
} Button;

typedef struct {
    bool show;
    Rect background;
    Rect foreground;
    Text title;
    Text text;
    Button xButton;
    Button buttons[3];
    void (*func)(int, char**);
} MessageBoxRend;

typedef struct {
    //Mix_Music* music;
    bool playing;
    int playFor;
    int id;
} Music;
typedef struct {
    SDL_Texture* texture;
    SDL_Surface* surface;
    TTF_Font* font;
} StandardObject;
typedef struct {
    char key[32];
    bool pressed;
    bool released;
    bool held;
} Key;
typedef struct {
    Point pnt;
    bool mouseDown;
    int left;
    int right;
    int center;
    int state;
} Mouse;

Window makeWindowFromContext(SDL_Window* win, SDL_Renderer* ren);
char* getFile(char* file);
void showRenders(Window window);
void clearScreen(Window window);
void setRenderColor(Window window, Color color);

// SDL Specific functions
SDL_Color convertColor(Color color);
// end SDL Specific functions

Text initText(char* str, int x, int y, int length, Color color, Font font);
Rect initRect(int x, int y, int w, int h, Color color);
Sprite initSprite(int x, int y, int w, int h, char* file, Window window);
Button createButton(int x, int y, int w, int h, char* str, Color color, Color textColor, Font fnt, void* click);
MessageBoxRend createMessageBox(int x, int y, int w, int h, char* title, char* text, Color color, Color textColor, Font fnt, void* click);
Music getSound(char* file, int loops);
Key keyInfo(char key);
Mouse getMousePosition();
Font initFont(char* file, int size);

void playSound(Music music);
void stopSound();
void renderText(Window window, Text text);
void renderRect(Window window, Rect rect);
void renderSprite(Window window, Sprite sprite);
void renderButton(Window window, Button button);
void renderMBox(Window window, MessageBoxRend msgBox, bool mouseDown, int mouseX, int mouseY);
void simpleCollisionDetector(Sprite* sprite1, Sprite* sprite2, int *xHit, int *yHit);
void checkButtonClicked(Button button, bool mouseDown, int mouse_x, int mouse_y, int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H