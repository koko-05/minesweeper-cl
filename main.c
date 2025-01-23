#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <math.h>

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

static int C_SIZE_X;
static int C_SIZE_Y;
static int WINSTATE = 0; // 0 is playing, 1 wins, -1 loses
static float ZOOM, CAM_X, CAM_Y;


void lose_animation();
void win_animation();

//(7x4)
#define TEXTURE_SIZE_X 7
#define TEXTURE_SIZE_Y 4
static const char *SPRITE_BOMB = "\
._____.\
| ### |\
| ### |\
|_____|";
static const char *SPRITE_UNDISCOVERED = "\
._____.\
|/////|\
|/////|\
|_____|";
static const char *SPRITE_NUMBERS[9] = 
{
"\
._____.\
|     |\
|     |\
|_____|",
"\
._____.\
| /|  |\
| _|_ |\
l_____|",
"\
.-==--.\
| __| |\
| |__ |\
l_____|",
"\
.-__--.\
|  _| |\
| __/ |\
l_____|",
"\
._____.\
| |_| |\
|   | |\
l_____|",
"\
.--__-.\
| |   |\
| _\\  |\
l_____|",
"\
.__--_.\
| |__ |\
| \\__/|\
l_____|",
"\
.--__-.\
|  _/ |\
|  /  |\
l_____|",
"\
.--_--.\
| |_| |\
| |_| |\
l-----|"
};

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


bool mscl_discover_cell( int i, bool softpass )
{
    if ( GRID[i] & (0x1 << 6))       { return false; }
    if (i < 0 || i > COLUMNS*ROWS-1) { return false; }
    if ( (GRID[i] & (0x1 << 7)) )    { return !softpass;  }


    GRID[i] |= 0x1 << 6;

    if ( softpass && (GRID[i] & ~(0xC0)) != 0) { return false; }

    if ( (GRID[i] & ~(0xC0)) == 0 )
    {
        mscl_discover_cell( i + 1,           true);
        mscl_discover_cell( i - 1,           true);
        mscl_discover_cell( i + COLUMNS + 1, true);
        mscl_discover_cell( i + COLUMNS - 1, true);
        mscl_discover_cell( i + COLUMNS,     true);
        mscl_discover_cell( i - COLUMNS + 1, true);
        mscl_discover_cell( i - COLUMNS - 1, true);
        mscl_discover_cell( i - COLUMNS,     true);
    }
    return false;
}

bool mscl_check_win()
{
    for (int i = 0; i < ROWS*COLUMNS; i++)
    {
        if ( !(GRID[i] & (0x1 << 7)) && !(GRID[i] & (0x1 << 6)))
        {
            return false;
        }
    }
    return true;
}

bool mscl_on_input( int cx, int cy )
{
    const float x      = (CAM_X + (cx - C_SIZE_X/2)*ZOOM) + (COLUMNS/2)*TEXTURE_SIZE_X;
    const float y      = (-CAM_Y + (cy - C_SIZE_Y/2)*ZOOM) + (ROWS/2)*(TEXTURE_SIZE_Y);
    const int   index  = abs(y / TEXTURE_SIZE_Y) * COLUMNS + x / TEXTURE_SIZE_X;
    
    //mvprintw(5,10, "c(%i, %i) -> i: %i\n (%f,%f)", cx, cy, index, x, y );
    //refresh();
    //sleep(7);

    if ( mscl_discover_cell(index, false) ) { WINSTATE = -1; return true; }
    if ( mscl_check_win() ) { WINSTATE = 1; return true; }
    return false;
}

bool mscl_loop()
{
    int kinput = getch();
    
    // inputs, fuck switch statements
    if ( kinput == 'x') { ZOOM += 0.05f; }
    if ( kinput == 'c') { ZOOM = ZOOM - 0.05f <= 0.1f ? 0.1f : ZOOM - 0.05f; }
    if ( kinput == 'd') { CAM_X += 1.5f * ZOOM; }
    if ( kinput == 'a') { CAM_X -= 1.5f * ZOOM; }
    if ( kinput == 's') { CAM_Y -= 1.5f * ZOOM; }
    if ( kinput == 'w') { CAM_Y += 1.5f * ZOOM; }
    if ( kinput == '0') { CAM_Y = 0.0f; CAM_X = 0.0f; ZOOM = 1.0f; }
    if ( kinput == 'q') { return false; }
    if ( kinput == KEY_MOUSE )
    {
        MEVENT e;
        if (getmouse(&e) == OK) {
            if ( e.bstate & BUTTON1_PRESSED )
            {
                return !mscl_on_input(e.x, e.y);
            }
        }
    }

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
        else { printf("%i", GRID[i] ^ 0x40); }

        if ( i % COLUMNS == COLUMNS - 1 ) { printf("|\n"); }
    }
}



