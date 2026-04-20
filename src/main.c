#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Standard Pokémon LCG
uint32_t LCG(uint32_t seed) {
    return (0x41C64E6D * seed + 0x00006073);
}

// The Cracker Function
int crack() {
    uint8_t desiredNature = 15; // 15 = Modest in Gen 4
    printf("Starting 'Holy Grail' Search...\n");
    printf("Target: 6x31 IVs, Modest, Shiny\n");
    printf("Press HOME to exit to Loader.\n\n");

    // Outer Loop: Search all 4.29 Billion Encounter Seeds
    for (uint64_t i = 0; i <= 0xFFFFFFFF; i++) {
        uint32_t state = (uint32_t)i;

        // Pull PID Low then PID High (Method 1)
        state = LCG(state);
        uint16_t p_low = (uint16_t)(state >> 16);
        state = LCG(state);
        uint16_t p_high = (uint16_t)(state >> 16);
        uint32_t PID = (p_high << 16) | p_low;

        // 1. Check Nature (PID % 25)
        if ((PID % 25) == desiredNature) {
            // 2. Pull IVs
            state = LCG(state);
            uint16_t iv1 = (uint16_t)(state >> 16);
            state = LCG(state);
            uint16_t iv2 = (uint16_t)(state >> 16);

            // 3. Check for 6x31 IVs (0x7FFF = three 5-bit stats of 31)
            if (iv1 == 0x7FFF && iv2 == 0x7FFF) {
                printf("\n[!] Perfect PID Found: %08X\n", PID);
                printf("    Seed: %08X\n", (uint32_t)i);
                printf("    Searching for matching IDs...\n");

                uint16_t p_xor = (uint16_t)(p_high ^ p_low);

                // Inner Loop: Search for Trainer ID seed to make this PID shiny
                for (uint64_t j = 0; j <= 0xFFFFFFFF; j++) {
                    uint32_t id_state = (uint32_t)j;
                    
                    id_state = LCG(id_state);
                    uint16_t tTID = (uint16_t)(id_state >> 16);
                    id_state = LCG(id_state);
                    uint16_t tSID = (uint16_t)(id_state >> 16);

                    if (((tTID ^ tSID) ^ p_xor) < 8) {
                        printf("\n!!!! HOLY GRAIL FOUND !!!!\n");
                        printf("TID Seed: %08X\n", (uint32_t)j);
                        printf("TID: %u | SID: %u\n", tTID, tSID);
                        printf("Encounter Seed: %08X\n", (uint32_t)i);
                        printf("PID: %08X\n", PID);
                        return 0; // Success
                    }

                    // Wii Fix: Don't lock up during ID search
                    if (j % 50000000 == 0) {
                        WPAD_ScanPads();
                        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
                    }
                }
            }
        }

        // Wii Fix: Update progress and check for HOME button every 10M iterations
        if (i % 10000000 == 0) {
            printf(".");
            WPAD_ScanPads();
            if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
            
            // Periodically flush video to keep the screen alive
            VIDEO_WaitVSync();
        }
    }

    printf("\nSearch complete. No perfect match exists in 32-bit space.\n");
    return 0;
}

int main(int argc, char **argv) {
    // Initialise the video system
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Move cursor and start
    printf("\x1b[2;0H");
    printf("Wii RNG Cracker v1.0\n");
    printf("--------------------\n");

    crack();

    printf("\nPress HOME to exit.\n");

    while(SYS_MainLoop()) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) exit(0);
        VIDEO_WaitVSync();
    }

    return 0;
}