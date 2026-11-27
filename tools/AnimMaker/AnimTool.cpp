//---------------------------------------------------------------------------
//! @file   AnimTool.cpp
//! @brief  Small DxLib tool to make .mv1 animation files(.mv1のアニメファイルを作るための小さなDxLibツール)
//!
//! Usage(使い方):
//!   AnimTool.exe dump    <model.mv1> <idleAnim.mv1> <out.txt>
//!       Write every bone (frame) of the model: name, parent, and its WORLD matrix in the
//!       rest pose and in the first frame of the idle animation. make_knockdown.py reads this.
//!       (モデルの全ボーン(フレーム)を書き出す：名前・親・初期姿勢とアイドルアニメ最初のフレームでのワールド行列。
//!        make_knockdown.py がこれを読む)
//!   AnimTool.exe convert <in.x> <out.mv1>
//!       Load a DirectX .x file (made by make_knockdown.py) and save it as .mv1
//!       (make_knockdown.py が作った DirectX の .x ファイルを読み込み、.mv1 で保存する)
//!   AnimTool.exe verify  <model.mv1> <anim.mv1> <out.txt>
//!       Play the animation on the model and write the Chest/Head height over time, to check it
//!       (モデルにアニメを再生させて、Chest/Headの高さを時間ごとに書き出す。確認用)
//!   AnimTool.exe info    <file.mv1> [more files...]
//!       Print how many animations each file has, with their names and lengths
//!       (各ファイルにアニメがいくつあるか、名前と長さを表示する)
//!   AnimTool.exe render  <model.mv1> <animA.mv1> <animB.mv1> side|quarter <out.png>
//!       Draw 8 moments of each animation into one PNG, to look at the motion
//!       (各アニメの8つの瞬間を1枚のPNGに描く。動きを目で確認するため)
//---------------------------------------------------------------------------
#include <DxLib.h>
#include <cstdio>
#include <cstring>

