#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <math.h>

#include "sprites.h"

/*
 * (most significant first)
 * 1 -> is bomb
 * 1 -> discovered
 * 0  (unused)
 * 0  (unused)
 * next 4 bits are bomb count 
 * */
static uint8_t *GRID;
static uint32_t ROWS;
static uint32_t COLUMNS;

void mscl_start( uint32_t rows, uint32_t columns, float density )
{
    srand(time(0));
    GRID    = malloc( rows * columns );
    ROWS    = rows;
    COLUMNS = columns;

    // initialize grid
    memset(GRID, 0, rows*columns);
    for ( uint32_t i = 0; i < rows*columns; i++)
    {
        uint8_t* block = &GRID[i];
        uint8_t  bomb  = rand() % 100 <= density * 100.0f ? 0x1 : 0x0;
        *block         = *block | (bomb << 7);
        
        if (bomb)
        {
            if ( i % COLUMNS != columns - 1)    { GRID[i + 1] += 1; }
            if ( i % COLUMNS != 0)              { GRID[i - 1] += 1; }

            if ( i / COLUMNS != ROWS - 1 ) 
            {
                if ( i % COLUMNS != columns -1) { GRID[i + COLUMNS + 1] += 1; }
                if ( i % COLUMNS != 0)          { GRID[i + COLUMNS - 1] += 1; }
                GRID[i + COLUMNS] += 1;
            }

            if ( i / COLUMNS != 0)
            {
                if ( i % COLUMNS != columns -1) { GRID[i - COLUMNS + 1] += 1; }
                if ( i % COLUMNS != 0)          { GRID[i - COLUMNS - 1] += 1; }

                GRID[i - COLUMNS] += 1;
            }
        }
    }
}

bool mscl_loop()
{
    return true;
}

void mscl_cleanup()
{
    free( GRID );
}

void mscl_debug_render_grid()
{
    for (int i = 0; i < ROWS * COLUMNS; i++)
    {
        if (GRID[i] & (0x1 << 7)) { printf("#"); } 
        else { printf("%i", GRID[i]); }

        if ( i % COLUMNS == COLUMNS - 1 ) { printf("|\n"); }
    }
}

static int C_SIZE_X;
static int C_SIZE_Y;

static float ZOOM, CAM_X, CAM_Y;

//(7x4)
#define TEXTURE_SIZE_X 7
#define TEXTURE_SIZE_Y 4

void draw_sprite(int32_t ix, int32_t iy, const char *texture)
{
    if (texture == SPRITE_BOMB)       { attron(A_BOLD                ); }
    if (texture == SPRITE_NUMBERS[1]) { attron(A_BOLD | COLOR_PAIR(1)); }
    if (texture == SPRITE_NUMBERS[2]) { attron(A_BOLD | COLOR_PAIR(2)); }
    if (texture == SPRITE_NUMBERS[3]) { attron(A_BOLD | COLOR_PAIR(3)); }
    if (texture == SPRITE_NUMBERS[4]) { attron(A_BOLD | COLOR_PAIR(4)); }
    if (texture == SPRITE_NUMBERS[5]) { attron(A_BOLD | COLOR_PAIR(5)); }
    if (texture == SPRITE_NUMBERS[6]) { attron(A_BOLD | COLOR_PAIR(6)); }
    if (texture == SPRITE_NUMBERS[7]) { attron(A_BOLD | COLOR_PAIR(7)); }
    if (texture == SPRITE_NUMBERS[8]) { attron(A_BOLD | COLOR_PAIR(8)); }

    for (float y = 0; floor(y) < TEXTURE_SIZE_Y; y += ZOOM)
    {
        for (float x = 0; floor(x) < TEXTURE_SIZE_X; x += ZOOM)
        {
            if (iy + y/ZOOM > C_SIZE_Y - 1 || ix + x / ZOOM > C_SIZE_X - 1 ||
                iy + y/ZOOM < 0            || ix + x / ZOOM < 0 ) { continue; }
            move(iy + floor(y / ZOOM), ix + floor(x / ZOOM));
            addch(texture[(int)floor(y)*TEXTURE_SIZE_X + (int)floor(x)]);
        }
    }

    attrset(A_NORMAL);
}

void draw_cell(uint32_t i)
{
    static int32_t x = 0;
    static int32_t y = 0;

    if (GRID[i] & (0x1 << 7)) 
    { 
        draw_sprite(
            x * TEXTURE_SIZE_X / ZOOM + C_SIZE_X / 2 - (int)roundf(CAM_X) - COLUMNS * (TEXTURE_SIZE_X / ZOOM) / 2, 
            y * TEXTURE_SIZE_Y / ZOOM + C_SIZE_Y / 2 + (int)roundf(CAM_Y) - ROWS    * (TEXTURE_SIZE_Y / ZOOM) / 2, 
            SPRITE_BOMB 
        );
    } 
    else 
    {
        draw_sprite(
            x * TEXTURE_SIZE_X / ZOOM + C_SIZE_X / 2 - (int)roundf(CAM_X) - COLUMNS * (TEXTURE_SIZE_X / ZOOM) / 2, 
            y * TEXTURE_SIZE_Y / ZOOM + C_SIZE_Y / 2 + (int)roundf(CAM_Y) - ROWS    * (TEXTURE_SIZE_Y / ZOOM) / 2, 
            SPRITE_NUMBERS[GRID[i]]
        );
    }

    x++;

    if ( i % COLUMNS == COLUMNS - 1 ) 
    {
        y++;
        x = 0;
    }

    if ( i == COLUMNS * ROWS - 1) 
    {
        y = 0;
        x = 0;
    }
}

int main() 
{
    mscl_start( 8, 8, 0.2 );
    initscr(); cbreak(); noecho();
    start_color(); keypad(stdscr, TRUE);
    getmaxyx(stdscr, C_SIZE_Y, C_SIZE_X);

    /* Color pairs */
    init_pair(1, COLOR_CYAN,    COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_RED,     COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_BLUE,    COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(8, COLOR_RED,     COLOR_BLACK);

    ZOOM  = 1.0f; CAM_X = 0.0f; CAM_Y = 0.0f;

    while ( mscl_loop() )
    {
        /* render */
        clear();
        for (int i = 0; i < ROWS * COLUMNS; i++)
        {
            draw_cell(i);
        }
        border(0,0,0,0,0,0,0,0);
        mvprintw(0,0, "z: %+.1f p: (%+.1f, %+.1f)", ZOOM, CAM_X, CAM_Y);
        refresh();

        int kinput = getch();
        if ( kinput == 'x') { ZOOM += 0.06f; }
        if ( kinput == 'c') { ZOOM = ZOOM - 0.06f <= 0.1f ? 0.06f : ZOOM - 0.06f; }
        if ( kinput == 'l') { CAM_X += 0.5f / ZOOM; }
        if ( kinput == 'h') { CAM_X -= 0.5f / ZOOM; }
        if ( kinput == 'j') { CAM_Y -= 0.5f / ZOOM; }
        if ( kinput == 'k') { CAM_Y += 0.5f / ZOOM; }
        if ( kinput == 'q') { break; }
    }

    mscl_cleanup();
    endwin();
    return 0;
}
