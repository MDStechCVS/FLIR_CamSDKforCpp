#pragma once
#include "global.h"

struct ColormapArray
{
	static const TCHAR* colormapStrings[];
};

enum PaletteTypes
{
    PALETTE_IRON = 0,
    PALETTE_RAINBOW,
    PALETTE_ARCTIC,
    PALETTE_JET,
    PALETTE_INFER,
    PALETTE_PLASMA,
    PALETTE_RED_GRAY,
    PALETTE_VIRIDIS,
    PALETTE_MAGMA,
    PALETTE_CIVIDIS,
    PALETTE_COOLWARM,
    PALETTE_SPRING,
    PALETTE_SUMMER
};

extern std::vector<std::string> iron_palette;
extern std::vector<std::string> Rainbow_palette;
extern std::vector<std::string> Arctic_palette;
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



