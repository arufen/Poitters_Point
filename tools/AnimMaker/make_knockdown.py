# -*- coding: utf-8 -*-
"""
make_knockdown.py
Make the Player's knockdown animations (Down = hit and fall on the back, GetUp = stand up again)
as DirectX .x files. AnimTool.exe then converts them to .mv1.
(プレイヤーのダウンアニメ(Down＝当たって仰向けに倒れる、GetUp＝また立ち上がる)を DirectX の .x ファイルで作る。
 そのあと AnimTool.exe で .mv1 に変換する)

Usage(使い方):
    python make_knockdown.py <bones.txt from "AnimTool dump"> <output folder>

How it works(しくみ):
  1. bones.txt has every bone's WORLD matrix in the first frame of Idle.mv1
     (bones.txt にはIdle.mv1の最初のフレームでの全ボーンのワールド行列が入っている)
  2. Each key pose below says "rotate this bone by N degrees" ON TOP of that idle pose
     (下のキーポーズは、そのアイドル姿勢の「上に」「このボーンをN度回す」と書いてある)
  3. Forward kinematics: children follow their parents, the root can rotate around the feet
     (フォワードキネマティクス：子は親についていき、ルートは足元を中心に回れる)
  4. "ground" poses are lifted/lowered so the body lies ON the ground, never inside it
     (「ground」のポーズは、体が地面にめり込まず、地面の「上」にのるように上下を合わせる)
  5. Every frame in between is blended with an ease curve and written as matrix keys
     (間のフレームはイーズカーブでつないで、行列のキーとして書き出す)

Coordinate system: DxLib (left-handed). The model looks toward -Z, so +Z is BACKWARD. Y is up.
(座標系：DxLib(左手系)。モデルは-Zを向いているので+Zが「後ろ」。Yが上)
"""
import math
import os
import sys

FPS = 30

# ---------------------------------------------------------------------------
# Small 4x4 matrix helpers. Row vectors like DxLib: world = local * parentWorld, translation in row 3
# (4x4行列の小さな関数。DxLibと同じ行ベクトル：world = local × 親world、平行移動は3行目)
# ---------------------------------------------------------------------------
def mat_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def mat_identity():
    return [[1.0 if r == c else 0.0 for c in range(4)] for r in range(4)]


def mat_inverse(m):
    # General 4x4 inverse (Gauss-Jordan)(一般的な4x4の逆行列(ガウス・ジョルダン法))
    a = [row[:] + [1.0 if r == c else 0.0 for c in range(4)] for r, row in enumerate(m)]
    for col in range(4):
        piv = max(range(col, 4), key=lambda r: abs(a[r][col]))
        a[col], a[piv] = a[piv], a[col]
        p = a[col][col]
        a[col] = [v / p for v in a[col]]
        for r in range(4):
            if r != col:
                f = a[r][col]
                a[r] = [a[r][c] - f * a[col][c] for c in range(8)]
    return [row[4:] for row in a]


def rot3(axis, deg):
    """3x3 rotation for ROW vectors. Sign rules used in this file(この関数の符号のルール):
       axis 'x', +deg : something pointing UP tips BACKWARD (+Z); a leg pointing DOWN swings FORWARD (-Z)
                        (上を向いた物は後ろ(+Z)へ倒れる。下を向いた脚は前(-Z)へ振れる)
       axis 'z', +deg : something pointing to +X turns UP (+Y)(+Xを向いた物が上(+Y)へ回る)
       axis 'y', +deg : turn around the up axis(上の軸のまわりに回る)"""
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    if axis == "x":
        return [[1, 0, 0], [0, c, s], [0, -s, c]]
    if axis == "z":
        return [[c, s, 0], [-s, c, 0], [0, 0, 1]]
    return [[c, 0, -s], [0, 1, 0], [s, 0, c]]


def m3_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(3)) for c in range(3)] for r in range(3)]


def m3_identity():
    return [[1.0 if r == c else 0.0 for c in range(3)] for r in range(3)]


def v3_mul(v, m):
    return [sum(v[k] * m[k][c] for k in range(3)) for c in range(3)]


def rotation_from(spec):
    """spec = {'x': deg, 'y': deg, 'z': deg}, applied X then Y then Z(X→Y→Zの順にかける)"""
    m = m3_identity()
    for axis in ("x", "y", "z"):
        if spec.get(axis, 0.0) != 0.0:
            m = m3_mul(m, rot3(axis, spec[axis]))
    return m


