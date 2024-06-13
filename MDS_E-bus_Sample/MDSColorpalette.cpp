#include "MDSColorpalette.h"

// 컬러맵 초기 데이터

const TCHAR* ColormapArray::colormapStrings[] =
{
    _T("Iron"),
    _T("BoardDetection"),
    _T("MDS"),
    _T("Rainbow"),
    _T("Arctic"),
    _T("Jet"),
    _T("Infer"),
    _T("Plasma"),
    _T("Red&Gray"),
    _T("Viridis"),
    _T("Magma"),
    _T("Cividis"),
    _T("Coolwarm"),
    _T("CoolDepth"),
    _T("HighContrastJet"),
    _T("AutumnFire"),

    nullptr
};

#define PALETTESIZE 1024 



// =============================================================================
PaletteManager::PaletteManager()
    : palettes(
        {
        {PALETTE_TYPE::PALETTE_IRON, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_BOARDDETECTION, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_MDS, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_RAINBOW, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_ARCTIC, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_JET, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_INFER, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_PLASMA, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_RED_GRAY, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_VIRIDIS, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_MAGMA, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_CIVIDIS, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_COOLWARM, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_COOLDEPTH, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_HIGHCONTRASTJET, std::vector<std::string>()},
        {PALETTE_TYPE::PALETTE_AUTUMNFIRE, std::vector<std::string>()}
        })
{

}

// =============================================================================
void PaletteManager::init(const std::string& baseDir)
{
    initPalettes(baseDir);
}

// =============================================================================
void PaletteManager::initPalettes(const std::string& baseDir)
{
    palettes[PALETTE_TYPE::PALETTE_IRON] = loadPaletteFromFile(baseDir, "paletteType_Iron.txt");
    palettes[PALETTE_TYPE::PALETTE_BOARDDETECTION] = loadPaletteFromFile(baseDir, "paletteType_BoardDetection.txt");
    palettes[PALETTE_TYPE::PALETTE_MDS] = loadPaletteFromFile(baseDir, "paletteType_MDS.txt");
    palettes[PALETTE_TYPE::PALETTE_RAINBOW] = loadPaletteFromFile(baseDir, "paletteType_Rainbow.txt");
    palettes[PALETTE_TYPE::PALETTE_ARCTIC] = loadPaletteFromFile(baseDir, "paletteType_Arctic.txt");
    palettes[PALETTE_TYPE::PALETTE_JET] = loadPaletteFromFile(baseDir, "paletteType_Jet.txt");
    palettes[PALETTE_TYPE::PALETTE_INFER] = loadPaletteFromFile(baseDir, "paletteType_Infer.txt");
    palettes[PALETTE_TYPE::PALETTE_PLASMA] = loadPaletteFromFile(baseDir, "paletteType_Plasma.txt");
    palettes[PALETTE_TYPE::PALETTE_RED_GRAY] = loadPaletteFromFile(baseDir, "paletteType_RedGray.txt");
    palettes[PALETTE_TYPE::PALETTE_VIRIDIS] = loadPaletteFromFile(baseDir, "paletteType_Viridis.txt");
    palettes[PALETTE_TYPE::PALETTE_MAGMA] = loadPaletteFromFile(baseDir, "paletteType_Magma.txt");
    palettes[PALETTE_TYPE::PALETTE_CIVIDIS] = loadPaletteFromFile(baseDir, "paletteType_Cividis.txt");
    palettes[PALETTE_TYPE::PALETTE_COOLWARM] = loadPaletteFromFile(baseDir, "paletteType_Coolwarm.txt");
    palettes[PALETTE_TYPE::PALETTE_COOLDEPTH] = loadPaletteFromFile(baseDir, "paletteType_CoolDepth.txt");
    palettes[PALETTE_TYPE::PALETTE_HIGHCONTRASTJET] = loadPaletteFromFile(baseDir, "paletteType_HighContrastJet.txt");
    palettes[PALETTE_TYPE::PALETTE_AUTUMNFIRE] = loadPaletteFromFile(baseDir, "paletteType_AutumnFire.txt");
}

// =============================================================================
std::vector<std::string> PaletteManager::loadPaletteFromFile(const std::string& baseDir, const std::string& filename)
{
    std::vector<std::string> paletteStrings;
    std::ifstream file(baseDir + "\\" + filename);
    std::string line;
    while (std::getline(file, line))
    {
        paletteStrings.push_back(line);
    }
    return paletteStrings;
}

// =============================================================================
std::vector<cv::Vec3b> PaletteManager::loadPaletteFromStrings(const std::vector<std::string>& paletteStrings)
{
    std::vector<cv::Vec3b> palette;
    for (const std::string& line : paletteStrings)
    {
        std::istringstream ss(line);
        int r, g, b;
        char comma;
        if (ss >> r >> comma >> g >> comma >> b)
        {
            palette.push_back(cv::Vec3b(b, g, r)); // OpenCV uses BGR format
        }
    }
    return palette;
}

// =============================================================================
std::vector<cv::Vec3b> PaletteManager::GetPalette(PALETTE_TYPE paletteType)
{
    auto it = palettes.find(paletteType);
    if (it != palettes.end())
    {
        return loadPaletteFromStrings(it->second);
    }
    return std::vector<cv::Vec3b>(); // 기본값 또는 오류 처리를 수행할 수 있음
}

// =============================================================================
std::vector<std::string> compressIronPalette(const std::vector<std::string>& originalPalette, int compressionFactor)
{
    std::vector<std::string> compressedPalette;
    int step = originalPalette.size() / compressionFactor;

    for (int i = 0; i < originalPalette.size(); i += step) 
    {
        compressedPalette.push_back(originalPalette[i]);
    }

    return compressedPalette;
}