void draw_sprite(int32_t ix, int32_t iy, const char *texture, int32_t tsx, int32_t tsy, int color_override )
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
    if (color_override != -1) { attron(A_BOLD | COLOR_PAIR(color_override)); }

    for (float y = 0; floor(y) < tsy; y += ZOOM)
    {
        for (float x = 0; floor(x) < tsx; x += ZOOM)
        {
            if (iy + floor(y/ZOOM) > C_SIZE_Y - 1 || ix + floor(x / ZOOM) > C_SIZE_X - 1 ||
                iy + y/ZOOM < 0            || ix + x / ZOOM < 0 ) { continue; }
            move(iy + roundf(y / ZOOM), ix + roundf(x / ZOOM)); // im not sure why, but rounding here fixes things
            char ch = texture[(int)floor(y)*tsx + (int)floor(x)];
            if ( ch != ' ' ) {addch(ch);}
        }
    }

    attrset(A_NORMAL);
}

void draw_cell(uint32_t i)
{
    static int32_t x = 0;
    static int32_t y = 0;

    const int32_t rts_x = floorf(TEXTURE_SIZE_X / ZOOM);
    const int32_t rts_y = floorf(TEXTURE_SIZE_Y / ZOOM);

    if (!(GRID[i] & (0x1 << 6)))
    {
        draw_sprite(
            x * rts_x + C_SIZE_X / 2 - (int)roundf(CAM_X / ZOOM) - COLUMNS * rts_x / 2, 
            y * rts_y + C_SIZE_Y / 2 + (int)roundf(CAM_Y / ZOOM) - ROWS    * rts_y / 2, 
            SPRITE_UNDISCOVERED,
            TEXTURE_SIZE_X,
            TEXTURE_SIZE_Y, -1
        );
    }
    else if (GRID[i] & (0x1 << 7)) 
    { 
        draw_sprite(
            x * rts_x + C_SIZE_X / 2 - (int)roundf(CAM_X / ZOOM) - COLUMNS * rts_x / 2, 
            y * rts_y + C_SIZE_Y / 2 + (int)roundf(CAM_Y / ZOOM) - ROWS    * rts_y / 2, 
            SPRITE_BOMB,
            TEXTURE_SIZE_X,
            TEXTURE_SIZE_Y, -1
        );
    } 
    else 
    {
        draw_sprite(
            x * rts_x + C_SIZE_X / 2 - (int)roundf(CAM_X / ZOOM) - COLUMNS * rts_x / 2, 
            y * rts_y + C_SIZE_Y / 2 + (int)roundf(CAM_Y / ZOOM) - ROWS    * rts_y / 2, 
            SPRITE_NUMBERS[GRID[i] ^ 0x40],
            TEXTURE_SIZE_X,
            TEXTURE_SIZE_Y, -1
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

void win_animation()
{
    const char *smiley = "\
    #|     |#       \
    #|     |#       \
\\#  #|     |#   #/  \
|#              #|  \
 \\#            #/   \
  \\###########/     ";

   ZOOM = 0.2f;
   draw_sprite(C_SIZE_X/8, 0, smiley, 20, 6, 3);
   refresh();
}

void lose_animation()
{
    const char *no_smile = "\
    #|     |#       \
    #|     |#       \
    #|     |#       \
  #           #     \
    _------_   #    \
   /        \\       ";

   ZOOM = 0.25f;
   draw_sprite(C_SIZE_X/8, 5, no_smile, 20, 6, 6);
   refresh();
}

int main(int argc, char* argv[]) 
{
    float density = 0.2f;
    if (argc > 1) {
        if (strcmp(argv[1], "--density") || strcmp(argv[1], "-d"))
        {
            density = atof(argv[2]);
        }
    }

    mscl_start( 8, 8, density );
    initscr(); cbreak(); noecho();
    start_color(); keypad(stdscr, TRUE);
    getmaxyx(stdscr, C_SIZE_Y, C_SIZE_X);
    mousemask(BUTTON1_PRESSED, NULL);

    /* Color pairs */
    init_pair(1, COLOR_CYAN,    COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_RED,     COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_BLUE,    COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(8, COLOR_RED,     COLOR_BLACK);

    ZOOM = 1.0f; CAM_X = 0.0f; CAM_Y = 0.0f;

    do 
    {
        /* render */
        clear();
        for (int i = 0; i < ROWS * COLUMNS; i++)
        {
            draw_cell(i);
        }
        border(0,0,0,0,0,0,0,0);
        mvprintw(0,0, "z: %+.1f p: (%+.1f, %+.1f) d: %f", ZOOM, CAM_X, CAM_Y, density);
        refresh();
    } while ( mscl_loop() );

    clear();
    for (int i = 0; i < ROWS * COLUMNS; i++)
    {
        GRID[i] |= 0x1 << 6;
        draw_cell(i);
    }
    border(0,0,0,0,0,0,0,0);
    refresh();

    if ( WINSTATE == -1 ) { lose_animation(); }
    else if ( WINSTATE ==  1 ) { win_animation();  }
    sleep(3);

    mscl_cleanup();
    endwin();
    return 0;
}
