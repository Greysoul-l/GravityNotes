#pragma once
#include "sprite2d.h"
#include "font.h"
#include "status_manager.h"
#include "fade.h"

static constexpr int COMBO_MAX_DIGITS = 4;

// SplitSprite に SetColor を追加するサブクラス（フェード対応用）
class FadableSplitSprite : public SplitSprite
{
public:
    using SplitSprite::SplitSprite;
    void SetColor(const XMFLOAT4& c) { m_Color = c; }
};

struct SoundData;

class GameUI
{
public:
    void Init();
    void Reset();
    void Update(const StatusManager* pStatus, bool isHoldingRainbow = false);
    void Draw();
    void Finalize();
    void NotifyJudge(JUDGE judge);
    void StartEndSequence(bool isDead, bool isAllHit);
    void ShowResultLogos();

private:
    // 風切りエフェクト
    FadableSplitSprite* m_pWindCutSprite   = nullptr;
    float               m_WindCutAnimTimer = 0.0f;
    float               m_WindCutAlpha     = 0.0f;
    bool                m_IsHoldingRainbow = false;
    // コンボ
    FadableSplitSprite* m_pComboDigits[COMBO_MAX_DIGITS] = {};
    FontRenderer* m_pComboLabel = nullptr;
    int           m_LastCombo   = -1;
    void UpdateComboDigits(int combo);

    // HP
    Sprite2D*     m_pHPBarBg = nullptr;
    Sprite2D*     m_pHPBarFg = nullptr;
    FontRenderer* m_pHPText  = nullptr;
    int           m_LastHP   = -1;
    void UpdateHP(int hp, int maxHP);

    // Hit / Miss / Kaihi
    Sprite2D* m_pHitSprite   = nullptr;
    Sprite2D* m_pMissSprite  = nullptr;
    Sprite2D* m_pKaihiSprite = nullptr;
    float     m_JudgeTimer   = 0.0f;
    JUDGE     m_CurrentJudge = JUDGE_NONE;

    // 終了演出用UI
    Sprite2D* m_pFadeOverlay    = nullptr;
    Sprite2D* m_pClearSprite    = nullptr;
    Sprite2D* m_pAllHitSprite   = nullptr;
    Sprite2D* m_pGameOverSprite = nullptr;

    // SEサウンドデータ
    SoundData* m_pStageClearSe  = nullptr;
    SoundData* m_pGameOverSe    = nullptr;
    SoundData* m_pAllHitSe      = nullptr;

    bool m_ShowEndOverlay = false;
    bool m_ShowLogos      = false;
    bool m_IsDead         = false;
    bool m_IsAllHit       = false;

    float m_FadeTimer    = 0.0f;
    float m_FadeDuration = 3.0f;

    float m_LogoAnimTimer    = 0.0f;
    float m_LogoAnimDuration = 0.35f;
};
