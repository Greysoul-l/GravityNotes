#include "game_ui.h"
#include "define.h"
#include <string>

// --- 仮置き定数（後で調整してください） ---
//コンボ表示
static constexpr float DIGIT_W        = 60.0f;   // 1桁の幅
static constexpr float DIGIT_H        = 80.0f;   // 1桁の高さ
static constexpr float DIGIT_SPACING  = 55.0f;   // 桁間隔
static constexpr float DIGIT_ANCHOR_X = 100.0f;   // 1桁目（一の位）の固定X座標
static constexpr float COMBO_CENTER_Y = 50.0f;  // 数字のY座標
static constexpr float LABEL_X        = DIGIT_ANCHOR_X + DIGIT_W * 0.5f + 8.0f;
static constexpr float LABEL_Y        = COMBO_CENTER_Y + DIGIT_H * 0.5f;

// Hit / Miss 表示
static constexpr float JUDGE_X           = SCREEN_WIDTH  * 0.5f;   // 表示中心X
static constexpr float JUDGE_Y           = SCREEN_HEIGHT * 0.4f;   // 表示中心Y
static constexpr float JUDGE_W           = 150.0f;                 // スプライト幅
static constexpr float JUDGE_H           = 150.0f;                 // スプライト高さ
static constexpr float JUDGE_DISPLAY_SEC = 0.6f;                   // 表示秒数

// HP バー
static constexpr float HP_BAR_LEFT  = SCREEN_WIDTH  * 0.80f;  // バー左端X
static constexpr float HP_BAR_Y     = SCREEN_HEIGHT * 0.05f;  // バー中心Y
static constexpr float HP_BAR_MAX_W = 250.0f;                 // バー最大幅
static constexpr float HP_BAR_H     = 24.0f;                  // バー高さ
static constexpr float HP_TEXT_X    = HP_BAR_LEFT;            // テキスト左端X
static constexpr float HP_TEXT_Y    = HP_BAR_Y - 22.0f;       // テキストY（バー上）

void GameUI::Init()
{
    for (int i = 0; i < COMBO_MAX_DIGITS; ++i)
    {
        m_pComboDigits[i] = new FadableSplitSprite(
            { DIGIT_ANCHOR_X, COMBO_CENTER_Y },
            { DIGIT_W, DIGIT_H },
            0.0f,
            { 1.0f, 1.0f, 1.0f, 1.0f },
            BLENDSTATE_ALFA,
            L"asset\\texture\\Ingame_Number_UI.png",
            5, 2
        );
        // 初期は全桁を画面外へ
        m_pComboDigits[i]->SetPos({ -999.0f, -999.0f });
    }

    m_pComboLabel = new FontRenderer(
        { LABEL_X, LABEL_Y },
        31.0f, 0.0f,
        { 1.0f, 1.0f, 1.0f, 1.0f },
        "COMBO",
        TA_START
    );

    // 初期コンボ(0)を表示
    UpdateComboDigits(0);

    // HP バー背景（暗いグレー、幅固定）
    float bgCX = HP_BAR_LEFT + HP_BAR_MAX_W * 0.5f;
    m_pHPBarBg = new Sprite2D(
        { bgCX, HP_BAR_Y },
        { HP_BAR_MAX_W, HP_BAR_H },
        0.0f,
        { 0.2f, 0.2f, 0.2f, 1.0f },
        BLENDSTATE_NONE,
        L"asset\\texture\\fade.png"
    );
    // HP バー前景（緑、HP 比率で幅が変わる）
    m_pHPBarFg = new Sprite2D(
        { bgCX, HP_BAR_Y },
        { HP_BAR_MAX_W, HP_BAR_H },
        0.0f,
        { 0.2f, 1.0f, 0.3f, 1.0f },
        BLENDSTATE_NONE,
        L"asset\\texture\\fade.png"
    );
    // Hit / Miss スプライト（初期は非表示、タイマーで制御）
    m_pHitSprite = new Sprite2D(
        { JUDGE_X, JUDGE_Y }, { JUDGE_W, JUDGE_H }, 0.0f,
        { 1.0f, 1.0f, 1.0f, 1.0f }, BLENDSTATE_ALFA,
        L"asset\\texture\\Ingame_Rank_hit_UI.png"
    );
    m_pMissSprite = new Sprite2D(
        { JUDGE_X, JUDGE_Y }, { JUDGE_W, JUDGE_H }, 0.0f,
        { 1.0f, 1.0f, 1.0f, 1.0f }, BLENDSTATE_ALFA,
        L"asset\\texture\\Ingame_Rank_miss_UI.png"
    );

    // HP テキスト
    m_pHPText = new FontRenderer(
        { HP_TEXT_X, HP_TEXT_Y },
        20.0f, 0.0f,
        { 1.0f, 1.0f, 1.0f, 1.0f },
        "HP 1000/ 1000",
        TA_START
    );
}

