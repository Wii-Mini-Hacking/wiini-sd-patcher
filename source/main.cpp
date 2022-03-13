//Wii mini SD Patcher

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/machine/processor.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/es.h>
#include <ogc/ipc.h>
#include <fat.h>
#include <unistd.h>

#include "system.h"
#include "sha1.h"
#include "systitles.h"
#include "priiloader.h"
#include "debug.h"
#include "iosinst.h"
#include "haxx_certs.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MEM_PROT        0xD8B420A

static const unsigned char FSAccessPattern[] =
{
    0x9B, 0x05, 0x40, 0x03, 0x99, 0x05, 0x42, 0x8B,
};
static const unsigned char FSAccessPatch[] =
{
    0x9B, 0x05, 0x40, 0x03, 0x1C, 0x0B, 0x42, 0x8B,
};


extern "C" {
    extern void __exception_setreload(unsigned int t);
};

static bool apply_patch(char *name, const u8 *old, const u8 *patch, u32 size, bool enable)
{
    if(!enable)
    {
        const u8 *n = old;
        old = patch;
        patch = n;
    }
    u8 *ptr = (u8*)0x93400000;
    u32 found = 0;
    while((u32)ptr < (0x94000000 - size))
    {
        if(memcmp(ptr, old, size) == 0)
        {
            found++;
            u32 i;
            for(i = 0; i < size; i++)
                ptr[i] = patch[i];
            DCFlushRange(ptr, size);
        }
        ptr++;
    }
    printf("patched %s %u times.\n", name, found);
    return (found > 0);
}

const u8 isfs_perm_old[] = { 0x42, 0x8B, 0xD0, 0x01, 0x25, 0x66 };
const u8 isfs_perm_patch[] = { 0x42, 0x8B, 0xE0, 0x01, 0x25, 0x66 };
const u8 setuid_old[] = { 0xD1, 0x2A, 0x1C, 0x39 };
const u8 setuid_patch[] = { 0x46, 0xC0, 0x1C, 0x39 };
const u8 es_identify_old[] = { 0x28, 0x03, 0xD1, 0x23 };
const u8 es_identify_patch[] = { 0x28, 0x03, 0x00, 0x00 };
const u8 hash_old[] = { 0x20, 0x07, 0x23, 0xA2 };
const u8 hash_patch[] = { 0x20, 0x00, 0x23, 0xA2 };
const u8 new_hash_old[] = { 0x20, 0x07, 0x4B, 0x0B };
const u8 new_hash_patch[] = { 0x20, 0x00, 0x4B, 0x0B };

void PatchIOS(bool enable)
{
    /* Disable memory protection */
    write16((vu32)0xCD8B420A, 0);
    /* Do Patching */
    //apply_patch("isfs_permissions", isfs_perm_old, isfs_perm_patch, sizeof(isfs_perm_patch), enable);
    apply_patch("es_setuid", setuid_old, setuid_patch, sizeof(setuid_patch), enable);
    apply_patch("es_identify", es_identify_old, es_identify_patch, sizeof(es_identify_patch), enable);
    apply_patch("hash_check", hash_old, hash_patch, sizeof(hash_patch), enable);
    apply_patch("new_hash_check", new_hash_old, new_hash_patch, sizeof(new_hash_patch), enable);
    /* Enable memory protection */
    write16((vu32)0xCD8B420A, 1);
}

u64 bootTime = 0;

