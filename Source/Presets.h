#pragma once
#include <utility>
#include <vector>

// Préréglages d'usine — valeurs réelles (dB, Hz, %, etc.). Les paramètres absents
// reprennent leur valeur par défaut.
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        { "00 Init - Transparent", {} },

        { "01 Hip-Hop Loud",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 25 },
            { "lsFreq", 70 }, { "lsGain", 1.5f }, { "b1Freq", 300 }, { "b1Gain", -1.0f }, { "b1Q", 1.2f },
            { "b3Freq", 4000 }, { "b3Gain", 1.0f }, { "hsFreq", 11000 }, { "hsGain", 1.5f },
            { "mbOn", 1 }, { "loThr", -18 }, { "loRatio", 2.5f }, { "midThr", -20 }, { "midRatio", 2.0f }, { "hiThr", -22 }, { "hiRatio", 2.0f },
            { "satOn", 1 }, { "satType", 0 }, { "satDrive", 6 }, { "satMix", 40 },
            { "excOn", 1 }, { "excSub", 30 }, { "excAir", 15 },
            { "imgOn", 1 }, { "imgWidth", 110 }, { "monoBassOn", 1 }, { "monoBassFreq", 120 },
            { "clipOn", 1 }, { "clipDrive", 2.0f }, { "clipCeiling", -0.5f }, { "clipKnee", 50 },
            { "limOn", 1 }, { "limGain", 6.0f }, { "limCeiling", -0.3f }, { "limRelease", 60 } } },

        { "02 Trap 808 Punch",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 22 }, { "lsFreq", 55 }, { "lsGain", 2.0f },
            { "b1Freq", 250 }, { "b1Gain", -1.5f }, { "b1Q", 1.0f }, { "hsFreq", 12000 }, { "hsGain", 1.0f },
            { "mbOn", 1 }, { "xLow", 140 }, { "loThr", -16 }, { "loRatio", 3.0f }, { "loAtt", 25 }, { "loRel", 120 },
            { "excOn", 1 }, { "excSub", 55 }, { "excAir", 10 },
            { "imgOn", 1 }, { "monoBassOn", 1 }, { "monoBassFreq", 150 },
            { "clipOn", 1 }, { "clipDrive", 3.0f }, { "clipCeiling", -0.5f }, { "clipKnee", 40 },
            { "limOn", 1 }, { "limGain", 7.0f }, { "limCeiling", -0.3f }, { "limRelease", 50 } } },

        { "03 Gospel Warm & Wide",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 28 }, { "lsFreq", 90 }, { "lsGain", 1.0f },
            { "b1Freq", 400 }, { "b1Gain", -1.5f }, { "b1Q", 0.9f }, { "b3Freq", 3500 }, { "b3Gain", 0.8f },
            { "hsFreq", 12000 }, { "hsGain", 2.0f },
            { "mbOn", 1 }, { "loRatio", 2.0f }, { "midRatio", 1.8f }, { "hiRatio", 1.6f },
            { "satOn", 1 }, { "satType", 1 }, { "satDrive", 5 }, { "satMix", 50 },
            { "excOn", 1 }, { "excSub", 15 }, { "excAir", 25 },
            { "imgOn", 1 }, { "imgWidth", 125 }, { "monoBassOn", 1 }, { "monoBassFreq", 110 },
            { "limOn", 1 }, { "limGain", 4.0f }, { "limCeiling", -1.0f }, { "limRelease", 120 } } },

        { "04 R&B Smooth",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 25 }, { "lsFreq", 80 }, { "lsGain", 1.0f },
            { "b2Freq", 2500 }, { "b2Gain", -0.8f }, { "b2Q", 0.8f }, { "hsFreq", 14000 }, { "hsGain", 1.5f },
            { "mbOn", 1 }, { "loRatio", 1.8f }, { "midRatio", 1.6f }, { "hiThr", -24 }, { "hiRatio", 2.5f },
            { "satOn", 1 }, { "satType", 1 }, { "satDrive", 4 }, { "satMix", 35 },
            { "imgOn", 1 }, { "imgWidth", 115 },
            { "limOn", 1 }, { "limGain", 4.0f }, { "limCeiling", -1.0f }, { "limRelease", 150 } } },

        { "05 Afro / Kompa Groove",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "lsFreq", 75 }, { "lsGain", 1.0f },
            { "b1Freq", 300 }, { "b1Gain", -1.5f }, { "b3Freq", 3000 }, { "b3Gain", 1.5f },
            { "mbOn", 1 }, { "midThr", -18 }, { "midRatio", 2.0f },
            { "satOn", 1 }, { "satDrive", 4 }, { "satMix", 30 },
            { "excOn", 1 }, { "excSub", 20 }, { "excAir", 20 },
            { "imgOn", 1 }, { "imgWidth", 115 }, { "monoBassOn", 1 },
            { "limOn", 1 }, { "limGain", 5.0f }, { "limCeiling", -0.5f } } },

        { "06 Streaming -14 LUFS Safe",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 22 },
            { "mbOn", 1 }, { "loRatio", 1.8f }, { "midRatio", 1.6f }, { "hiRatio", 1.6f },
            { "imgOn", 1 }, { "monoBassOn", 1 },
            { "limOn", 1 }, { "limGain", 3.0f }, { "limCeiling", -1.0f }, { "limRelease", 100 } } },

        { "07 Club Master MAX",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 30 }, { "lsFreq", 60 }, { "lsGain", 1.5f }, { "hsGain", 1.5f },
            { "mbOn", 1 }, { "loThr", -20 }, { "loRatio", 3.0f }, { "midThr", -20 }, { "midRatio", 2.5f }, { "hiThr", -22 }, { "hiRatio", 2.5f },
            { "satOn", 1 }, { "satDrive", 8 }, { "satMix", 45 },
            { "excOn", 1 }, { "excSub", 35 }, { "excAir", 20 },
            { "imgOn", 1 }, { "monoBassOn", 1 }, { "monoBassFreq", 130 },
            { "clipOn", 1 }, { "clipDrive", 4.0f }, { "clipCeiling", -0.3f }, { "clipKnee", 35 },
            { "limOn", 1 }, { "limGain", 9.0f }, { "limCeiling", -0.1f }, { "limRelease", 40 } } },

        { "08 Drill Dark & Hard",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 24 }, { "lsFreq", 50 }, { "lsGain", 1.5f },
            { "b1Freq", 350 }, { "b1Gain", -2.0f }, { "b1Q", 1.4f }, { "b3Freq", 5000 }, { "b3Gain", 1.2f }, { "hsGain", -0.5f },
            { "mbOn", 1 }, { "xLow", 130 }, { "loThr", -18 }, { "loRatio", 3.0f }, { "midThr", -20 }, { "midRatio", 2.2f },
            { "satOn", 1 }, { "satType", 0 }, { "satDrive", 9 }, { "satMix", 45 },
            { "excOn", 1 }, { "excSub", 45 }, { "excAir", 5 },
            { "imgOn", 1 }, { "imgWidth", 105 }, { "monoBassOn", 1 }, { "monoBassFreq", 140 },
            { "clipOn", 1 }, { "clipDrive", 3.5f }, { "clipCeiling", -0.5f }, { "clipKnee", 30 },
            { "limOn", 1 }, { "limGain", 7.5f }, { "limCeiling", -0.3f }, { "limRelease", 45 } } },

        { "09 Vocal Forward (Rap / Chant)",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 28 }, { "b1Freq", 280 }, { "b1Gain", -1.5f }, { "b1Q", 1.0f },
            { "b2Freq", 2200 }, { "b2Gain", 1.5f }, { "b2Q", 0.9f }, { "b3Freq", 6500 }, { "b3Gain", -0.8f }, { "b3Q", 2.0f },
            { "hsFreq", 12000 }, { "hsGain", 1.0f },
            { "mbOn", 1 }, { "midThr", -22 }, { "midRatio", 2.2f }, { "midGain", 1.0f }, { "hiThr", -24 }, { "hiRatio", 2.0f },
            { "satOn", 1 }, { "satType", 1 }, { "satDrive", 4 }, { "satMix", 30 },
            { "imgOn", 1 }, { "imgWidth", 105 },
            { "limOn", 1 }, { "limGain", 5.0f }, { "limCeiling", -1.0f }, { "limRelease", 90 } } },

        { "10 Lo-Fi Chill Vintage",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 40 }, { "lsFreq", 120 }, { "lsGain", 1.0f },
            { "b2Freq", 900 }, { "b2Gain", 1.0f }, { "b2Q", 0.6f }, { "hsFreq", 8000 }, { "hsGain", -4.0f },
            { "mbOn", 1 }, { "loRatio", 2.0f }, { "midRatio", 2.0f }, { "hiRatio", 2.0f },
            { "satOn", 1 }, { "satType", 0 }, { "satDrive", 12 }, { "satMix", 60 },
            { "imgOn", 1 }, { "imgWidth", 90 },
            { "limOn", 1 }, { "limGain", 3.0f }, { "limCeiling", -1.0f }, { "limRelease", 200 } } },

        { "11 Acoustique / Live Naturel",
          { { "eqOn", 1 }, { "hpfOn", 1 }, { "hpfFreq", 30 }, { "b1Freq", 220 }, { "b1Gain", -0.8f }, { "b1Q", 0.8f },
            { "hsFreq", 14000 }, { "hsGain", 1.0f },
            { "mbOn", 1 }, { "mbMix", 60 }, { "loRatio", 1.5f }, { "midRatio", 1.5f }, { "hiRatio", 1.5f },
            { "excOn", 1 }, { "excSub", 0 }, { "excAir", 15 },
            { "imgOn", 1 }, { "imgWidth", 110 },
            { "limOn", 1 }, { "limGain", 2.0f }, { "limCeiling", -1.0f }, { "limRelease", 250 } } },
    };
    return presets;
}
