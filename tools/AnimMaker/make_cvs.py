# -*- coding: utf-8 -*-
"""
make_cvs.py
Animations for cha0002 "バイト (CVS)": fast legs, the fastest throw, low HP, easily blown away.
Uses the same machinery as make_knockdown.py (same rig: CVS.mv1 has the same bones as Salaryman.mv1).
(cha0002「バイト(CVS)」用のアニメ：足が速い・投げが最速・HPが低い・吹き飛びやすい。
 make_knockdown.py と同じしくみを使う(CVS.mv1 は Salaryman.mv1 と同じボーン))

Makes(作るもの):
    Run.x    : run cycle, loops (16 frames = 0.53 s at 30 fps)(走るループ。16フレーム＝0.53秒)
    Down.x   : big blow-away: flies higher and further than the Player (18 frames)(大きく吹き飛ぶ。プレイヤーより高く遠くへ)
    GetUp.x  : quick hop-up from the back (14 frames)(仰向けから素早く跳ね起きる)

Usage(使い方):
    AnimTool.exe dump data/Game/Models/Enemy/CVS/CVS.mv1 data/Game/Models/Enemy/CVS/Anims/Idle.mv1 bones_cvs.txt
    python make_cvs.py bones_cvs.txt <output folder>
    AnimTool.exe convert Run.x Run.mv1   (same for Down.x, GetUp.x)(Down.x, GetUp.x も同じ)
"""
import os
import sys

# The tool folder itself, so "import make_knockdown" works even with "python -I"(python -I でも import できるようにツールのフォルダを足す)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_knockdown as core  # noqa: E402

LEG_L, LEG_R = core.LEG_L, core.LEG_R
KNEE_L, KNEE_R = core.KNEE_L, core.KNEE_R
ARM_L_UP, ARM_L_LOW = "Arm:Left:Upper", "Arm:Left:Lower"
ARM_R_UP, ARM_R_LOW = "Arm:Right:Upper", "Arm:Right:Lower"

# ---------------------------------------------------------------------------
# Run cycle. Built on the REST pose (T-pose), not on Idle, because Idle is a side-on stance and a run must be
# left/right even. Arms: first lowered with Z, then swung with X ("order": "zx").
# Signs (see core.rot3): leg "x" + = swing FORWARD, - = swing back. knee "x" - = bend (shin goes back).
# Arm "x" + = swing forward. Left arm "z" - = down, right arm "z" + = down. Body "x" - = lean forward.
# (走るループ。Idleではなく「初期姿勢(Tポーズ)」をもとに作る。Idleは横向きの構えなので、左右対称の走りにならないため。
#  腕は先にZで下ろし、それからXで振る(「order」: "zx")。
#  符号(core.rot3参照)：脚「x」+＝前へ振る、-＝後ろへ。ひざ「x」-＝曲げる(すねが後ろへ)。
#  腕「x」+＝前へ振る。左腕「z」-＝下ろす、右腕「z」+＝下ろす。Body「x」-＝前かがみ)
# ---------------------------------------------------------------------------
def run_pose(phase_right_forward, passing):
    """phase_right_forward: True = right leg in front. passing: True = legs pass under the body (body is highest)
    (phase_right_forward：True＝右脚が前。passing：True＝脚が体の下を通る瞬間(体が一番高い))"""
    s = 1.0 if phase_right_forward else -1.0
    if passing:
        legs = {LEG_R: {"x": 8.0 * s}, LEG_L: {"x": -8.0 * s},
                KNEE_R: {"x": -95.0 if s < 0 else -25.0}, KNEE_L: {"x": -95.0 if s > 0 else -25.0}}
        swing, lift = 15.0, 10.0
    else:
        legs = {LEG_R: {"x": 48.0 * s}, LEG_L: {"x": -48.0 * s},
                KNEE_R: {"x": -15.0 if s > 0 else -70.0}, KNEE_L: {"x": -15.0 if s < 0 else -70.0}}
        swing, lift = 55.0, 0.0

    bones = {
        "Body": {"x": -14.0},              # lean forward(前かがみ)
        "Head": {"x": 8.0},                # keep the face up(顔は前を見る)
        # Arms swing opposite to the legs(腕は脚と逆に振る)
        ARM_L_UP: {"z": -72.0, "x": swing * s, "order": "zx"},
        ARM_R_UP: {"z": 72.0, "x": -swing * s, "order": "zx"},
        # Forearms bent forward, like a runner(ランナーのようにひじを前に曲げる)
        ARM_L_LOW: {"y": 75.0},
        ARM_R_LOW: {"y": -75.0},
    }
    bones.update(legs)
    return {"root": {"rot": 0.0, "z": 0.0, "feet": True, "lift": lift}, "bones": bones}


