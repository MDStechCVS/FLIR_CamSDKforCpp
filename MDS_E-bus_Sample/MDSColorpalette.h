#pragma once
#include "global.h"

struct ColormapArray
{
	static const TCHAR* colormapStrings[];
};

enum class PaletteTypes
{
    PALETTE_IRON = 0,
    PALETTE_BOARDDETECTION,
    PALETTE_MDS,
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
    PALETTE_COOLDEPTH,
    PALETTE_HIGHCONTRASTJET,
    PALETTE_AUTUMNFIRE
};
using PALETTE_TYPE = PaletteTypes;


class PaletteManager
{
public:
    PaletteManager();
    void init(const std::string& baseDir);
    std::vector<cv::Vec3b> GetPalette(PALETTE_TYPE paletteType);

private:
    std::unordered_map<PALETTE_TYPE, std::vector<std::string>> palettes;
    void initPalettes(const std::string& baseDir);
    std::vector<std::string> loadPaletteFromFile(const std::string& baseDir, const std::string& filename);
    std::vector<cv::Vec3b> loadPaletteFromStrings(const std::vector<std::string>& paletteStrings);
};





