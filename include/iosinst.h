#pragma once

void loadIOSModules();
void installIOS(u8 slot, u8 newSlot, bool patches, bool nowifi);
s32 installWAD(WAD* Wad);
bool fileExists(const char* path);