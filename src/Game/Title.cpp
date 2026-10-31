//---------------------------------------------------------------------------//
//!	@file	Title.cpp
//! @brief	Title
//---------------------------------------------------------------------------//
#include "Title.h"
#include "Main_menu.h"
#include "MainStage.h"
#include <System/Scene.h>
#include <WinMain.h>
#include <algorithm>    // std::clamp
#include <cmath>

namespace PoittersPoint {

//! @brief 初期化
//! @return 初期化済み
bool Title::Init()
{
    // 画像の読み込み
    handleSky_      = LoadGraph(u8"data/Game/Title/Sky.png");
    handleBuilding_ = LoadGraph(u8"data/Game/Title/Building.png");
    handlePlayer_   = LoadGraph(u8"data/Game/Title/Player.png");
    handleLogo_     = LoadGraph(u8"data/Game/Title/Professional_Butler.png");

    // ロゴの白いシルエットを作る（色を白にして、透明度だけ残す）
    // 黒い文字にも光が乗るように、加算合成用に白い版を用意する
    {
        int si = LoadSoftImage(u8"data/Game/Title/Professional_Butler.png");
        if(si != -1) {
            int sw = 0, sh = 0;
            GetSoftImageSize(si, &sw, &sh);

            for(int y = 0; y < sh; y++) {
                for(int x = 0; x < sw; x++) {
                    int r, g, b, a;
                    GetPixelSoftImage(si, x, y, &r, &g, &b, &a);
                    DrawPixelSoftImage(si, x, y, 255, 255, 255, a);
                }
            }

            handleLogoShine_ = CreateGraphFromSoftImage(si);
            DeleteSoftImage(si);
        }
    }

    timer_        = 0.0f;
    stateTimer_   = 0.0f;
    fadeAlpha_    = 0.0f;
    introTimer_   = 0.0f;
    flashTimer_   = -1.0f;
    shineTimer_   = 0.0f;
    sparkleTimer_ = -1.0f;
    sparkleWait_  = kSparkleInterval - kSparkleStart;
    state_        = State::Intro;

    return true;
}

void Title::Update()
{
    timer_ += 0.05f;

    // フラッシュの時間を進める
    if(flashTimer_ >= 0.0f) {
        flashTimer_ += 1.0f;
        if(flashTimer_ >= kFlashTotal) {
            flashTimer_ = -1.0f;    // 終了
        }
    }

    // ロゴの光の演出タイマー（カメラ演出が終わってから1回だけ進める）
    if(state_ != State::Intro && shineTimer_ < kShineTotal) {
        shineTimer_ += 1.0f;
    }

    // 「キラン」の繰り返し
    if(sparkleTimer_ >= 0.0f) {
        // 光っている最中は、Enterが押された後でも最後まで進める
        sparkleTimer_ += 1.0f;
        if(sparkleTimer_ >= kSparkleFrames) {
            sparkleTimer_ = -1.0f;    // 終了
            sparkleWait_  = 0.0f;
        }
    }
    else if(state_ == State::WaitInput) {
        // 新しく光らせるのは入力待ちの間だけ（Enter後は光らない）
        sparkleWait_ += 1.0f;
        if(sparkleWait_ >= kSparkleInterval) {
            sparkleTimer_ = 0.0f;
        }
    }

    switch(state_) {
    case State::Intro:
        introTimer_ += 1.0f;
        // 演出が終わる、またはEnterでスキップしたら入力待ちへ
        if(introTimer_ >= kIntroFrames || Input::IsKeyDown(KEY_INPUT_RETURN)) {
            introTimer_ = kIntroFrames;
            state_      = State::WaitInput;
        }
        break;

    case State::WaitInput:
        // Enterキーが押されたら、フラッシュしてから点滅状態へ移行
        if(Input::IsKeyDown(KEY_INPUT_RETURN)) {
            state_      = State::Blinking;
            stateTimer_ = 0.0f;
            flashTimer_ = 0.0f;    // フラッシュ開始
        }
        break;

    case State::Blinking:
        stateTimer_ += 1.0f;
        // 約1秒間（60フレーム）高速点滅したあとフェードアウトへ
        if(stateTimer_ >= 60.0f) {
            state_      = State::FadeOut;
            stateTimer_ = 0.0f;
        }
        break;

    case State::FadeOut:
        // 少しずつ暗くする（約0.8秒で完全暗転）
        fadeAlpha_ += 5.0f;
        if(fadeAlpha_ >= 255.0f) {
            fadeAlpha_ = 255.0f;
            // 完全暗転したらメインメニューへ遷移
            Scene::Change(Scene::GetScene<MainMenu>());
        }
        break;
    }
}

void Title::LateDraw()
{
    int w = WINDOW_W;
    int h = WINDOW_H;

    // --- 1. タイトル画像の描画（奥から順に重ねる） ---
    // 基準は1280x720。画面サイズに合わせて拡大縮小する
    const float sx = static_cast<float>(w) / 1280.0f;
    const float sy = static_cast<float>(h) / 720.0f;

    // カメラ演出の進行度 e: 0=アップ → 1=今のタイトル配置（ease-in-out）
    float e = 1.0f;
    if(state_ == State::Intro) {
        const float t = introTimer_ / kIntroFrames;
        e             = (t < 0.5f) ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // ズームの中心（プレイヤーの上半身あたり）。最終的な画面上の座標
    const float fx = 665.0f * sx;
    const float fy = 470.0f * sy;

    // カメラの中心：最初は注目点が画面中央に来て、最後は元の位置に戻る
    const float cx = (w * 0.5f) * (1.0f - e) + fx * e;
    const float cy = (h * 0.5f) * (1.0f - e) + fy * e;

    // 1レイヤーを描く。startScaleは演出開始時の拡大率（終了時は1.0）
    auto drawLayer = [&](int handle, float x, float y, float dw, float dh, float startScale) {
        if(handle == -1)
            return;

        const float s  = startScale + (1.0f - startScale) * e;
        const float x0 = cx + (x - fx) * s;
        const float y0 = cy + (y - fy) * s;
        const float x1 = cx + (x + dw - fx) * s;
        const float y1 = cy + (y + dh - fy) * s;

        DrawExtendGraphF(x0, y0, x1, y1, handle, TRUE);    // Fつきで小数座標。動きが滑らかになる
    };

    // 拡大率（奥ほど小さく、手前ほど大きくすると奥行きが出る）
    // 空・ビルは全画面なので、画面外が見えないよう空は1.5以上にすること
    const float scaleSky      = 1.6f;
    const float scaleBuilding = 2.2f;
    const float scalePlayer   = 3.0f;
    const float scaleLogo     = 3.0f;

    // 拡大縮小を滑らかにする（バイリニア補間）
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    // 光の筋と粒の強さ。プレイヤーやロゴと同じく、演出の後半でフェードインする
    const float fxAmount = std::clamp((e - 0.5f) * 2.0f, 0.0f, 1.0f);

    // 1) 空（全画面）
    drawLayer(handleSky_, 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), scaleSky);

    // 2) ビル（全画面）
    drawLayer(handleBuilding_, 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), scaleBuilding);