extern "C" {
    void __SYS_PreInit() {
        bootTime = gettime();
    }
    extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

    __exception_setreload(3);
	// Initialise the video system
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    DebugStart();
    printf("\n\n");
    printf("Wii mini SD Patcher Loading...\n");
    Debug("Wii mini SD Patcher Loading...\n");

    //Disables MEMPROT for patches
    write16(MEM_PROT, 0);
    //Patches FS access
    for(u32 u = 0x93A00000; u < 0x94000000; u+=2) {
        if(memcmp((void*)(u), FSAccessPattern, sizeof(FSAccessPattern)) == 0) {
            printf("FSAccessPatch:%08X\r\n", u);
            Debug("FSAccessPatch:%08X\r\n", u);
            memcpy((void*)u, FSAccessPatch, sizeof(FSAccessPatch));
            DCFlushRange((void*)u, sizeof(FSAccessPatch));
            break;
        }
    }

    #ifndef TEST_MODE //Define to disable IOS calls for dirty Dolphin testing of the UI
    printf("Initializing FAT\n");
    Debug("Initializing FAT\n");

    if (!initFAT()) {
        printf("Couldn't init FAT\n");
        Debug("Couldn't init FAT\n");
        exit(1);
    }

    PatchIOS(true);
    loadIOSModules();
    printf("\x1b[2;0HInitialization complete.");
    #endif

    // This function initialises the attached controllers
    PAD_Init();
    WPAD_Init();

    Debug("Starting install\n");
    bool nowifi = true;
    bool confirmInstall = false;
    do {
        printf("\x1b[2J");
        fflush(stdout);
        printf("\x1b[47m\x1b[31m\x1b[0;25HWii mini SD Patcher v1.0-Beta1\x1b[0;80H");
        printf("\x1b[5;1H\x1b[40m\x1b[37mPlease make sure the following files are present\nin the top-level directory of your USB drive:\n");
        //41 + fileExists(file) Sets the bgcolor to red if file is missing, green if present
        printf("\x1b[42m    IOS36-64-v3608.wad\n"); // these are checked for presence in loadIOSModules
        printf("\x1b[42m    IOS58-64-v6176.wad\n");
        printf("\x1b[42m    IOS80-64-v6944.wad\n");
        //printf("\x1b[%dm    0000000100000002v514.wad\n", 41 + fileExists("/0000000100000002v514.wad")); -- will be added in the future
        printf("\x1b[14;1H\x1b[40mThese files can be obtained from NUS using sharpii or NUSDownloader\n\n");
        printf("Start/Home: Run install process\tA+B: Exit\n");

        printf("\x1b[26;1HBased on RVLoader Installer v1.5 by Aurelio,\n without whose help this would have never been possible.");
        printf("\x1b[28;1HAdapted for the Wii mini Hacking community by Devnol.");
        printf("\x1b[29;1HPowered by devkitPPC, Divine Pizza and mediocre quality souvlaki.");

        
        

        // Option selector will be readded in future versions for other features
        //printf("\x1b[7;0HSelect the desired install options:");
        //printf("\x1b[14;0HRight/Left: Toggle option\tUp/Down: Move cursor\nStart/Home: Run install process\tA+B: Exit");

        //u8 cursor = 0;
        while (1) {
            PAD_ScanPads();
            WPAD_ScanPads();

            int down = PAD_ButtonsDown(0);
            int held = PAD_ButtonsHeld(0);
            int wDown = WPAD_ButtonsDown(0);
            int wHeld = WPAD_ButtonsHeld(0);

            /*if (down & PAD_BUTTON_DOWN || wDown & WPAD_BUTTON_DOWN) {
                if (cursor < 1)
                    cursor++;
            }

            if (down & PAD_BUTTON_UP || wDown & WPAD_BUTTON_UP) {
                if (cursor > 0)
                    cursor--;
            }

            if (down & PAD_BUTTON_LEFT || wDown & WPAD_BUTTON_LEFT || down & PAD_BUTTON_RIGHT || wDown & WPAD_BUTTON_RIGHT) {
                if (cursor == 0)
                    nowifi = !nowifi;
                if (cursor == 1)
                    vga = !vga;
            }*/

            if (down & PAD_BUTTON_START || wDown & WPAD_BUTTON_HOME)
                break;

            if ((held & PAD_BUTTON_A || wHeld & WPAD_BUTTON_A) && (held & PAD_BUTTON_B || wHeld & WPAD_BUTTON_B)) {
                printf("\x1b[2J");
                fflush(stdout);
                printf("\x1b[2;0HExitting...");
                exit(0);
            }

            //printf("\x1b[9;0H%c Patch out WiFi: %s", (cursor == 0) ? '>' : ' ', nowifi ? "Yes" : "No ");
            //printf("\x1b[10;0H%c Enable VGA: %s", (cursor == 1) ? '>' : ' ', vga ? "Yes" : "No ");

            VIDEO_WaitVSync();
        }

        printf("\x1b[2J");
        fflush(stdout);
        printf("\x1b[2;0HYou have selected:");
        printf("\x1b[4;0HInstall nowifi IOS for SD support : %s", nowifi ? "Yes" : "No ");
        printf("\x1b[7;0HHold the Start/Home button for 2 seconds to begin the install process.\nPress B to return to the previous screen.");

        u64 timer = gettime();

        while (1) {
            PAD_ScanPads();
            WPAD_ScanPads();

            int down = PAD_ButtonsDown(0);
            int held = PAD_ButtonsHeld(0);
            int wDown = WPAD_ButtonsDown(0);
            int wHeld = WPAD_ButtonsHeld(0);

            if (down & PAD_BUTTON_B || wDown & WPAD_BUTTON_B) {
                confirmInstall = false;
                break;
            }
            if (down & PAD_BUTTON_START || wDown & WPAD_BUTTON_HOME) {
                timer = gettime();
            }

            if ((held & PAD_BUTTON_START || wHeld & WPAD_BUTTON_HOME) && ticks_to_secs(gettime() - timer) >= 2) {
                confirmInstall = true;
                break;
            }
        }

    } while (!confirmInstall);

    printf("\x1b[2J");
    fflush(stdout);
    printf("\x1b[2;0HBeginning installation\n");

    //installIOS(36, 236, true, true); //Install a patched IOS36 with NoWiFi into slot 236
    //installIOS(58, 240, false, true); //Install a copy of IOS58 with NoWiFi into slot 240
    //installIOS(80, 241, false, true); //Install a copy of IOS80 with NoWiFi into slot 241

    //Install IOSes 36, 58, 80 with NoWiFi
    installIOS(36, 36, false, true);
    installIOS(58, 58, false, true);
    installIOS(80, 80, false, true);

    printf("Finished IOS install\n");
    printf("Exiting...");
    exit(0);

    //printf("Now installing Priiloader\n");
    //Install Priiloader
    //installPriiloader(0);

    while (1) {
        PAD_ScanPads();
        WPAD_ScanPads();

        int down = PAD_ButtonsDown(0);
        if (down & PAD_BUTTON_START) exit(0);

        VIDEO_WaitVSync();

    }

	return 0;
}

