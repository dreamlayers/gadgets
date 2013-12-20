/* Raw font dumper tool for the LED sign. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdio.h>

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
# define SET_BINARY_MODE(handle) setmode(handle, O_BINARY)
#else
# define SET_BINARY_MODE(handle) ((void)0)
#endif

int main(void) {
    int ascii = 0;
    int line = 0;

    SET_BINARY_MODE(stdin);
    while (!feof(stdin)) {
        unsigned char i;
        int c = getchar();
        if (c < 0) break;
        printf("%03i,%i: ", ascii, line);
        for (i = 0x80; i != 0; i >>= 1) {
            if (c & i) {
                putchar('*');
            } else {
                putchar(' ');
            }
        }
        printf("\n");
        line++;
        if (line == 7) {
            line = 0;
            ascii++;
            printf("\n");
        }
    }
    return 0;
}
