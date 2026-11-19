//----------------------------------------------------------------------------
//!	@file	ps_model.fx
//!	@brief	MV1モデルピクセルシェーダー
//!	        Simple lighting so you can see the shape and edges of models (not a final look)
//!	        (モデルの形と角が分かるようにするための、かんたんなライティング(最終的な見た目ではない))
//----------------------------------------------------------------------------
#include "dxlib_ps.h"

// 頂点シェーダーの出力
struct VS_OUTPUT_MODEL
{
	float4	position_       : SV_Position;      //!< 座標       (スクリーン空間)
    float4  curr_position_  : CURR_POSITION;    //!< 現在の座標 (スクリーン空間)
	float3	world_position_ : WORLD_POSITION;   //!< ワールド座標
	float3	normal_         : NORMAL0;          //!< 法線
	float4	diffuse_        : COLOR0;           //!< Diffuseカラー
	float2	uv0_            : TEXCOORD0;        //!< テクスチャ座標
    float4  prev_position_  : PREV_POSITION;    //!< 1フレーム前の座標 (スクリーン空間) ※末尾に追加されているため注意
};

typedef	VS_OUTPUT_MODEL	PS_INPUT_MODEL;

//----------------------------------------------------------------------------
// Settings. Save the file while the game runs to see the change
// (設定。ゲーム実行中にファイルを保存すると変化が見える)
//----------------------------------------------------------------------------

// The way the sunlight TRAVELS (from the sky, going down). The shader flips it with a minus sign
// (太陽の光が進む方向(空から下へ)。シェーダーの中でマイナスをつけて反転する)
static const float3 SUN_DIR = normalize(float3(0.5, -1.0, 0.3));

// Light in the shadow side. 0 = black shadows, 1 = no shading at all
// (影側の明るさ。0＝真っ黒な影、1＝陰影なし)
static const float AMBIENT = 0.8;

// true  = every polygon gets one flat brightness, so the edges between polygons are easy to see
// false = smooth shading from the model's own normals
// (true＝ポリゴンごとに同じ明るさになるので、ポリゴンの角がはっきり見える
//  false＝モデルの法線を使ったなめらかな陰影)
static const bool FLAT_SHADING = true;

//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT_MODEL input)
{
	PS_OUTPUT	output;

	float2	uv = input.uv0_;
	float3	N = normalize(input.normal_);	// 法線

	//------------------------------------------------------------
	// Face normal: the real direction of this triangle, measured from the world positions of the pixels next to us.
	// Must be done BEFORE discard, because ddx/ddy read the neighbour pixels.
	// (面の法線：となりのピクセルのワールド座標から測った、この三角形の本当の向き。
	//  ddx/ddyはとなりのピクセルを読むので、discardより前にやること)
	//------------------------------------------------------------
	float3	dp_dx  = ddx(input.world_position_);
	float3	dp_dy  = ddy(input.world_position_);
	float3	face_N = normalize(cross(dp_dy, dp_dx));

	// A cross product can point either way. Turn it to the same side as the model's normal
	// (外積はどちら向きにもなりうる。モデルの法線と同じ側に向ける)
	if(dot(face_N, N) < 0.0) face_N = -face_N;

	//------------------------------------------------------------
	// 法線マップ
	//------------------------------------------------------------
	N = Normalmap(N, input.world_position_, uv);

	//------------------------------------------------------------
	// テクスチャカラーを読み込み
	//------------------------------------------------------------
	float4	textureColor = DiffuseTexture.Sample(DiffuseSampler, uv);

    // アルファテスト
	if(textureColor.a < 0.5) discard;

	// Multiply by the MV1 material diffuse color, so models without a texture are not white
	// (MV1マテリアルのDiffuseカラーを掛ける。テクスチャなしのモデルが白くならないようにする)
	float4	baseColor = textureColor * DxLib_Common.Material.Diffuse;

	//------------------------------------------------------------
	// Lighting: bright where the surface faces the sun, AMBIENT where it faces away
	// (ライティング：太陽の方を向いている面は明るく、反対側はAMBIENTの明るさ)
	//------------------------------------------------------------
	float3	shadeN = FLAT_SHADING ? face_N : N;
	float3	L      = -SUN_DIR;                        // From the surface TOWARD the sun(面から太陽への向き)
	float	ndotl  = saturate(dot(shadeN, L));        // 1 = faces the sun, 0 = side-on or away(1＝太陽向き、0＝横向きか反対)
	float	light  = AMBIENT + (1.0 - AMBIENT) * ndotl;

	output.color0_ = float4(baseColor.rgb * light, baseColor.a);

	// 出力パラメータを返す
	return output;
}
