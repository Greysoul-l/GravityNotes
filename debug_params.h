#pragma once
#include <fstream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include "framework/nlohmann/json.hpp"

struct CameraFaceOffset {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
};

// =========================================================================
// 【リリース用定数予約】
// デバッグで調整した確定値をここに書き込んでください。
// リリースビルド時には、JSONは読み込まれず、ここに定義した値が使われます。
// =========================================================================
namespace ReleaseConfig
{
    constexpr float noteSpeed = 15.0f;
    constexpr float hitDistance = 2.0f;
    constexpr float laneWidth = 2.0f;
    constexpr float gravityTransTime = 0.3f;
    constexpr float damageFlashColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
    constexpr float damageFlashDuration = 0.7f;
    constexpr float damageFlashInterval = 0.08f;
    constexpr int   baseScore = 100;
    constexpr float comboMultiplier = 0.1f;
    constexpr int   orbHealAmount = 30;
    constexpr float orbJudgeWindow = -0.5f;

    // 各面のカメラオフセット（Yaw, Pitch, PosX, PosY, PosZ）
    constexpr CameraFaceOffset cameraOffsets[4] = {
        { 0.0f, -12.3f, 0.0f, 1.3f, 2.5f }, // FLOOR
        { 10.3f, 0.0f, 1.7f, 0.0f, 2.5f }, // LEFT_WALL
        { 0.0f, 12.3f, 0.0f, -1.3f, 2.5f }, // CEILING
        { -10.3f, 0.0f, -1.7f, 0.0f, 2.5f } // RIGHT_WALL
    };
}

struct DebugParams
{
    // ノーツ
    float noteSpeed         = 15.0f;  // ノーツのZ軸移動速度 (units/sec)
    float hitDistance       = 2.0f;   // 判定が発生するZ距離 (units)

    // プレイヤー
    float laneWidth         = 2.0f;   // レーン間の距離 (units)
    float gravityTransTime  = 0.3f;   // 重力移動にかかる時間 (sec)
    float damageFlashColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
    float damageFlashDuration = 0.7f; // ダメージ点滅の合計時間 (sec)
    float damageFlashInterval = 0.08f;// 通常色/点滅色の切替間隔 (sec)

    // スコア
    int   baseScore         = 100;    // 1ヒットあたりの基本スコア
    float comboMultiplier   = 0.1f;   // コンボ数 × この値が倍率に加算（1.0 + combo * multiplier）

    // オーブ
    int   orbHealAmount     = 30;     // オーブ取得時のHP回復量
    float orbJudgeWindow    = -0.5f;   // オーブの早期HIT判定を開始する追加Z距離（HIT_ZONE_Zに加算。値を大きくするほどプレイヤーから遠い位置で取得判定になり、見た目の取得位置が胴側に上がる）