    // 3) 左右の白フェード（ビルの上、プレイヤーとロゴの下）
    DrawEdgeFade(w, h);

    // 4) 光の筋（プレイヤーとロゴの後ろ）
    DrawLightRays(w, h, fxAmount);

    // 5) ロゴ（元の縦横比を保つ）。演出の後半でフェードインする
    if(handleLogo_ != -1) {
        int lw = 0, lh = 0;
        GetGraphSize(handleLogo_, &lw, &lh);

        const float logoX = 345.0f * sx;
        const float logoY = 30.0f * sy;
        const float drawW = 630.0f * sx;
        const float drawH = drawW * static_cast<float>(lh) / static_cast<float>(lw);

        // e=0.5までは透明、e=1.0で不透明（演出後は常に1.0）
        const float logoAlpha = std::clamp((e - 0.5f) * 2.0f, 0.0f, 1.0f);
        if(logoAlpha > 0.0f) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(logoAlpha * 255.0f));
            drawLayer(handleLogo_, logoX, logoY, drawW, drawH, scaleLogo);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // --- ロゴの光：演出が終わったあと1回だけ、左から右へ流れる ---
        if(handleLogoShine_ != -1 && state_ != State::Intro && shineTimer_ < kShineFrames) {
            float t = shineTimer_ / kShineFrames;
            t       = t * t * (3.0f - 2.0f * t);    // 動き出しと終わりを少しゆっくりに

            const float center   = -0.25f + 1.5f * t;    // 光の帯の中心（ロゴ幅に対する割合）
            const float bandHalf = 0.12f;                // 帯の半分の幅
            const int   strips   = 12;                   // 帯を縦に分割する数（多いほど滑らか）

            for(int i = 0; i < strips; i++) {
                const float u0 = center - bandHalf + (bandHalf * 2.0f) * static_cast<float>(i) / strips;
                const float u1 = center - bandHalf + (bandHalf * 2.0f) * static_cast<float>(i + 1) / strips;

                // 帯の中央ほど明るく、両端は透明に近づける
                const float mid = (static_cast<float>(i) + 0.5f) / strips;
                const float k   = 1.0f - std::abs(mid * 2.0f - 1.0f);

                int ix0 = static_cast<int>(std::floor(logoX + u0 * drawW));
                int ix1 = static_cast<int>(std::floor(logoX + u1 * drawW));
                ix0     = std::clamp(ix0, 0, w);
                ix1     = std::clamp(ix1, 0, w);
                if(ix1 <= ix0)
                    continue;

                SetDrawArea(ix0, 0, ix1, h);    // この縦帯の中だけに描く
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(k * 200.0f));
                DrawExtendGraphF(logoX, logoY, logoX + drawW, logoY + drawH, handleLogoShine_, TRUE);
            }