RUN_A = run_pose(True, False)     # right leg forward(右脚が前)
RUN_B = run_pose(True, True)      # passing(脚が通過)
RUN_C = run_pose(False, False)    # left leg forward(左脚が前)
RUN_D = run_pose(False, True)     # passing(脚が通過)
ANIM_RUN = [(0, RUN_A, "linear"), (4, RUN_B, "inout"), (8, RUN_C, "inout"), (12, RUN_D, "inout"), (16, RUN_A, "inout")]

# ---------------------------------------------------------------------------
# Big blow-away Down (on top of the Idle pose, like the Player). Flies HIGHER, legs kick wildly.
# The far sideways travel is NOT in the animation: the game slides the character (knockback x1.6 for CVS in code),
# so the body never has to come back to its old spot when it gets up
# (大きく吹き飛ぶDown(プレイヤーと同じくIdleの姿勢がもと)。より「高く」飛び、脚を大きくばたつかせる。
#  横に遠くまで飛ぶ分はアニメに入れない：ゲームがキャラを滑らせる(コードでCVSはふっとび1.6倍)ので、
#  起き上がる時に体が元の場所へ戻る動きにならない)
# ---------------------------------------------------------------------------
CVS_HIT = {
    "root": {"rot": 22.0, "y": 22.0, "z": 12.0},
    "bones": {"Body": {"x": 18.0}, "Chest": {"x": 12.0}, "Head": {"x": 30.0},
              ARM_L_UP: {"z": 90.0, "x": 30.0}, ARM_R_UP: {"z": -90.0, "x": 30.0},
              LEG_L: {"x": 30.0}, LEG_R: {"x": 50.0}, KNEE_R: {"x": -40.0}},
}
CVS_AIR_HIGH = {
    "root": {"rot": 50.0, "y": 70.0, "z": 20.0},
    "bones": {"Body": {"x": 12.0}, "Head": {"x": 20.0},
              ARM_L_UP: {"z": 110.0, "x": 10.0}, ARM_R_UP: {"z": -110.0, "x": 10.0},
              LEG_L: {"x": 70.0}, LEG_R: {"x": 30.0}, KNEE_L: {"x": -50.0}, KNEE_R: {"x": -20.0}},
}
CVS_AIR_LOW = {
    "root": {"rot": 75.0, "y": 40.0, "z": 30.0},
    "bones": {"Body": {"x": 8.0}, "Head": {"x": 12.0},
              ARM_L_UP: {"z": 100.0}, ARM_R_UP: {"z": -100.0},
              LEG_L: {"x": 40.0}, LEG_R: {"x": 75.0}, KNEE_L: {"x": -20.0}, KNEE_R: {"x": -60.0}},
}
CVS_LAND = {
    "root": {"rot": 86.0, "z": 36.0, "ground": True},
    "bones": {"Body": {"x": 4.0}, "Head": {"x": 6.0},
              ARM_L_UP: {"z": 88.0, "x": -10.0}, ARM_R_UP: {"z": -88.0, "x": -10.0},
              LEG_L: {"x": 20.0, "z": 14.0}, LEG_R: {"x": 26.0, "z": -14.0}, KNEE_L: {"x": -15.0}, KNEE_R: {"x": -20.0}},
}
CVS_BOUNCE = {
    "root": {"rot": 80.0, "z": 38.0, "ground": True, "lift": 14.0},
    "bones": {"Head": {"x": -6.0},
              ARM_L_UP: {"z": 100.0}, ARM_R_UP: {"z": -100.0},
              LEG_L: {"x": 45.0, "z": 10.0}, LEG_R: {"x": 50.0, "z": -10.0}, KNEE_L: {"x": -25.0}, KNEE_R: {"x": -30.0}},
}
CVS_LIE = {
    "root": {"rot": 84.0, "z": 40.0, "ground": True},
    "bones": {"Body": {"x": 3.0}, "Head": {"x": 4.0},
              ARM_L_UP: {"z": 90.0, "x": -8.0}, ARM_R_UP: {"z": -90.0, "x": -8.0},
              LEG_L: {"x": 8.0, "z": 14.0}, LEG_R: {"x": 10.0, "z": -14.0}, KNEE_L: {"x": -12.0}, KNEE_R: {"x": -15.0}},
}
ANIM_DOWN = [(0, core.POSE_IDLE, "linear"), (3, CVS_HIT, "out"), (7, CVS_AIR_HIGH, "out"), (11, CVS_AIR_LOW, "in"),
             (13, CVS_LAND, "in"), (15, CVS_BOUNCE, "out"), (18, CVS_LIE, "inout")]

