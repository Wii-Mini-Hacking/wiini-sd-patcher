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
#include "filenames.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MEM_PROT        0xD8B420A

WAD SMWad;

extern "C" {
    extern void udelay(int us);
};

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


    printf("Initializing FAT\n");
    Debug("Initializing FAT\n");

    if (!initFAT()) {
        printf("Couldn't init FAT\n");
        Debug("Couldn't init FAT\n");
        exit(1);
    }

    // This function initialises the attached controllers
    PAD_Init();
    WPAD_Init();

    PatchIOS(true);
    udelay(5000000); //Naively wait for wpad sync
    printf("\x1b[2;0HInitialization complete.");

    SMRegion InstalledSM = GetSM();

    printf("Installed SM Version: %d\n", InstalledSM.version);
    int SM_REGION; //0 for NTSC, 1 for PAL

    PAD_ScanPads();
    WPAD_ScanPads();

    if ((InstalledSM.version == 4609) || (InstalledSM.version == 4689)){SM_REGION = 0;}
    else if ((InstalledSM.version == 4610) || (InstalledSM.version == 4690)){SM_REGION = 1;}
    else if (!((PAD_ButtonsHeld(0) & PAD_BUTTON_X || WPAD_ButtonsHeld(0) & WPAD_BUTTON_1) && (PAD_ButtonsHeld(0) & PAD_BUTTON_Y || WPAD_ButtonsHeld(0) & WPAD_BUTTON_A))){
        printf("\x1b[47m\x1b[31mWarning: running this homebrew on a non-mini Wii\nis unneeded and can have unintended side effects.\n");
        printf("If you want to proceed anyway, open the app again and hold 1+A/x+y on startup.\n");
        printf("exiting...\n");
        udelay(10000000);
        exit(0);
    } 
    else if (InstalledSM.region == 0x45){SM_REGION = 0;} //generalise to all pal/ntsc consoles if skip on check
    else if (InstalledSM.region  == 0x50){SM_REGION = 1;}
    else {
        printf("Error: Unknown region. Please contact the developers.\n");
        printf("This homebrew does not support Japanese or Korean consoles.\n");
        printf("exiting...\n");
        udelay(5000000);
        exit(0); 
    }

    Debug("Starting install\n");
    bool Install_IOS = true;
    bool Install_SM = true;
    bool Install_Prii = true;
    bool confirmInstall = false;


    do {
        printf("\x1b[2J");
        fflush(stdout);
        printf("\x1b[47m\x1b[31m\x1b[1;25HWii mini SD Patcher v1.0\x1b[0;80H");
        printf("\x1b[5;1H\x1b[40m\x1b[37mPlease make sure the following files are present\nin the top-level directory of your USB drive:\n");
        
        printf("\x1b[%dm    %s\n", 41 + fileExists(IOSWADPaths[0]), IOSWADPaths[0]); //41 + fileExists(file) Sets the bgcolor to red if file is missing, green if present
        printf("\x1b[%dm    %s\n", 41 + fileExists(IOSWADPaths[1]), IOSWADPaths[1]);
        printf("\x1b[%dm    %s\n", 41 + fileExists(IOSWADPaths[2]), IOSWADPaths[2]);
        printf("\x1b[%dm    %s\n", 41 + fileExists(SMWADPaths[SM_REGION]), SMWADPaths[SM_REGION]);
        printf("\x1b[11;1H\x1b[40mThese files can be obtained from NUS using sharpii-netcore or another program.\n\n");
        printf("\x1b[14;0HSelect the desired install options:");
        printf("\x1b[15;0HRight/Left: Toggle option\tUp/Down: Move cursor\nStart/Home: Run install process\tA+B: Exit");
        //printf("Start/Home: Run install process\tA+B: Exit\n");

        printf("\x1b[24;1HBased on RVLoader Installer v1.5 by Aurelio,\n without whose help this would have never been possible.");
        printf("\x1b[26;1HAdapted for the Wii mini Hacking community by Devnol.");
        printf("\x1b[27;1HPowered by devkitPPC, Divine Pizza and mediocre quality souvlaki.");

        u8 cursor = 0;
        while (1) {
            PAD_ScanPads();
            WPAD_ScanPads();

            int down = PAD_ButtonsDown(0);
            int held = PAD_ButtonsHeld(0);
            int wDown = WPAD_ButtonsDown(0);
            int wHeld = WPAD_ButtonsHeld(0);

            if (down & PAD_BUTTON_DOWN || wDown & WPAD_BUTTON_DOWN) {
                if ((cursor < 2 && !Install_SM) || cursor < 1)  { //don't allow disabling IOS patch if install sm is selected
                    cursor++;
                }
            }

            if (down & PAD_BUTTON_UP || wDown & WPAD_BUTTON_UP) {
                if (cursor > 0)
                    cursor--;
            }

            if (down & PAD_BUTTON_LEFT || wDown & WPAD_BUTTON_LEFT || down & PAD_BUTTON_RIGHT || wDown & WPAD_BUTTON_RIGHT) {
                if (cursor == 0) {
                    Install_SM = !Install_SM;
                    Install_IOS = (Install_SM || 1); //forbid installing sm without ios
                }
                if (cursor == 1)
                    Install_Prii = !Install_Prii;
                if (cursor == 2)
                    Install_IOS = !Install_IOS;
            }

            if (down & PAD_BUTTON_START || wDown & WPAD_BUTTON_HOME)
                break;

            if ((held & PAD_BUTTON_A || wHeld & WPAD_BUTTON_A) && (held & PAD_BUTTON_B || wHeld & WPAD_BUTTON_B)) {
                printf("\x1b[2J");
                fflush(stdout);
                printf("\x1b[2;0HExitting...");
                exit(0);
            }
            printf("\x1b[17;0H%c Install System Menu: %s", (cursor == 0) ? '>' : ' ', Install_SM ? "Yes" : "No ");
            printf("\x1b[18;0H%c Install Priiloader: %s", (cursor == 1) ? '>' : ' ', Install_Prii ? "Yes" : "No ");
            printf("\x1b[19;0H%c Install IOS with SD support: %s", (cursor == 2) ? '>' : ' ', Install_IOS ? "Yes" : "No ");

            VIDEO_WaitVSync();
        }

        printf("\x1b[2J");
        fflush(stdout);
        printf("\x1b[2;0HYou have selected:");
        printf("\x1b[4;0HInstall System Menu : %s", Install_SM ? "Yes" : "No ");
        printf("\x1b[5;0HInstall Priiloader : %s", Install_Prii ? "Yes" : "No ");
        printf("\x1b[6;0HInstall IOS with SD support: %s", Install_IOS ? "Yes" : "No ");
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

    if (Install_IOS) {
        loadIOSModules();
        installIOS(36, 236, true, true); //Install a patched IOS36 with NoWiFi into slot 236
        //Install IOSes 36, 58, 80 with NoWiFi
        installIOS(36, 36, false, true);
        installIOS(58, 58, false, true);
        installIOS(80, 80, false, true);
        printf("Finished IOS install\n");
        if (Install_SM) { //For you own good you can't install the Wii SM without Wii IOSes because funny brick will occur
            printf("Installing System Menu\n");
            if (openWAD(SMWADPaths[SM_REGION], &SMWad)) {
                //Change title version to avoid error -1035, also helpful for distinguishability
                if (SMWad.tmd->TitleVersion == 513) { //if SM is NTSC set version to v4689
                    SMWad.tmd->TitleVersion = 4689; 
                } else if (SMWad.tmd->TitleVersion == 514) { //if PAL set version to v4690
                    SMWad.tmd->TitleVersion = 4690;
                } else {
                    printf("Invalid System Menu wad, please use v4.3U/E\n");
                    udelay(3000000);
                    exit(0);
                }
                s32 ret = installWAD(&SMWad);
                if (ret != 0) {
                    printf("Something VERY bad happened. Manually reinstall the System Menu IMMEDIATELY!!\nIf you exit the hbc without installing the SM your console WILL be permabricked!\n");
                    printf("Priiloader will now be installed as a last ditch effort to\nSave anything that can be saved. Please wait for it to complete,\nthen reinstall your System Menu using a WAD manager.\n");
                    udelay(10000000);
                    installPriiloader(); //forcibly install priiloader as a last line of defense against the eternal doorstopification
                }
            } else {
                printf("Error loading System Menu WAD\n");
                udelay(3000000);
                exit(0);
            }
            
        }
    }


    //Install Priiloader
    if (Install_Prii) {
        printf("Now installing Priiloader\n");
        installPriiloader();
    }

    printf("Exitting...");
    exit(0);

    while (1) {
        PAD_ScanPads();
        WPAD_ScanPads();

        int down = PAD_ButtonsDown(0);
        if (down & PAD_BUTTON_START) exit(0);

        VIDEO_WaitVSync();

    }

	return 0;
}