            SetDrawArea(0, 0, w, h);    // 描画範囲を元に戻す
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // --- 「キラン」：光が流れたあと、ロゴの右側で星がきらりと光る ---
        if(state_ != State::Intro) {
            const float st = sparkleTimer_;
            if(st >= 0.0f && st < kSparkleFrames) {
                const float t     = st / kSparkleFrames;
                const float pulse = std::sin(t * DX_PI_F);    // 0 → 1 → 0

                // 星の中心（ロゴに対する割合）。位置はここで調整する
                const float starX = logoX + drawW * 0.95f;
                const float starY = logoY + drawH * 0.58f;

                const float angle = t * 0.6f;    // ゆっくり回す

                // 細長いひし形を1つ描く
                auto drawDiamond = [&](float ang, float len, float thick) {
                    const float c  = std::cos(ang);
                    const float s  = std::sin(ang);
                    const float lx = c * len, ly = s * len;         // 長軸
                    const float tx = -s * thick, ty = c * thick;    // 短軸
                    DrawQuadrangleAA(starX + lx,
                                     starY + ly,
                                     starX + tx,
                                     starY + ty,    //
                                     starX - lx,
                                     starY - ly,
                                     starX - tx,
                                     starY - ty,    //
                                     GetColor(255, 255, 255),
                                     TRUE);
                };

                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

                // やわらかい光のにじみ
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(120.0f * pulse));
                DrawCircleAA(starX, starY, 26.0f * sx * pulse, 32, GetColor(255, 240, 180), TRUE);

                // 十字の星（縦長・横長）と、小さな斜めの星
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                drawDiamond(angle + DX_PI_F * 0.5f, 80.0f * sx * pulse, 7.0f * sx * pulse);     // 縦
                drawDiamond(angle, 60.0f * sx * pulse, 7.0f * sx * pulse);                      // 横
                drawDiamond(angle + DX_PI_F * 0.25f, 32.0f * sx * pulse, 4.0f * sx * pulse);    // 斜め
                drawDiamond(angle - DX_PI_F * 0.25f, 32.0f * sx * pulse, 4.0f * sx * pulse);    // 斜め

                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }
    }

    // 6) 金色の粒（プレイヤーとロゴの後ろ）
    DrawParticles(w, h, fxAmount);

    // 7) プレイヤー（元の縦横比を保つ）。演出の後半でフェードインする
    if(handlePlayer_ != -1) {
        int pw = 0, ph = 0;
        GetGraphSize(handlePlayer_, &pw, &ph);

        const float drawW = 260.0f * sx;
        const float drawH = drawW * static_cast<float>(ph) / static_cast<float>(pw);

        // e=0.5までは透明、e=1.0で不透明（演出後は常に1.0）
        const float playerAlpha = std::clamp((e - 0.5f) * 2.0f, 0.0f, 1.0f);
        if(playerAlpha > 0.0f) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(playerAlpha * 255.0f));
            drawLayer(handlePlayer_, 535.0f * sx, 385.0f * sy, drawW, drawH, scalePlayer);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // 補間モードを元に戻す（DxLibの初期値）
    SetDrawMode(DX_DRAWMODE_NEAREST);

    // --- 2. 「PRESS ENTER」ボタン描画 ---
    SetFontSize(24);
    const char* msg = "PRESS ENTER";
    int         mw  = GetDrawFormatStringWidth(msg);

    const int arrowSpace = 40;    // 左右の矢印用の余白
    int       btnWidth   = mw + 40 + arrowSpace * 2;
    int       btnHeight  = 48;
    int       btnX       = (w - btnWidth) / 2;
    int       btnY       = h - 160;

    int alpha = 255;
    if(state_ == State::Intro) {
        // カメラ演出中はボタンを出さない
        alpha = 0;
    }
    else if(state_ == State::WaitInput) {
        // 通常時: ゆったりした点滅 (アルファ値: 50～255)
        alpha = static_cast<int>((std::sin(timer_) + 1.0f) * 0.5f * 205.0f) + 50;
    }
    else if(state_ == State::Blinking) {
        // Enter押下後: 高速点滅 (点滅表示/非表示を交互に切り替え)
        alpha = (static_cast<int>(stateTimer_) / 4 % 2 == 0) ? 255 : 0;
    }
    else if(state_ == State::FadeOut) {
        // フェードアウト中: ボタンは非表示
        alpha = 0;
    }

    if(alpha > 0) {
        const float a   = static_cast<float>(alpha) / 255.0f;
        const int   bx0 = btnX;
        const int   by0 = btnY;
        const int   bx1 = btnX + btnWidth;
        const int   by1 = btnY + btnHeight;
        const int   cy  = btnY + btnHeight / 2;

        const unsigned int gold     = GetColor(255, 215, 0);
        const unsigned int paleGold = GetColor(255, 240, 170);
        const unsigned int white    = GetColor(255, 255, 255);

        // 1) 外側のにじむ光（呼吸するように強弱をつける）
        const float breathe = 0.5f + 0.5f * std::sin(timer_ * 2.0f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>((12.0f + 14.0f * breathe) * a));
        for(int i = 4; i >= 1; i--) {
            DrawRoundRect(bx0 - i * 3, by0 - i * 3, bx1 + i * 3, by1 + i * 3, 8 + i * 3, 8 + i * 3, GetColor(255, 190, 60), TRUE);
        }

        // 2) ボタンの背景（濃い紺）と、上半分のうっすらしたツヤ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 0.85f));
        DrawRoundRect(bx0, by0, bx1, by1, 8, 8, GetColor(12, 18, 45), TRUE);

        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(40.0f * a));
        DrawRoundRect(bx0 + 3, by0 + 3, bx1 - 3, cy, 6, 6, GetColor(70, 90, 150), TRUE);

        // 3) 光のスイープ：約3秒に1回、ボタンの上を斜めの光が横切る
        {
            const float frame = timer_ * 20.0f;                      // timer_は1フレーム0.05なので20倍でフレーム数
            const float sp    = std::fmod(frame, 180.0f) / 50.0f;    // 0〜1の間だけ流れる（50フレーム）
            if(sp < 1.0f) {
                const float c = static_cast<float>(bx0) - 40.0f + (static_cast<float>(btnWidth) + 80.0f) * sp;

                SetDrawArea(bx0, by0, bx1, by1);    // ボタンの中だけに描く
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(110.0f * a));
                DrawQuadrangleAA(c,
                                 static_cast<float>(by0),
                                 c + 30.0f,
                                 static_cast<float>(by0),
                                 c + 10.0f,
                                 static_cast<float>(by1),
                                 c - 20.0f,
                                 static_cast<float>(by1),
                                 white,
                                 TRUE);
                SetDrawArea(0, 0, w, h);
            }
        }

        // 4) 金色の枠線（外側は太め、内側は細い二重線）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawRoundRect(bx0, by0, bx1, by1, 8, 8, gold, FALSE);
        DrawRoundRect(bx0 + 1, by0 + 1, bx1 - 1, by1 - 1, 8, 8, gold, FALSE);
        DrawRoundRect(bx0 + 4, by0 + 4, bx1 - 4, by1 - 4, 5, 5, paleGold, FALSE);

        // 5) 左右の矢印（文字に向かって行ったり来たりする）
        const float nudge = std::sin(timer_ * 3.0f) * 3.0f;
        const float lx    = static_cast<float>(bx0) + 22.0f + nudge;
        const float rx    = static_cast<float>(bx1) - 22.0f - nudge;
        const float fcy   = static_cast<float>(cy);
        DrawTriangleAA(lx, fcy - 7.0f, lx, fcy + 7.0f, lx + 10.0f, fcy, gold, TRUE);
        DrawTriangleAA(rx, fcy - 7.0f, rx, fcy + 7.0f, rx - 10.0f, fcy, gold, TRUE);

        // 6) テキスト（黒い影をつけて読みやすくする）
        int textX = btnX + (btnWidth - mw) / 2;
        int textY = btnY + (btnHeight - 24) / 2;
        DrawFormatString(textX + 1, textY + 2, GetColor(0, 0, 0), msg);
        DrawFormatString(textX, textY, white, msg);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // --- 3. フラッシュ演出（ゲーム風）：全体が白く光り、だんだん画面が見えてくる ---
    if(flashTimer_ >= 0.0f) {
        const float ft = flashTimer_;

        // 白さ intensity：一気に真っ白 → 少し止まる → ゆっくり透けて画面が見えてくる
        float intensity = 0.0f;
        if(ft < kFlashRise) {
            intensity = std::sin(ft / kFlashRise * DX_PI_F * 0.5f);    // 速く立ち上がる
        }
        else if(ft < kFlashRise + kFlashHold) {
            intensity = 1.0f;    // 真っ白の状態を少し保つ
        }
        else {
            const float u = (ft - kFlashRise - kFlashHold) / kFlashFall;    // 0 → 1
            const float s = u * u * (3.0f - 2.0f * u);                      // なめらかに変化
            intensity     = 1.0f - s;
        }
        intensity = std::clamp(intensity, 0.0f, 1.0f);

        // 全画面の白。alphaが下がるにつれて、後ろのタイトル画面が透けて見える
        const float peakAlpha = 210.0f;    // ピーク時の白さ（255で真っ白 / 下げると透ける）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(intensity * peakAlpha));
        DrawBox(0, 0, w, h, GetColor(255, 252, 240), TRUE);    // ほんのり暖かい白

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // --- 4. 暗転用フェードアウト黒マスク ---
    if(fadeAlpha_ > 0.0f) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(fadeAlpha_));
        DrawBox(0, 0, w, h, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

///! @brief 左右の白フェードを描画する
//! 白いボックスを中央で左右に分け、左はα→0、右は0→αにする
void Title::DrawEdgeFade(int w, int h)
{
    const int half  = w / 2;
    const int fadeW = static_cast<int>(half * kEdgeFadeRatio);
    if(fadeW <= 0)
        return;

    const unsigned int white = GetColor(255, 255, 255);

    for(int x = 0; x < fadeW; x += kEdgeFadeStep) {
        const int x0 = x;
        const int x1 = std::min(x + kEdgeFadeStep, fadeW);

        // 帯の中央位置での進行度 t: 0=画面端 → 1=フェード終端
        const float t     = (static_cast<float>(x0 + x1) * 0.5f) / static_cast<float>(fadeW);
        const int   alpha = static_cast<int>((1.0f - t) * kEdgeFadeMaxAlpha);
        if(alpha <= 0)
            continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        DrawBox(x0, 0, x1, h, white, TRUE);            // 左
        DrawBox(w - x1, 0, w - x0, h, white, TRUE);    // 右
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

//! @brief ビルの間から差し込む光の筋を描画する
void Title::DrawLightRays(int w, int h, float amount)
{
    if(amount <= 0.0f)
        return;

    const float sx = static_cast<float>(w) / 1280.0f;

    const int   rayCount = 6;    // 筋の本数
    const int   strips   = 6;    // 縦の分割数（下へ行くほど薄くするため）
    const float rayLen   = static_cast<float>(h) * 0.9f;
    const float slant    = 260.0f * sx;    // 下へ行くほど右へ流れる量

    for(int i = 0; i < rayCount; i++) {
        const float baseX = static_cast<float>(w) * (0.12f + 0.15f * static_cast<float>(i));    // 画面上端での位置
        const float topW  = (30.0f + 18.0f * static_cast<float>(i % 3)) * sx;                   // 筋の太さ

        // 筋ごとにゆっくり明るさが変わる
        const float pulse = 0.5f + 0.5f * std::sin(timer_ * 1.3f + static_cast<float>(i) * 1.9f);

        // 太い層と細い層を重ねて、中心を明るく縁をやわらかくする
        for(int layer = 0; layer < 2; layer++) {
            const float widthScale = (layer == 0) ? 1.0f : 0.45f;
            const float wd         = topW * widthScale;
            const float offset     = (topW - wd) * 0.5f;

            for(int s = 0; s < strips; s++) {
                const float v0 = static_cast<float>(s) / strips;
                const float v1 = static_cast<float>(s + 1) / strips;

                const int y0 = static_cast<int>(rayLen * v0);
                const int y1 = static_cast<int>(rayLen * v1);

                const int xa0 = static_cast<int>(baseX + offset + slant * v0);
                const int xa1 = static_cast<int>(baseX + offset + wd + slant * v0);
                const int xb0 = static_cast<int>(baseX + offset + slant * v1);
                const int xb1 = static_cast<int>(baseX + offset + wd + slant * v1);

                // 下へ行くほど薄くする
                const float fall = 1.0f - v0;
                const int   a    = static_cast<int>((7.0f + 12.0f * pulse) * fall * amount);
                if(a <= 0)
                    continue;

                SetDrawBlendMode(DX_BLENDMODE_ADD, a);
                DrawQuadrangle(xa0, y0, xa1, y0, xb1, y1, xb0, y1, GetColor(255, 245, 210), TRUE);
            }
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

//! @brief 下から舞い上がる金色の粒を描画する
void Title::DrawParticles(int w, int h, float amount)
{
    if(amount <= 0.0f)
        return;

    const float sx    = static_cast<float>(w) / 1280.0f;
    const float frame = timer_ * 20.0f;    // timer_は1フレーム0.05なので20倍でフレーム数
    const int   count = 40;                // 粒の数

    // 小数部分だけを取り出す（粒ごとの固定のばらつきを作るために使う）
    auto frac = [](float v) { return v - std::floor(v); };

    for(int i = 0; i < count; i++) {
        const float fi    = static_cast<float>(i);
        const float seedX = frac(fi * 0.61803f);
        const float seedY = frac(fi * 0.75488f);
        const float seedS = frac(fi * 0.38197f);

        const float speed = 0.35f + seedS * 0.8f;    // 上へ昇る速さ（px/フレーム）
        const float range = static_cast<float>(h) + 40.0f;

        // 下から上へ流れ、上まで行ったら下に戻る
        const float y = static_cast<float>(h) + 20.0f - std::fmod(seedY * range + frame * speed, range);
        const float x = seedX * static_cast<float>(w) + std::sin(frame * 0.02f + fi) * 14.0f * sx;    // 左右にゆらゆら
        const float r = (1.5f + seedS * 2.5f) * sx;

        // きらきら点滅
        const float twinkle = 0.5f + 0.5f * std::sin(frame * 0.05f + fi * 1.7f);
        const int   a       = static_cast<int>((60.0f + 150.0f * twinkle) * amount);

        // やわらかい外側の光と、明るい芯
        SetDrawBlendMode(DX_BLENDMODE_ADD, a / 3);
        DrawCircleAA(x, y, r * 2.2f, 12, GetColor(255, 200, 90), TRUE);

        SetDrawBlendMode(DX_BLENDMODE_ADD, a);
        DrawCircleAA(x, y, r, 12, GetColor(255, 245, 200), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

}    // namespace PoittersPoint
