#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// SDL2 librarys
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "renderer.h"

// structs (to make life a bit easier)

#define assetFolder "assets/"

Window makeWindowFromContext(SDL_Window* win, SDL_Renderer* ren) {
    Window window;
    window.win = win;
    window.ren = ren;
    return window;
}

Key keyInfo(char key)
{
    Key ret;

    // get key state from windows
    bool state = false;//GetAsyncKeyState(key);
    
    if(state)
    {
        ret.pressed = 1;
        ret.released = 0;
    }
    else
    {
        ret.pressed = 0;
        ret.released = 1;
    }

    return ret;
}

Mouse getMousePosition()
{
    Mouse ret;
    SDL_Event event;
    ret.state = SDL_GetMouseState(&ret.pnt.x, &ret.pnt.y);
    SDL_PollEvent(&event);
    ret.left = event.button.button == SDL_BUTTON_LEFT;
    ret.right = event.button.button == SDL_BUTTON_RIGHT;
    return ret;
}

Font initFont(char* file, int size)
{
    char* path = getFile(file);
    TTF_Font* font = TTF_OpenFont(path, size);
    if (font == NULL)
    {
        SDL_Log("TTF_OpenFont Error: %s\n", SDL_GetError());
        exit(1);
    }
    Font ret;
    ret.font = font;
    ret.size = size;
    return ret;
}

char* getFile(char* file)
{
    char* ret = malloc(strlen(assetFolder) + strlen(file) + 1);
    strcpy(ret, assetFolder);
    return strcat(ret, file);
}

void showRenders(Window window)
{
    SDL_RenderPresent(window.ren);
}

void clearScreen(Window window)
{
    SDL_RenderClear(window.ren);
}

void setRenderColor(Window window, Color color)
{
    SDL_SetRenderDrawColor(window.ren, color.r, color.g, color.b, color.a);
}

// SDL Specific functions

SDL_Color convertColor(Color color)
{
    SDL_Color ret = {color.r, color.g, color.b, color.a};
    return ret;
}

// end SDL Specific functions

Text initText(char* str, int x, int y, int length, Color color, Font font)
{
    Text text;
    text.str = malloc(length);
    strcpy(text.str, str);
    text.pnt.x = x;
    text.pnt.y = y;
    text.length = length;
    text.color = convertColor(color);
    text.font = font;
    return text;
}

Rect initRect(int x, int y, int w, int h, Color color)
{
    Rect rect;
    rect.pnt.x = x;
    rect.pnt.y = y;
    rect.size.w = w;
    rect.size.h = h;
    rect.color = convertColor(color);
    return rect;
}

Sprite initSprite(int x, int y, int w, int h, char* file, Window window)
{
    Sprite sprite;
    sprite.pnt.x = x;
    sprite.pnt.y = y;
    sprite.size.w = w;
    sprite.size.h = h;
    sprite.file = getFile(file);
    SDL_Log("init sprite: %s\n", sprite.file);

    SDL_Surface* surface = IMG_Load(sprite.file);
    sprite.texture = SDL_CreateTextureFromSurface(window.ren, surface);
    SDL_FreeSurface(surface);
    return sprite;
}

Button createButton(int x, int y, int w, int h, char* str, Color color, Color textColor, Font fnt, void* click)
{
    Button button;
    button.pnt.x = x;
    button.pnt.y = y;
    button.size.w = w;
    button.size.h = h;
    button.color = color;
    button.textColor = textColor;
    button.func = click;
    button.font = fnt;

    // button text
    button.str = malloc(strlen(str) + 1);
    strcpy(button.str, str);
    button.length = strlen(str) + 1;

    return button;
}

