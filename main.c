#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

/*
 * (most significant first)
 * 1 -> is bomb
 * 1 -> discovered
 * 0  (unused)
 * 0  (unused)
 * next 4 bits are bomb count 
 * */
static uint8_t* GRID;
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
    return false;
}

void mscl_cleanup()
{
    free( GRID );
}


int main() 
{
    mscl_start( 8, 8, 0.2 );

    for (int i = 0; i < ROWS * COLUMNS; i++)
    {
        if (GRID[i] & (0x1 << 7)) {
            printf("#");
        } else {
            printf("%i", GRID[i]);
        }

        if ( i % COLUMNS == COLUMNS - 1 ) {
            printf("|\n");
        }
    }

    while ( mscl_loop() )
    {
    }

    mscl_cleanup();

    return 0;
}