namespace {
void WriteMatrix(FILE* fp, const MATRIX& m)
{
    for(int r = 0; r < 4; ++r)
        for(int c = 0; c < 4; ++c) fprintf(fp, " %.6f", m.m[r][c]);
}

//Dump bones(ボーンを書き出す)
int Dump(const char* modelPath, const char* animPath, const char* outPath)
{
    int model = MV1LoadModel(modelPath);
    int anim  = MV1LoadModel(animPath);
    if(model < 0 || anim < 0) {
        printf("load failed: model=%d anim=%d\n", model, anim);
        return 1;
    }

    FILE* fp = nullptr;
    if(fopen_s(&fp, outPath, "w") != 0 || !fp)
        return 2;

    int num = MV1GetFrameNum(model);
    fprintf(fp, "frames %d\n", num);

    //Rest pose world matrices (no animation attached)(初期姿勢のワールド行列(アニメなし))
    for(int i = 0; i < num; ++i) {
        fprintf(fp, "rest %d %s %d", i, MV1GetFrameName(model, i), MV1GetFrameParent(model, i));
        WriteMatrix(fp, MV1GetFrameLocalWorldMatrix(model, i));
        fprintf(fp, "\n");
    }

    //Idle animation, first frame(アイドルアニメの最初のフレーム)
    int attach = MV1AttachAnim(model, 0, anim, TRUE);
    MV1SetAttachAnimTime(model, attach, 0.0f);
    fprintf(fp, "idleTotal %f\n", MV1GetAttachAnimTotalTime(model, attach));
    for(int i = 0; i < num; ++i) {
        fprintf(fp, "idle %d %s %d", i, MV1GetFrameName(model, i), MV1GetFrameParent(model, i));
        WriteMatrix(fp, MV1GetFrameLocalWorldMatrix(model, i));
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("dump ok: %d frames\n", num);
    return 0;
}

//.x -> .mv1
int Convert(const char* inPath, const char* outPath)
{
    int model = MV1LoadModel(inPath);
    if(model < 0) {
        printf("load failed: %s\n", inPath);
        return 1;
    }

    int animNum = MV1GetAnimNum(model);
    for(int i = 0; i < animNum; ++i) printf("anim %d: %s total=%f\n", i, MV1GetAnimName(model, i), MV1GetAnimTotalTime(model, i));

    //Anim16BitFlag = FALSE: keep full precision keys(Anim16BitFlag＝FALSE：キーを高精度のまま保存)
    int ret = MV1SaveModelToMV1File(model, outPath, MV1_SAVETYPE_NORMAL, -1, TRUE, 1, 1, 0, FALSE);
    printf("save %s -> %d\n", outPath, ret);
    return ret == 0 ? 0 : 3;
}

//Play the animation on the model and print heights(モデルでアニメを再生して高さを出す)
int Verify(const char* modelPath, const char* animPath, const char* outPath)
{
    int model = MV1LoadModel(modelPath);
    int anim  = MV1LoadModel(animPath);
    if(model < 0 || anim < 0) {
        printf("load failed: model=%d anim=%d\n", model, anim);
        return 1;
    }

    FILE* fp = nullptr;
    if(fopen_s(&fp, outPath, "w") != 0 || !fp)
        return 2;

    int attach = MV1AttachAnim(model, 0, anim, TRUE);
    if(attach < 0) {
        printf("attach failed\n");
        return 3;
    }

    float total = MV1GetAttachAnimTotalTime(model, attach);
    int   chest = MV1SearchFrame(model, "Chest");
    int   head  = MV1SearchFrame(model, "Head");
    int   footL = MV1SearchFrame(model, "Leg:Left:Lower_end");
    fprintf(fp, "total %f chest=%d head=%d footL=%d\n", total, chest, head, footL);

    for(float t = 0.0f; t <= total + 0.001f; t += 1.0f) {
        MV1SetAttachAnimTime(model, attach, t);
        VECTOR c = MV1GetFramePosition(model, chest);
        VECTOR h = MV1GetFramePosition(model, head);
        VECTOR f = MV1GetFramePosition(model, footL);
        fprintf(fp, "t=%5.1f chest(%7.2f %7.2f %7.2f) head(%7.2f %7.2f %7.2f) footL(%7.2f %7.2f %7.2f)\n", t, c.x, c.y, c.z, h.x, h.y, h.z, f.x,
                f.y, f.z);
    }

    fclose(fp);
    printf("verify ok: total=%f\n", total);
    return 0;
}
//Draw the model at several moments of 2 animations into one picture (row 1 = anim A, row 2 = anim B)
//(2つのアニメのいくつかの瞬間のモデルを1枚の絵にする(1行目＝アニメA、2行目＝アニメB))
int Render(const char* modelPath, const char* animA, const char* animB, const char* view, const char* outPath)
{
    const int COLS = 8;
    int       model = MV1LoadModel(modelPath);
    int       anims[2] = {MV1LoadModel(animA), MV1LoadModel(animB)};
    if(model < 0 || anims[0] < 0 || anims[1] < 0) {
        printf("load failed\n");
        return 1;
    }

    bool side = strcmp(view, "side") == 0;

    SetDrawScreen(DX_SCREEN_BACK);
    SetBackgroundColor(70, 80, 95);
    ClearDrawScreen();
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    //One copy of the model per picture(絵ごとにモデルを1つ複製)
    for(int row = 0; row < 2; ++row) {
        int   tmp    = MV1DuplicateModel(model);
        int   at     = MV1AttachAnim(tmp, 0, anims[row], TRUE);
        float total  = MV1GetAttachAnimTotalTime(tmp, at);
        MV1DeleteModel(tmp);

        for(int col = 0; col < COLS; ++col) {
            int m = MV1DuplicateModel(model);
            int a = MV1AttachAnim(m, 0, anims[row], TRUE);
            MV1SetAttachAnimTime(m, a, total * col / (COLS - 1));

            //Each picture gets its own camera viewport(絵ごとに自分のカメラの範囲)
            int w = 1600 / COLS, h = 360;
            int x0 = col * w, y0 = row * h;
            SetDrawArea(x0, y0, x0 + w, y0 + h);
            SetCameraScreenCenter((float)(x0 + w / 2), (float)(y0 + h / 2));
            SetupCamera_Ortho(1250.0f);
            SetCameraNearFar(10.0f, 5000.0f);
            if(side)
                SetCameraPositionAndTarget_UpVecY(VGet(1500.0f, 120.0f, 120.0f), VGet(0.0f, 120.0f, 120.0f));    //From the side(横から)
            else
                SetCameraPositionAndTarget_UpVecY(VGet(1100.0f, 700.0f, -1100.0f), VGet(0.0f, 90.0f, 120.0f));    //From front-right above(右前の上から)

            //Cell background and light from the camera side(セルの背景と、カメラ側からの光)
            DrawBox(x0, y0, x0 + w, y0 + h, GetColor(70, 80, 95), TRUE);
            SetLightDirection(side ? VGet(-1.0f, -0.6f, 0.3f) : VGet(-0.6f, -0.8f, 0.6f));

            //Ground line(地面の線)
            DrawLine3D(VGet(0, 0, -300), VGet(0, 0, 500), GetColor(200, 200, 200));
            DrawLine3D(VGet(-300, 0, 0), VGet(300, 0, 0), GetColor(120, 120, 120));
            MV1DrawModel(m);
            MV1DeleteModel(m);

            SetDrawArea(0, 0, 1600, 720);
            DrawFormatString(x0 + 4, y0 + 4, GetColor(255, 255, 0), "%s t=%.1f", row == 0 ? "A" : "B", total * col / (COLS - 1));
        }
    }

    SaveDrawScreenToPNG(0, 0, 1600, 720, outPath);
    printf("render ok: %s\n", outPath);
    return 0;
}
}    // namespace

int main(int argc, char** argv)
{
    if(argc < 4) {
        printf("usage: AnimTool dump|convert|verify ...\n");
        return 1;
    }

    //Small hidden-ish window. DxLib needs a window to load models(小さなウィンドウ。DxLibはモデル読み込みにウィンドウが必要)
    ChangeWindowMode(TRUE);
    SetGraphMode(1600, 720, 32);
    SetOutApplicationLogValidFlag(FALSE);
    SetAlwaysRunFlag(TRUE);
    if(DxLib_Init() == -1)
        return 9;

    int ret = 1;
    if(strcmp(argv[1], "dump") == 0 && argc >= 5)
        ret = Dump(argv[2], argv[3], argv[4]);
    else if(strcmp(argv[1], "convert") == 0)
        ret = Convert(argv[2], argv[3]);
    else if(strcmp(argv[1], "verify") == 0 && argc >= 5)
        ret = Verify(argv[2], argv[3], argv[4]);
    else if(strcmp(argv[1], "info") == 0) {
        //List the animations inside each file(各ファイルの中のアニメを一覧にする)
        for(int i = 2; i < argc; ++i) {
            int h = MV1LoadModel(argv[i]);
            printf("%s : handle=%d anims=%d frames=%d\n", argv[i], h, h >= 0 ? MV1GetAnimNum(h) : -1, h >= 0 ? MV1GetFrameNum(h) : -1);
            for(int a = 0; h >= 0 && a < MV1GetAnimNum(h); ++a) printf("   [%d] %s total=%.1f\n", a, MV1GetAnimName(h, a), MV1GetAnimTotalTime(h, a));
        }
        ret = 0;
    }
    else if(strcmp(argv[1], "render") == 0 && argc >= 7)
        ret = Render(argv[2], argv[3], argv[4], argv[5], argv[6]);

    DxLib_End();
    return ret;
}
