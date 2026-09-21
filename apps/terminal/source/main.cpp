#include <3ds.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("\x1b[2;15HINFINIT3 TERMINAL");
    printf("\x1b[4;12HNEW 3DS NATIVE T0");
    printf("\x1b[6;10Hlibctru skeleton online");
    printf("\x1b[9;12HPress START to exit");

    while (aptMainLoop())
    {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;
    }

    gfxExit();
    return 0;
}
