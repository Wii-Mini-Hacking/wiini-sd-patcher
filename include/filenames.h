#pragma once

#define IOS36_WAD_PATH "/IOS36-64-3608.wad"
#define IOS58_WAD_PATH "/IOS58-64-6176.wad"
#define IOS80_WAD_PATH "/IOS80-64-6944.wad"
static const char* IOSWADPaths[3] = {IOS36_WAD_PATH, IOS58_WAD_PATH, IOS80_WAD_PATH};

#define SM_NTSC_PATH "/SystemMenu4.3U-v513.wad"
#define SM_PAL_PATH "/SystemMenu4.3E-v514.wad"
static const char* SMWADPaths[2] = {SM_NTSC_PATH, SM_PAL_PATH};

typedef struct {
	int version;
	int region;

} SMRegion;

static const SMRegion regionlist[] = {
	{33, 'X'},
	{128, 'J'}, {97, 'E'}, {130, 'P'},
	{162, 'P'},
	{192, 'J'}, {193, 'E'}, {194, 'P'},
	{224, 'J'}, {225, 'E'}, {226, 'P'},
	{256, 'J'}, {257, 'E'}, {258, 'P'},
	{288, 'J'}, {289, 'E'}, {290, 'P'},
	{352, 'J'}, {353, 'E'}, {354, 'P'}, {326, 'K'},
	{384, 'J'}, {385, 'E'}, {386, 'P'},
	{390, 'K'},
	{416, 'J'}, {417, 'E'}, {418, 'P'},
	{448, 'J'}, {449, 'E'}, {450, 'P'}, {454, 'K'},
	{480, 'J'}, {481, 'E'}, {482, 'P'}, {486, 'K'},
	{512, 'E'}, {513, 'E'}, {514, 'P'}, {518, 'K'},
	{4609, 'E'}, {4610, 'P'}, //Version numbers for Wii mini SM
	{4689, 'E'}, {4690, 'P'} //4.3 SM installed by Wii mini SD Patcher
};

