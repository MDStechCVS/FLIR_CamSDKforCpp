#pragma once
#include "global.h"

struct ColormapArray
{
	static const TCHAR* colormapStrings[];
};

enum PaletteTypes
{
    PALETTE_IRON = 0,
    PALETTE_RAINBOW = 1,
    PALETTE_JET = 2,
    PALETTE_INFER = 3,
    PALETTE_PLASMA = 4,
    PALETTE_RED_GRAY = 5,
    PALETTE_VIRIDIS = 6,
    PALETTE_MAGMA = 7,
    PALETTE_CIVIDIS = 8,
    PALETTE_COOLWARM = 9,
    PALETTE_SPRING = 10,
    PALETTE_SUMMER = 11
};

extern std::vector<std::string> iron_palette;
extern std::vector<std::string> Rainbow_palette;
extern std::vector<std::string> Jet_palette;
extern std::vector<std::string> Infer_palette;
extern std::vector<std::string> Plasma_palette;
extern std::vector<std::string> Red_gray_palette;
extern std::vector<std::string> Viridis_palette;
extern std::vector<std::string> Magma_palette;
extern std::vector<std::string> Cividis_palette;
extern std::vector<std::string> Coolwarm_palette;
extern std::vector<std::string> Spring_palette;
extern std::vector<std::string> Summer_palette;