# ---------------------------------------------------------------------------
# Quick hop-up GetUp: tuck the legs, roll back a little, then spring up onto the feet
# (素早く跳ね起きるGetUp：脚を抱えて少し後ろに転がり、はずみで足から立ち上がる)
# ---------------------------------------------------------------------------
CVS_TUCK = {
    "root": {"rot": 100.0, "z": 44.0, "ground": True},
    "bones": {"Body": {"x": -15.0}, "Head": {"x": -10.0},
              ARM_L_UP: {"z": 40.0, "x": 60.0}, ARM_R_UP: {"z": -40.0, "x": 60.0},
              LEG_L: {"x": 110.0}, LEG_R: {"x": 110.0}, KNEE_L: {"x": -110.0}, KNEE_R: {"x": -110.0}},
}
CVS_SPRING = {
    "root": {"rot": 15.0, "y": 30.0, "z": 30.0},
    "bones": {"Body": {"x": -10.0}, "Head": {"x": 5.0},
              ARM_L_UP: {"z": 80.0, "x": 20.0}, ARM_R_UP: {"z": -80.0, "x": 20.0},
              LEG_L: {"x": 45.0}, LEG_R: {"x": 45.0}, KNEE_L: {"x": -80.0}, KNEE_R: {"x": -80.0}},
}
CVS_LAND_CROUCH = {
    "root": {"rot": 0.0, "z": 10.0, "feet": True},
    "bones": {"Body": {"x": -20.0}, "Head": {"x": 10.0},
              ARM_L_UP: {"z": 30.0, "x": -20.0}, ARM_R_UP: {"z": -30.0, "x": -20.0},
              LEG_L: {"x": 50.0}, LEG_R: {"x": 50.0}, KNEE_L: {"x": -95.0}, KNEE_R: {"x": -95.0}},
}
# Ends at Idle again, at the place it landed (the root moves back to 0 = the feet of the object)
# (最後はまたIdle。ルートは0に戻る＝オブジェクトの足元)
ANIM_GETUP = [(0, CVS_LIE, "linear"), (3, CVS_TUCK, "inout"), (7, CVS_SPRING, "out"), (10, CVS_LAND_CROUCH, "in"),
              (14, core.POSE_IDLE, "inout")]


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    rest, idle = core.load_bones(sys.argv[1])
    out_dir = sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)

    # Run uses the REST pose as its base, the others use Idle(Runは初期姿勢がもと、ほかはIdleがもと)
    jobs = (("Run", rest, ANIM_RUN), ("Down", idle, ANIM_DOWN), ("GetUp", idle, ANIM_GETUP))
    for name, base, timeline in jobs:
        rig = core.Rig(base)
        base_world = {i: base[i][2] for i in rig.order}
        frames = core.bake(rig, timeline)
        core.write_x(os.path.join(out_dir, name + ".x"), rig, base_world, name, frames)

        idx = {rig.names[i]: i for i in rig.order}
        print("== %s (%d frames)" % (name, len(frames)))
        for f, w in enumerate(frames):
            h, ft = w[idx["Head_end"]][3], w[idx["Leg:Left:Lower_end_end"]][3]
            low, low_name = rig.lowest(w, with_name=True)
            print("  f%2d head(%6.1f,%6.1f,%6.1f) footL(%6.1f,%6.1f,%6.1f) low=%6.1f %s" %
                  (f, h[0], h[1], h[2], ft[0], ft[1], ft[2], low, low_name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