# ---------------------------------------------------------------------------
# Read bones.txt(bones.txtを読む)
# ---------------------------------------------------------------------------
def load_bones(path):
    rest, idle = {}, {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.split()
            if not p or p[0] not in ("rest", "idle"):
                continue
            idx, name, parent = int(p[1]), p[2], int(p[3])
            vals = [float(v) for v in p[4:20]]
            mat = [vals[0:4], vals[4:8], vals[8:12], vals[12:16]]
            (rest if p[0] == "rest" else idle)[idx] = (name, parent, mat)
    return rest, idle


# ---------------------------------------------------------------------------
# Key poses(キーポーズ)
# Bone names are the real ones from Salaryman.mv1. Angles are in degrees, see rot3() for the signs.
# "root" moves the whole body: rotation around the feet + offset. ground=True = sit on the ground automatically
# (ボーン名はSalaryman.mv1の本物の名前。角度は度、符号はrot3()を参照。
#  「root」は体全体を動かす：足元を中心に回転＋移動。ground=Trueなら自動で地面の上にのせる)
# ---------------------------------------------------------------------------
LEG_L, LEG_R = "Leg:Left:Upper", "Leg:Right:Upper"
KNEE_L, KNEE_R = "Leg:Left:Lower", "Leg:Right:Lower"
ARM_L, ARM_R = "Arm:Left:Upper", "Arm:Left:Lower"
ARM_R_UP, ARM_R_LOW = "Arm:Right:Upper", "Arm:Right:Lower"

POSE_IDLE = {"root": {"rot": 0.0, "y": 0.0, "z": 0.0}, "bones": {}}

# Hit: the upper body snaps back, arms fly up, small hop(当たった瞬間：上半身が後ろにのけぞり、腕が上がり、小さく跳ねる)
POSE_HIT = {
    "root": {"rot": 18.0, "y": 14.0, "z": 6.0},
    "bones": {
        "Body": {"x": 15.0},
        "Chest": {"x": 12.0},
        "Head": {"x": 25.0},
        ARM_L: {"z": 75.0, "x": 25.0},
        "Arm:Left:Lower": {"z": 20.0},
        ARM_R_UP: {"z": -75.0, "x": 25.0},
        ARM_R_LOW: {"z": -20.0},
        LEG_L: {"x": 20.0},
        LEG_R: {"x": 35.0},
        KNEE_R: {"x": -30.0},
    },
}

# In the air, falling backward, legs kick up(空中で後ろに倒れていく。脚が上にはね上がる)
POSE_AIR = {
    "root": {"rot": 55.0, "y": 22.0, "z": 18.0},
    "bones": {
        "Body": {"x": 10.0},
        "Chest": {"x": 8.0},
        "Head": {"x": 15.0},
        ARM_L: {"z": 95.0, "x": 10.0},
        "Arm:Left:Lower": {"z": 10.0},
        ARM_R_UP: {"z": -95.0, "x": 10.0},
        ARM_R_LOW: {"z": -10.0},
        LEG_L: {"x": 45.0},
        LEG_R: {"x": 55.0},
        KNEE_L: {"x": -25.0},
        KNEE_R: {"x": -35.0},
    },
}

# Hits the ground on the back, spread out like a star (大の字)(背中から地面に落ちる。大の字)
POSE_LAND = {
    "root": {"rot": 84.0, "z": 26.0, "ground": True},
    "bones": {
        "Body": {"x": 4.0},
        "Head": {"x": 6.0},
        ARM_L: {"z": 85.0, "x": -10.0},
        ARM_R_UP: {"z": -85.0, "x": -10.0},
        LEG_L: {"x": 14.0, "z": 12.0},
        LEG_R: {"x": 18.0, "z": -12.0},
        KNEE_L: {"x": -15.0},
        KNEE_R: {"x": -20.0},
    },
}

# Small bounce after landing(着地のあとの小さなバウンド)
POSE_BOUNCE = {
    "root": {"rot": 80.0, "z": 28.0, "ground": True, "lift": 8.0},
    "bones": {
        "Body": {"x": 2.0},
        "Head": {"x": -4.0},
        ARM_L: {"z": 95.0, "x": -5.0},
        ARM_R_UP: {"z": -95.0, "x": -5.0},
        LEG_L: {"x": 40.0, "z": 10.0},
        LEG_R: {"x": 45.0, "z": -10.0},
        KNEE_L: {"x": -20.0},
        KNEE_R: {"x": -25.0},
    },
}

# Lying still (end of Down, start of GetUp)(倒れて止まっている(Downの最後、GetUpの最初))
POSE_LIE = {
    "root": {"rot": 84.0, "z": 28.0, "ground": True},
    "bones": {
        "Body": {"x": 3.0},
        "Head": {"x": 4.0},
        ARM_L: {"z": 88.0, "x": -8.0},
        ARM_R_UP: {"z": -88.0, "x": -8.0},
        LEG_L: {"x": 8.0, "z": 12.0},
        LEG_R: {"x": 10.0, "z": -12.0},
        KNEE_L: {"x": -12.0},
        KNEE_R: {"x": -15.0},
    },
}

# Sit up with the legs stretched forward, hands on the ground behind. The legs are short (thigh, shin and boot are
# ~37 each), so a knees-up sit can not reach the ground: a long-sit fits this chibi body better
# (脚を前に伸ばして座り、後ろの地面に手をつく。脚が短い(太もも・すね・ブーツが各約37)ので、ひざを立てた座り方では
#  お尻が地面に届かない。このデフォルメ体型には脚を伸ばした座り方が合う)
POSE_SIT = {
    "root": {"rot": 18.0, "z": 20.0, "ground": True},
    "bones": {
        "Body": {"x": -8.0},
        "Head": {"x": -6.0},
        ARM_L: {"z": 25.0, "x": 45.0},
        ARM_R_UP: {"z": -25.0, "x": 45.0},
        LEG_L: {"x": 72.0, "z": 8.0},
        LEG_R: {"x": 72.0, "z": -8.0},
        KNEE_L: {"x": -5.0},
        KNEE_R: {"x": -5.0},
    },
}

# Crouch: feet on the ground, getting up(しゃがみ：足を地面について立ち上がっていく)
POSE_CROUCH = {
    "root": {"rot": 0.0, "z": 8.0, "feet": True},
    "bones": {
        "Body": {"x": -25.0},
        "Chest": {"x": -5.0},
        "Head": {"x": 10.0},
        ARM_L: {"z": 20.0, "x": -25.0},
        ARM_R_UP: {"z": -20.0, "x": -25.0},
        LEG_L: {"x": 55.0},
        LEG_R: {"x": 55.0},
        KNEE_L: {"x": -100.0},
        KNEE_R: {"x": -100.0},
    },
}

# Almost standing, a little shake of the head(ほぼ立った。頭を少しふる)
POSE_STAND = {
    "root": {"rot": -4.0, "z": 2.0, "feet": True},
    "bones": {
        "Body": {"x": -6.0},
        "Head": {"x": -8.0, "y": 12.0},
        LEG_L: {"x": 12.0},
        LEG_R: {"x": 12.0},
        KNEE_L: {"x": -20.0},
        KNEE_R: {"x": -20.0},
    },
}

# Timelines: (frame, pose, ease to this key)(タイムライン：(フレーム, ポーズ, このキーへのイーズ))
#   Down  : 15 frames = 0.50 s (hit -> fly -> land -> bounce -> lie)(当たる→飛ぶ→落ちる→バウンド→倒れる)
#   GetUp : 15 frames = 0.50 s (lie -> sit up -> crouch -> stand -> idle)(倒れている→起きる→しゃがむ→立つ→アイドル)
ANIM_DOWN = [(0, POSE_IDLE, "linear"), (3, POSE_HIT, "out"), (7, POSE_AIR, "inout"), (10, POSE_LAND, "in"), (12, POSE_BOUNCE, "out"),
             (15, POSE_LIE, "inout")]
ANIM_GETUP = [(0, POSE_LIE, "linear"), (5, POSE_SIT, "inout"), (10, POSE_CROUCH, "inout"), (13, POSE_STAND, "out"), (15, POSE_IDLE, "inout")]

# Body thickness around each bone, used to keep the mesh above the ground (guesses, the head is big)
# (各ボーンのまわりの体の厚み。メッシュを地面より上に保つために使う(目安。頭が大きい))
RADIUS = {
    "Head_end": 48.0, "Head": 30.0, "Chest": 30.0, "Body": 20.0, "center": 14.0,
    "Arm:Left:Lower_end_end": 8.0, "Arm:Right:Lower_end_end": 8.0, "Arm:Left:Lower_end": 9.0, "Arm:Right:Lower_end": 9.0,
    "Leg:Left:Lower_end_end": 2.0, "Leg:Right:Lower_end_end": 2.0, "Leg:Left:Lower_end": 9.0, "Leg:Right:Lower_end": 9.0,
    "Leg:Left:Lower": 10.0, "Leg:Right:Lower": 10.0,
}


# ---------------------------------------------------------------------------
# Forward kinematics(フォワードキネマティクス)
# ---------------------------------------------------------------------------
class Rig:
    def __init__(self, idle):
        self.idle = idle
        # Only real bones: the armature and everything under it(本物のボーンだけ：アーマチュアとその下全部)
        self.root = next(i for i, (n, p, m) in idle.items() if n.endswith(".arma"))
        self.order = []
        self._collect(self.root)
        self.names = {i: idle[i][0] for i in self.order}

    def _collect(self, i):
        self.order.append(i)
        for j, (n, p, m) in sorted(self.idle.items()):
            if p == i:
                self._collect(j)

    def solve(self, pose, lift_auto=0.0):
        """Return {bone index: world matrix} for a pose(ポーズの全ボーンのワールド行列を返す)"""
        bones, root = pose["bones"], pose["root"]
        world, delta = {}, {}
        for i in self.order:
            name, parent, w_idle = self.idle[i]
            d_own = rotation_from(bones.get(name, {}))
            p_idle = w_idle[3][:3]
            if i == self.root:
                # Root: turn around the feet (origin) + move(ルート：足元(原点)を中心に回る＋移動)
                d = m3_mul(d_own, rot3("x", root.get("rot", 0.0)))
                pos = v3_mul(p_idle, d)
                pos = [pos[0], pos[1] + root.get("y", 0.0) + root.get("lift", 0.0) + lift_auto, pos[2] + root.get("z", 0.0)]
            else:
                d = m3_mul(d_own, delta[parent])    # own turn first, then everything the parent did(自分の回転→親の動き全部)
                off = [p_idle[k] - self.idle[parent][2][3][k] for k in range(3)]
                off = v3_mul(off, delta[parent])    # the joint is carried by the parent(関節は親に運ばれる)
                pos = [world[parent][3][k] + off[k] for k in range(3)]
            delta[i] = d
            rot = [v3_mul(w_idle[r][:3], d) for r in range(3)]    # idle rotation, then the turn(アイドルの回転→回す)
            world[i] = [rot[0] + [0.0], rot[1] + [0.0], rot[2] + [0.0], pos + [1.0]]
        return world

    def lowest(self, world, feet_only=False, with_name=False):
        """How far the lowest part of the body is above the ground(体の一番低い所が地面からどれだけ上か)"""
        low, low_name = 1e9, ""
        for i in self.order:
            n = self.names[i]
            if n.endswith(".arma") or n == "MAIN":
                continue    # Helper bones, not part of the body(補助のボーン。体の一部ではない)
            if feet_only and not n.startswith("Leg:"):
                continue
            r = 0.0 if feet_only else RADIUS.get(n, 6.0)
            if world[i][3][1] - r < low:
                low, low_name = world[i][3][1] - r, n
        return (low, low_name) if with_name else low

    def solve_grounded(self, pose):
        root = pose["root"]
        world = self.solve(pose)
        if root.get("ground") or root.get("feet"):
            # Move up/down so the lowest point touches the ground exactly(一番低い所がちょうど地面につくように上下させる)
            low = self.lowest(world, feet_only=bool(root.get("feet")))
            world = self.solve(pose, lift_auto=-low)
        return world


def ease(kind, t):
    if kind == "in":
        return t * t
    if kind == "out":
        return 1.0 - (1.0 - t) * (1.0 - t)
    if kind == "inout":
        return t * t * (3.0 - 2.0 * t)
    return t


def lerp_pose(a, b, t):
    """Blend two poses: angles and root values are mixed(2つのポーズをまぜる：角度とルートの値をまぜる)"""
    out = {"root": {}, "bones": {}}
    for k in ("rot", "y", "z", "lift"):
        out["root"][k] = a["root"].get(k, 0.0) * (1 - t) + b["root"].get(k, 0.0) * t
    # Ground rule of the target pose wins at the end, of the start pose at the beginning(地面のルールは前半は始めのポーズ、後半は目標のポーズ)
    src = a if t < 0.5 else b
    for k in ("ground", "feet"):
        if src["root"].get(k):
            out["root"][k] = True
    for name in set(a["bones"]) | set(b["bones"]):
        sa, sb = a["bones"].get(name, {}), b["bones"].get(name, {})
        out["bones"][name] = {ax: sa.get(ax, 0.0) * (1 - t) + sb.get(ax, 0.0) * t for ax in ("x", "y", "z")}
    return out


def bake(rig, timeline):
    """One pose per frame(フレームごとに1ポーズ)"""
    frames = []
    last = timeline[-1][0]
    for f in range(last + 1):
        for (f0, p0, _), (f1, p1, kind) in zip(timeline, timeline[1:]):
            if f0 <= f <= f1:
                t = 0.0 if f1 == f0 else (f - f0) / (f1 - f0)
                frames.append(rig.solve_grounded(lerp_pose(p0, p1, ease(kind, t))))
                break
    return frames


# ---------------------------------------------------------------------------
# Write .x (skeleton only + one AnimationSet with matrix keys)(.xを書く(骨だけ＋行列キーのアニメ1つ))
# ---------------------------------------------------------------------------
def fmt_matrix(m):
    return ",".join("%.6f" % m[r][c] for r in range(4) for c in range(4))


def local_of(rig, world, i):
    parent = rig.idle[i][1]
    if i == rig.root or parent not in world:
        return world[i]
    return mat_mul(world[i], mat_inverse(world[parent]))


def write_x(path, rig, rest_world, anim_name, frames):
    out = ["xof 0303txt 0032", ""]

    # Skeleton in the idle pose (the game uses Salaryman.mv1's own skeleton, this is only for the keys)
    # (アイドル姿勢の骨。ゲームはSalaryman.mv1の骨を使う。これはキーのためだけ)
    def frame_block(i, indent):
        sp = "  " * indent
        name = rig.names[i]
        out.append("%sFrame %s {" % (sp, name))
        out.append("%s  FrameTransformMatrix { %s;; }" % (sp, fmt_matrix(local_of(rig, rest_world, i))))
        for j in rig.order:
            if rig.idle[j][1] == i:
                frame_block(j, indent + 1)
        out.append("%s}" % sp)

    frame_block(rig.root, 0)
    out.append("")
    out.append("AnimTicksPerSecond { %d; }" % FPS)
    out.append("AnimationSet %s {" % anim_name)
    for i in rig.order:
        keys = ["%d;16;%s;;" % (f, fmt_matrix(local_of(rig, w, i))) for f, w in enumerate(frames)]
        out.append("  Animation {")
        out.append("    {%s}" % rig.names[i])
        out.append("    AnimationKey { 4; %d; %s; }" % (len(keys), ",".join(keys)))
        out.append("  }")
    out.append("}")

    with open(path, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(out) + "\n")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    rest, idle = load_bones(sys.argv[1])
    rig = Rig(idle)
    rest_world = {i: idle[i][2] for i in rig.order}
    os.makedirs(sys.argv[2], exist_ok=True)

    for name, timeline in (("Down", ANIM_DOWN), ("GetUp", ANIM_GETUP)):
        frames = bake(rig, timeline)
        path = os.path.join(sys.argv[2], name + ".x")
        write_x(path, rig, rest_world, name, frames)
        # Print the head/chest/foot height per frame so it can be checked without the game(ゲームなしで確認できるよう頭・胸・足の高さを出す)
        idx = {rig.names[i]: i for i in rig.order}
        print("== %s (%d frames)" % (name, len(frames)))
        for f, w in enumerate(frames):
            h, c, ft = w[idx["Head_end"]][3], w[idx["Chest"]][3], w[idx["Leg:Left:Lower_end_end"]][3]
            low, low_name = rig.lowest(w, with_name=True)
            print("  f%2d head(%6.1f,%6.1f,%6.1f) chest(%6.1f,%6.1f,%6.1f) footL(%6.1f,%6.1f,%6.1f) low=%6.1f %s" %
                  (f, h[0], h[1], h[2], c[0], c[1], c[2], ft[0], ft[1], ft[2], low, low_name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