void GameUI::Update(const StatusManager* pStatus)
{
    int combo = pStatus->GetCombo();
    if (combo != m_LastCombo)
    {
        UpdateComboDigits(combo);
        m_LastCombo = combo;
    }

    int hp    = pStatus->GetHP();
    int maxHP = pStatus->GetMaxHP();
    if (hp != m_LastHP)
    {
        UpdateHP(hp, maxHP);
        m_LastHP = hp;
    }

    if (m_JudgeTimer > 0.0f)
        m_JudgeTimer -= 1.0f / FPS;


}

void GameUI::UpdateComboDigits(int combo)
{
    std::string s = std::to_string(combo);
    int numDigits = static_cast<int>(s.size());

    for (int i = 0; i < COMBO_MAX_DIGITS; ++i)
    {
        // 右詰めで有効桁を割り当てる
        int digitIndex = i - (COMBO_MAX_DIGITS - numDigits);

        if (digitIndex < 0)
        {
            // 使わない桁は画面外へ
            m_pComboDigits[i]->SetPos({ -999.0f, -999.0f });
        }
        else
        {
            m_pComboDigits[i]->SetTextureNumber(s[digitIndex] - '0');

            // 一の位を DIGIT_ANCHOR_X に固定し、桁が増えるほど左へ並べる
            float x = DIGIT_ANCHOR_X + (i - (COMBO_MAX_DIGITS - 1)) * DIGIT_SPACING;
            m_pComboDigits[i]->SetPos({ x, COMBO_CENTER_Y });
        }
    }
}

void GameUI::NotifyJudge(JUDGE judge)
{
    if (judge == JUDGE_NONE) return;
    m_ShowHit    = (judge != JUDGE_MISS);
    m_JudgeTimer = JUDGE_DISPLAY_SEC;
}

void GameUI::UpdateHP(int hp, int maxHP)
{
    if (hp < 0) hp = 0;
    float ratio = (maxHP > 0) ? static_cast<float>(hp) / maxHP : 0.0f;
    float fgW   = HP_BAR_MAX_W * ratio;
    float fgCX  = HP_BAR_LEFT + fgW * 0.5f;
    m_pHPBarFg->SetSize({ fgW, HP_BAR_H });
    m_pHPBarFg->SetPos({ fgCX, HP_BAR_Y });
    m_pHPText->SetText("HP " + std::to_string(hp) + " / " + std::to_string(maxHP));
}

void GameUI::Draw()
{
    // HP
    m_pHPBarBg->Draw();
    m_pHPBarFg->Draw();
    m_pHPText->Draw();

    // Hit / Miss
    if (m_JudgeTimer > 0.0f)
    {
        if (m_ShowHit) m_pHitSprite->Draw();
        else           m_pMissSprite->Draw();
    }

    // コンボ
    m_pComboLabel->Draw();
    for (int i = 0; i < COMBO_MAX_DIGITS; ++i)
    {
        m_pComboDigits[i]->Draw();
    }
}

void GameUI::Finalize()
{
    for (int i = 0; i < COMBO_MAX_DIGITS; ++i)
    {
        delete m_pComboDigits[i];
        m_pComboDigits[i] = nullptr;
    }
    delete m_pComboLabel;
    m_pComboLabel = nullptr;

    delete m_pHPBarBg;   m_pHPBarBg   = nullptr;
    delete m_pHPBarFg;   m_pHPBarFg   = nullptr;
    delete m_pHPText;    m_pHPText    = nullptr;
    delete m_pHitSprite; m_pHitSprite = nullptr;
    delete m_pMissSprite;m_pMissSprite= nullptr;
}