MessageBoxRend createMessageBox(int x, int y, int w, int h, char* title, char* text, Color color, Color textColor, Font fnt, void* click)
{
    MessageBoxRend msgBox;
    msgBox.show = true;
    msgBox.background = initRect(x, y, w, h, (Color){127, 127, 127, 255});
    msgBox.foreground = initRect(x + 10, y + 10, w - 20, h - 20, color);
    msgBox.title = initText(title, x + 20, y + 20, strlen(title) + 1, textColor, fnt);
    msgBox.text = initText(text, x + 20, y + 40, strlen(text) + 1, textColor, fnt);
    msgBox.xButton = createButton(x + w - 30, y + 10, 20, 20, "X", (Color){127, 127, 127, 255}, textColor, fnt, click);
    return msgBox;
}

Music getSound(char* file, int loops)
{
    Music music;
    //music.music = Mix_LoadMUS(getFile(file));
    music.playFor = loops;
    return music;
}

void playSound(Music music)
{
    //Mix_PlayMusic(music.music, music.playFor);
}
void stopSound()
{
    //Mix_HaltMusic();
}

void renderText(Window window, Text text)
{
    SDL_Surface* surface = TTF_RenderText_Solid(text.font.font, text.str, text.color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(window.ren, surface);
    SDL_Rect rect = {text.pnt.x, text.pnt.y, surface->w, surface->h};
    SDL_RenderCopy(window.ren, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderRect(Window window, Rect rect)
{
    SDL_SetRenderDrawColor(window.ren, rect.color.r, rect.color.g, rect.color.b, rect.color.a);
    SDL_Rect sdlRect = {rect.pnt.x, rect.pnt.y, rect.size.w, rect.size.h};
    SDL_RenderFillRect(window.ren, &sdlRect);
}

void renderSprite(Window window, Sprite sprite)
{
    SDL_Rect rect = {sprite.pnt.x, sprite.pnt.y, sprite.size.w, sprite.size.h};
    SDL_RenderCopy(window.ren, sprite.texture, NULL, &rect);
}

void renderButton(Window window, Button button)
{
    renderRect(window, initRect(button.pnt.x, button.pnt.y, button.size.w, button.size.h, button.color));
    int textLengthInPixels;
    int textHeightInPixels;
    TTF_SizeText(button.font.font, button.str, &textLengthInPixels, &textHeightInPixels);
    renderText(window, initText(button.str, button.pnt.x + button.size.w / 2 - textLengthInPixels / 2, button.pnt.y + button.size.h / 2 - textHeightInPixels/2, button.length, button.textColor, button.font));
}

void renderMBox(Window window, MessageBoxRend msgBox, bool mouseDown, int mouseX, int mouseY)
{
    if(!msgBox.show)
        return;
    else {
        renderRect(window, msgBox.background);
        renderRect(window, msgBox.foreground);
        renderText(window, msgBox.title);
        renderText(window, msgBox.text);
        renderButton(window, msgBox.xButton);
        checkButtonClicked(msgBox.xButton, mouseDown, mouseX, mouseY, 0, NULL);
    }
}

void simpleCollisionDetector(Sprite* sprite1, Sprite* sprite2, int *xHit, int *yHit)
{
    if (sprite1->pnt.x < sprite2->pnt.x + sprite2->size.w &&
        sprite1->pnt.x + sprite1->size.w > sprite2->pnt.x &&
        sprite1->pnt.y < sprite2->pnt.y + sprite2->size.h &&
        sprite1->pnt.y + sprite1->size.h > sprite2->pnt.y)
    {
        *xHit = sprite1->pnt.x < sprite2->pnt.x + sprite2->size.w &&
                sprite1->pnt.x + sprite1->size.w > sprite2->pnt.x;
        *yHit = sprite1->pnt.y < sprite2->pnt.y + sprite2->size.h &&
                sprite1->pnt.y + sprite1->size.h > sprite2->pnt.y;
    }
}

void checkButtonClicked(Button button, bool mouseDown, int mouse_x, int mouse_y, int argc, char** argv)
{
    if (mouse_x > button.pnt.x && mouse_x < button.pnt.x + button.size.w &&
        mouse_y > button.pnt.y && mouse_y < button.pnt.y + button.size.h && mouseDown)
    {
        button.func(argc, argv);
    }
}