    // カメラの面ごとオフセット
    CameraFaceOffset cameraOffsets[4] = {
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // FACE_FLOOR (0)
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // FACE_LEFT_WALL (1)
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // FACE_CEILING (2)
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }  // FACE_RIGHT_WALL (3)
    };

    static DebugParams& Get()
    {
        static DebugParams s;
        return s;
    }

    void Save()
    {
#ifdef _DEBUG
        try
        {
            nlohmann::json j;
            j["noteSpeed"] = noteSpeed;
            j["hitDistance"] = hitDistance;
            j["laneWidth"] = laneWidth;
            j["gravityTransTime"] = gravityTransTime;
            
            nlohmann::json flashColor = { damageFlashColor[0], damageFlashColor[1], damageFlashColor[2], damageFlashColor[3] };
            j["damageFlashColor"] = flashColor;
            j["damageFlashDuration"] = damageFlashDuration;
            j["damageFlashInterval"] = damageFlashInterval;

            j["baseScore"] = baseScore;
            j["comboMultiplier"] = comboMultiplier;
            j["orbHealAmount"] = orbHealAmount;
            j["orbJudgeWindow"] = orbJudgeWindow;

            // JSONへのカメラオフセット出力は全廃止

            // C++コピペ用テキストファイルの出力
            std::ofstream txtFile("debugscene/camera_offsets_release.txt");
            if (txtFile.is_open())
            {
                txtFile << std::fixed << std::setprecision(1);
                txtFile << "    constexpr CameraFaceOffset cameraOffsets[4] = {\n";
                const char* comments[] = { " // FLOOR", " // LEFT_WALL", " // CEILING", " // RIGHT_WALL" };
                for (int i = 0; i < 4; i++)
                {
                    float y = roundf(cameraOffsets[i].yaw * 10.0f) / 10.0f;
                    float p = roundf(cameraOffsets[i].pitch * 10.0f) / 10.0f;
                    float px = roundf(cameraOffsets[i].posX * 10.0f) / 10.0f;
                    float py = roundf(cameraOffsets[i].posY * 10.0f) / 10.0f;
                    float pz = roundf(cameraOffsets[i].posZ * 10.0f) / 10.0f;

                    txtFile << "        { " 
                            << y << "f, " 
                            << p << "f, " 
                            << px << "f, " 
                            << py << "f, " 
                            << pz << "f }";
                    if (i < 3) txtFile << ",";
                    txtFile << comments[i] << "\n";
                }
                txtFile << "    };\n";
                txtFile.close();
            }
        }
        catch (...)
        {
        }
#endif
    }

    void LoadCameraOffsets()
    {
#ifdef _DEBUG
        try
        {
            std::ifstream file("debugscene/camera_offsets_release.txt");
            if (!file.is_open()) return;

            std::string line;
            int faceIndex = 0;
            while (std::getline(file, line) && faceIndex < 4)
            {
                size_t start = line.find('{');
                size_t end = line.find('}');
                if (start != std::string::npos && end != std::string::npos && start < end)
                {
                    std::string inner = line.substr(start + 1, end - start - 1);
                    for (char& c : inner)
                    {
                        if (c == 'f' || c == ',') c = ' ';
                    }
                    std::stringstream ss(inner);
                    float y = 0.0f, p = 0.0f, px = 0.0f, py = 0.0f, pz = 0.0f;
                    if (ss >> y >> p >> px >> py >> pz)
                    {
                        cameraOffsets[faceIndex].yaw = y;
                        cameraOffsets[faceIndex].pitch = p;
                        cameraOffsets[faceIndex].posX = px;
                        cameraOffsets[faceIndex].posY = py;
                        cameraOffsets[faceIndex].posZ = pz;
                    }
                    faceIndex++;
                }
            }
            file.close();
        }
        catch (...)
        {
        }
#endif
    }

    void Load()
    {
#ifdef _DEBUG
        try
        {
            std::ifstream file("debugscene/debug_params.json");
            if (file.is_open())
            {
                nlohmann::json j;
                file >> j;
                file.close();

                noteSpeed = j.value("noteSpeed", noteSpeed);
                hitDistance = j.value("hitDistance", hitDistance);
                laneWidth = j.value("laneWidth", laneWidth);
                gravityTransTime = j.value("gravityTransTime", gravityTransTime);

                if (j.contains("damageFlashColor") && j["damageFlashColor"].is_array() && j["damageFlashColor"].size() == 4)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        damageFlashColor[i] = j["damageFlashColor"][i].get<float>();
                    }
                }
                damageFlashDuration = j.value("damageFlashDuration", damageFlashDuration);
                damageFlashInterval = j.value("damageFlashInterval", damageFlashInterval);

                baseScore = j.value("baseScore", baseScore);
                comboMultiplier = j.value("comboMultiplier", comboMultiplier);
                orbHealAmount = j.value("orbHealAmount", orbHealAmount);
                orbJudgeWindow = j.value("orbJudgeWindow", orbJudgeWindow);
            }

            // カメラオフセットはテキストファイルから読み込む
            LoadCameraOffsets();
        }
        catch (...)
        {
        }
#endif
    }

private:
    DebugParams()
    {
#ifdef _DEBUG
        Load();
#else
        // リリースビルド時は定数を適用
        noteSpeed = ReleaseConfig::noteSpeed;
        hitDistance = ReleaseConfig::hitDistance;
        laneWidth = ReleaseConfig::laneWidth;
        gravityTransTime = ReleaseConfig::gravityTransTime;
        for (int i = 0; i < 4; i++) damageFlashColor[i] = ReleaseConfig::damageFlashColor[i];
        damageFlashDuration = ReleaseConfig::damageFlashDuration;
        damageFlashInterval = ReleaseConfig::damageFlashInterval;
        baseScore = ReleaseConfig::baseScore;
        comboMultiplier = ReleaseConfig::comboMultiplier;
        orbHealAmount = ReleaseConfig::orbHealAmount;
        orbJudgeWindow = ReleaseConfig::orbJudgeWindow;
        for (int i = 0; i < 4; i++) cameraOffsets[i] = ReleaseConfig::cameraOffsets[i];
#endif
    }
};

#define D_PARAMS (DebugParams::Get())
