/*! 
  @file rx_shadow_gl.h
	
  @brief シャドウマップ(GLSL無し版)
	     参考 : http://www.paulsprojects.net/tutorials/smt/smt.html
		 シャドウマップの解像度はウィンドウ解像度以下に制限される
 
  @author Makoto Fujisawa
  @date 2013-
*/
// FILE --rx_shadowmap--

#ifndef _RX_SHADOWMAP_GL_H_
#define _RX_SHADOWMAP_GL_H_


//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
#include "rx_utility.h"

using namespace std;

struct rxFrustum
{
	double Near;
	double Far;
	double FOV;	// deg
	double W, H;
	Vec3 Origin;
	Vec3 LookAt;
	Vec3 Up;
};


/*!
 * 視錐台の生成
 * @param[in] fov_deg 視野角[deg]
 * @param[in] near_d,far_d 視線方向の範囲
 * @param[in] w,h ウィンドウサイズ
 * @param[in] pos 視点位置
 * @param[in] lookat 注視点位置
 * @param[in] up 上方向
 * @return 視錐台
 */
inline rxFrustum CalFrustum(double fov_deg, double near_d, double far_d, int w, int h,
	Vec3 pos, Vec3 lookat = Vec3(0.0), Vec3 up = Vec3(0.0, 1.0, 0.0))
{
	rxFrustum f;
	f.Near = near_d;
	f.Far = far_d;
	f.FOV = fov_deg;
	f.W = w;
	f.H = h;
	f.Origin = pos;
	f.LookAt = lookat;
	f.Up = up;
	return f;
}


//-----------------------------------------------------------------------------
// シャドウマッピングクラス
//-----------------------------------------------------------------------------
class rxShadowMap
{


public:
	//! デフォルトコンストラクタ
	rxShadowMap()
	{
	}

	//! デストラクタ
	~rxShadowMap(){}



	/*!
	 * シャドウマップ用テクスチャの初期化
	 * @param[in] w,h  シャドウマップの解像度
	 */
	int InitShadow(int w_ = 512, int h_ = 512)
	{
		return 1;
	}

	/*!
	 * シャドウマップ(デプステクスチャ)の作成
	 * @param[in] light 光源
	 * @param[in] fpDraw 描画関数ポインタ
	 */
	void RenderSceneWithShadow(rxFrustum &light, void (*fpDraw)(void*), void* func_obj)
	{
		// 描画
		fpDraw(func_obj);
	}


	/*!
	 * デプスマップをテクスチャとして表示
	 * @param[in] w,h ウィンドウサイズ
	 */
	void DrawDepthTex(int w, int h)
	{
	}
};






#endif // #ifdef _RX_SHADOWMAP_H_