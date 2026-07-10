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

class GameUI
{
public:
    void Init();
    void Update(const StatusManager* pStatus);
    void Draw();
    void Finalize();
    void NotifyJudge(JUDGE judge);

private:
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

    // Hit / Miss
    Sprite2D* m_pHitSprite  = nullptr;
    Sprite2D* m_pMissSprite = nullptr;
    float     m_JudgeTimer  = 0.0f;
    bool      m_ShowHit     = false;
};
