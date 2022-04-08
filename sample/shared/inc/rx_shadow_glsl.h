/*! 
  @file rx_shadowmap.h
	
  @brief 
 
  @author Makoto Fujisawa
  @date 2011-
*/
// FILE --rx_shadowmap--

#ifndef _RX_SHADOWMAP_H_
#define _RX_SHADOWMAP_H_


//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext.hpp"	// for glm::to_string()

// GLSL
#include "rx_shaders.h"

using namespace std;

#define USE_GLSL_SHADOW

bool CalInvMat4x4(const GLfloat m[16], GLfloat invm[16]);


//-----------------------------------------------------------------------------
// Shadow Map シェーダ
//-----------------------------------------------------------------------------
const char shadow_vs[] = RXSTR(
// フラグメントシェーダに値を渡すための変数
varying vec4 vPos;
varying vec3 vNrm;
varying vec4 vShadowCoord;	//!< シャドウデプスマップの参照用座標

void main(void)
{
	// フラグメントシェーダでの計算用(モデルビュー変換のみ)
	vPos = gl_ModelViewMatrix*gl_Vertex;			// 頂点位置
	vNrm = normalize(gl_NormalMatrix*gl_Normal);	// 頂点法線
	vShadowCoord = gl_TextureMatrix[7]*gl_ModelViewMatrix*gl_Vertex;	// 影用座標値(光源中心座標)

	// 描画用
	gl_Position = gl_ProjectionMatrix*vPos;	// 頂点位置
	gl_FrontColor = gl_Color;				// 頂点色
	gl_TexCoord[0] = gl_TextureMatrix[0]*gl_MultiTexCoord0;		// 頂点テクスチャ座標
}
);

const char shadow_fs[] = RXSTR(
// バーテックスシェーダから受け取る変数
varying vec4 vPos;
varying vec3 vNrm;
varying vec4 vShadowCoord;


// GLから設定される定数(uniform)
uniform sampler2D tex;			//!< 模様
uniform sampler2D depth_tex;	//!< デプス値テクスチャ

// 影の濃さ
uniform float shadow_ambient;

/*!
 * Phong反射モデルによるシェーディング
 * @return 表面反射色
 */
vec4 PhongShading(vec4 diff)
{
	vec3 N = normalize(vNrm);			// 法線ベクトル
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// ライトベクトル

	// 環境光の計算
	//  - OpenGLが計算した光源強度と反射係数の積(gl_FrontLightProduct)を用いる．
	vec4 ambient = gl_FrontLightProduct[0].ambient;

	// 拡散反射の計算
	float dcoef = max(dot(L, N), 0.0);
	vec4 diffuse = diff*dcoef;

	// 鏡面反射の計算
	vec4 specular = vec4(0.0);
	if(dcoef > 0.0){
		vec3 V = normalize(-vPos.xyz);		// 視線ベクトル

		// 反射ベクトルの計算(フォン反射モデル)
		vec3 R = reflect(-L, N);
		//vec3 R = 2.0*dot(N, L)*N-L;	// reflect関数を用いない場合
		float specularLight = pow(max(dot(R, V), 0.0), gl_FrontMaterial.shininess);

		specular = gl_FrontLightProduct[0].specular*specularLight;
	}
	return ambient+diffuse+specular;
}

/*!
 * 影生成のための係数(影のあるところで1, それ以外で0)
 * @return 影係数(影のあるところで1, それ以外で0)
 */
float ShadowCoef(void)
{
	// 光源座標
	vec4 shadow_coord1 = vShadowCoord/vShadowCoord.w;

	// 光源からのデプス値(視点)
	float view_d = shadow_coord1.z;//-0.0001;
	
	// 格納された光源からの最小デプス値を取得
	float light_d_sum = 0.0;
	float sampling_range = 2.0;
	float scale = 1.0/1024.0;
	for(float y = -sampling_range; y <= sampling_range; y += 1.0){
		for(float x = -sampling_range; x <= sampling_range; x += 1.0){
			vec2 offset = vec2(x, y)*scale;
			light_d_sum += texture2D(depth_tex, shadow_coord1.xy+offset).x;
		}
	}
	float light_d = light_d_sum/((2.0*sampling_range+1.0)*(2.0*sampling_range+1.0));
	//float light_d = texture2D(depth_tex, shadow_coord1.xy).x;

	// 影で0,日向で1
	float shadow_coef = 1.0;
	if(vShadowCoord.w > 0.0){
		shadow_coef = light_d < view_d ? 0.0 : 1.0;
	}

	return shadow_coef;
}

void main(void)
{	
	vec4 diff = gl_FrontLightProduct[0].diffuse;
	vec4 tex_col = texture2D(tex, gl_TexCoord[0].xy*4.0);
	if(tex_col.a > 1e-6) diff = tex_col;

	vec4 light_col = PhongShading(diff);	// 表面反射色
	float shadow_coef = ShadowCoef();	// 影影響係数
	//float alpha = 0.5;
	//light_col = (1.0-alpha)*light_col+alpha*tex_col*light_col;

	// 出力
	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
}
);


//-----------------------------------------------------------------------------
// シャドウマッピングクラス
//-----------------------------------------------------------------------------
class ShadowMap
{
public:
	// 視錐台
	struct Frustum
	{
		float near;
		float far;
		float fov;	// deg
		float w, h;
		glm::vec3 origin;
		glm::vec3 lookat;
		glm::vec3 up;
	};

protected:
	GLuint m_iFBODepth;		//!< 光源から見たときのデプスを格納するFramebuffer object
	GLuint m_iTexDepth;		//!< m_iFBODepthにattachするテクスチャ
	float m_fDepthSize[2];	//!< デプスを格納するテクスチャのサイズ

	rxGLSL m_glslShading;	//!< GLSLシェーダ

public:
	//! デフォルトコンストラクタ
	ShadowMap()
	{
		m_iFBODepth = 0;
		m_iTexDepth = 0;
		m_fDepthSize[0] = m_fDepthSize[1] = 512;
	}

	//! デストラクタ
	~ShadowMap(){}


	/*!
	 * シャドウマップ用FBOの初期化
	 * @param[in] w,h  シャドウマップの解像度
	 */
	void InitShadow(int w, int h)
	{
		glewInit();
		if(!glewIsSupported("GL_ARB_depth_texture "
							"GL_ARB_shadow "
							)){
			cout << "ERROR: Support for necessary OpenGL extensions missing." << endl;
			return;
		}

		m_fDepthSize[0] = w;
		m_fDepthSize[1] = h;
	
		// デプス値テクスチャ
		glActiveTexture(GL_TEXTURE7);
		glGenTextures(1, &m_iTexDepth);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);

		// テクスチャパラメータの設定
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		GLfloat border_color[4] = {1, 1, 1, 1};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	
		// テクスチャ領域の確保(GL_DEPTH_COMPONENTを用いる)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_fDepthSize[0], m_fDepthSize[1], 0, 
					 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	

		// FBO作成
		glGenFramebuffersEXT(1, &m_iFBODepth);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFBODepth);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	
		// デプスマップテクスチャをFBOに接続
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_iTexDepth, 0);
	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// GLSL
		m_glslShading = CreateGLSL(shadow_vs, shadow_fs, "shadow");
	}


	/*!
	 * 影付きでシーン描画
	 * @param[in] light 光源
	 * @param[in] fpDraw 描画関数のポインタ
	 */
	void RenderSceneWithShadow(Frustum &light, void (*fpDraw)(void*), void* func_obj, bool self_shading = false)
	{
		float light_proj[16], camera_proj[16];
		float light_modelview[16], camera_modelview[16];

		//
		// 現在の視点行列，光源行列を取得 or 作成
		//
		glMatrixMode(GL_PROJECTION);

		// 視点プロジェクション行列の取得
		glGetFloatv(GL_PROJECTION_MATRIX, camera_proj);
		glPushMatrix();	// 今のプロジェクション行列を退避させておく

		// 光源プロジェクション行列の生成と取得
		glLoadIdentity();
		//glm::mat4 mp = glm::perspective(light.fov, light.w/light.h, light.near, light.far);
		//glMultMatrixf(glm::value_ptr(mp));
		gluPerspective(light.fov, (double)light.w/(double)light.h, light.near, light.far);
		glGetFloatv(GL_PROJECTION_MATRIX, light_proj);
		
		glMatrixMode(GL_MODELVIEW);

		// 視点モデルビュー行列の取得
		glGetFloatv(GL_MODELVIEW_MATRIX, camera_modelview);
		glPushMatrix();	// 今のモデルビュー行列を退避させておく

		// 光源モデルビュー行列の生成と取得
		glLoadIdentity();
		//glm::mat4 mv = glm::lookAt(light.origin, light.lookat, light.up);
		//glMultMatrixf(glm::value_ptr(mv));
		gluLookAt(light.origin[0], light.origin[1], light.origin[2], 
				  light.lookat[0], light.lookat[1], light.lookat[2], 
				  light.up[0], light.up[1], light.up[2]);
		glGetFloatv(GL_MODELVIEW_MATRIX, light_modelview);

		// 今のビューポート情報を後で戻すために確保
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);


		//
		// 光源からレンダリングしてシャドウマップを生成
		//
		glBindFramebuffer(GL_FRAMEBUFFER, m_iFBODepth);	// FBOにレンダリング

		// カラー，デプスバッファのクリア
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClearDepth(1.0f);

		// ビューポートをシャドウマップの大きさに変更
		glViewport(0, 0, m_fDepthSize[0], m_fDepthSize[1]);
	
		// 光源を視点として設定
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(light_proj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(light_modelview);
	
		// デプス値以外の色のレンダリングを無効にする
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
	
		glPolygonOffset(1.1f, 40.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);

		glEnable(GL_TEXTURE_2D);	
	
		glDisable(GL_LIGHTING);
		if(self_shading){
			glDisable(GL_CULL_FACE);
		}
		else{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}

		glUseProgram(0);
		fpDraw(func_obj);

		glDisable(GL_POLYGON_OFFSET_FILL);
	
	
		const float bias[16] = { 0.5, 0.0, 0.0, 0.0, 
								  0.0, 0.5, 0.0, 0.0,
								  0.0, 0.0, 0.5, 0.0,
								  0.5, 0.5, 0.5, 1.0 };
	

	
		// テクスチャモードに移行
		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
	
		glLoadIdentity();
		glLoadMatrixf(bias);
	
		// 光源中心座標となるようにテクスチャ行列を設定
		// テクスチャ変換行列にモデルビュー，プロジェクションを設定
		glMultMatrixf(light_proj);
		glMultMatrixf(light_modelview);

		//// 現在のモデルビューの逆行列をかけておく
		GLfloat camera_modelview_inv[16];
		CalInvMat4x4(camera_modelview, camera_modelview_inv);
		glMultMatrixf(camera_modelview_inv);

		// 回転と平行移動で分けて計算する場合(こっちの方が高速)
		//GLfloat rot[16];
		//rot[0] = camera_modelview[0]; rot[1] = camera_modelview[1]; rot[2]  = camera_modelview[2];  rot[3]  = 0.0f;
		//rot[4] = camera_modelview[4]; rot[5] = camera_modelview[5]; rot[6]  = camera_modelview[6];  rot[7]  = 0.0f;
		//rot[8] = camera_modelview[8]; rot[9] = camera_modelview[9]; rot[10] = camera_modelview[10]; rot[11] = 0.0f;
		//rot[12] = 0.0f;               rot[13] = 0.0f;               rot[14] = 0.0f;                 rot[15] = 1.0f;
		//glMultTransposeMatrixf(rot);
		//glTranslatef(-camera_modelview[12], -camera_modelview[13], -camera_modelview[14]);
	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 無効にした色のレンダリングを有効にする
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 

		// 元のビューポート行列に戻す
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		// 退避させておいた視点行列を元に戻す
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();


		//
		// カメラから見たときのシーン描画
		// 
		glEnable(GL_TEXTURE_2D);

		// デプステクスチャを貼り付け
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// GLSLシェーダをセット
		glUseProgram(m_glslShading.Prog);
		glUniform1i(glGetUniformLocation(m_glslShading.Prog, "tex"), 0);
		glUniform1i(glGetUniformLocation(m_glslShading.Prog, "depth_tex"), 7);
		glUniform1f(glGetUniformLocation(m_glslShading.Prog, "shadow_ambient"), 0.7f);

		fpDraw(func_obj);

		glUseProgram(0);
		glActiveTexture(GL_TEXTURE0);

		glBindTexture(GL_TEXTURE_2D, 0);

	}

	/*!
	 * デプスマップをテクスチャとして表示
	 * @param[in] w,h ウィンドウサイズ
	 */
	void DrawDepthTex(int w, int h)
	{
		glUseProgram(0);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glColor4f(1, 1, 1, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);
		glEnable(GL_TEXTURE_2D);

		glNormal3d(0, 0, -1);
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex3f(0.05*w,     0.05*h, 0);
		glTexCoord2d(1, 0); glVertex3f(0.05*w+100, 0.05*h, 0);
		glTexCoord2d(1, 1); glVertex3f(0.05*w+100, 0.05*h+100, 0);
		glTexCoord2d(0, 1); glVertex3f(0.05*w,     0.05*h+100, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
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
inline ShadowMap::Frustum CalFrustum(float fov_deg, float near_d, float far_d, int w, int h, 
									 glm::vec3 pos, glm::vec3 lookat, glm::vec3 up)
{
	ShadowMap::Frustum f;
	f.near = near_d;
	f.far = far_d;
	f.fov = fov_deg;
	f.w = w;
	f.h = h;
	f.origin = pos;
	f.lookat = lookat;
	f.up = up;
	return f;
}

/*!
 * プロジェクション行列，視点位置の設定
 * @param[in] f 視錘台
 */
inline void SetFrustum(const ShadowMap::Frustum &f)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glm::mat4 mp = glm::perspective(f.fov, (float)f.w/(float)f.h, f.near, f.far);
	glMultMatrixf(glm::value_ptr(mp));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glm::mat4 mv = glm::lookAt(f.origin, f.lookat, f.up);
	glMultMatrixf(glm::value_ptr(mv));
}

/*!
 * 4x4行列の行列式の計算
 *  | m[0]  m[1]  m[2]  m[3]  |
 *  | m[4]  m[5]  m[6]  m[7]  |
 *  | m[8]  m[9]  m[10] m[11] |
 *  | m[12] m[13] m[14] m[15] |
 * @param[in] m 元の行列
 * @return 行列式の値
 */
inline float CalDetMat4x4(const GLfloat m[16])
{
	return m[0]*m[5]*m[10]*m[15]+m[0]*m[6]*m[11]*m[13]+m[0]*m[7]*m[9]*m[14]
		  +m[1]*m[4]*m[11]*m[14]+m[1]*m[6]*m[8]*m[15]+m[1]*m[7]*m[10]*m[12]
		  +m[2]*m[4]*m[9]*m[15]+m[2]*m[5]*m[11]*m[12]+m[2]*m[7]*m[8]*m[13]
		  +m[3]*m[4]*m[10]*m[13]+m[3]*m[5]*m[8]*m[14]+m[3]*m[6]*m[9]*m[12]
		  -m[0]*m[5]*m[11]*m[14]-m[0]*m[6]*m[9]*m[15]-m[0]*m[7]*m[10]*m[13]
		  -m[1]*m[4]*m[10]*m[15]-m[1]*m[6]*m[11]*m[12]-m[1]*m[7]*m[8]*m[14]
		  -m[2]*m[4]*m[11]*m[13]-m[2]*m[5]*m[8]*m[15]-m[2]*m[7]*m[9]*m[12]
		  -m[3]*m[4]*m[9]*m[14]-m[3]*m[5]*m[10]*m[12]-m[3]*m[6]*m[8]*m[13];
}
 
/*!
 * 4x4行列の行列式の計算
 *  | m[0]  m[1]  m[2]  m[3]  |
 *  | m[4]  m[5]  m[6]  m[7]  |
 *  | m[8]  m[9]  m[10] m[11] |
 *  | m[12] m[13] m[14] m[15] |
 * @param[in] m 元の行列
 * @param[out] invm 逆行列
 * @return 逆行列の存在
 */
inline bool CalInvMat4x4(const GLfloat m[16], GLfloat invm[16])
{
	GLfloat det = CalDetMat4x4(m);
	if(fabs(det) < 1e-6){
		return false;
	}
	else{
		GLfloat inv_det = 1.0/det;
 
		invm[0]  = inv_det*(m[5]*m[10]*m[15]+m[6]*m[11]*m[13]+m[7]*m[9]*m[14]-m[5]*m[11]*m[14]-m[6]*m[9]*m[15]-m[7]*m[10]*m[13]);
		invm[1]  = inv_det*(m[1]*m[11]*m[14]+m[2]*m[9]*m[15]+m[3]*m[10]*m[13]-m[1]*m[10]*m[15]-m[2]*m[11]*m[13]-m[3]*m[9]*m[14]);
		invm[2]  = inv_det*(m[1]*m[6]*m[15]+m[2]*m[7]*m[13]+m[3]*m[5]*m[14]-m[1]*m[7]*m[14]-m[2]*m[5]*m[15]-m[3]*m[6]*m[13]);
		invm[3]  = inv_det*(m[1]*m[7]*m[10]+m[2]*m[5]*m[11]+m[3]*m[6]*m[9]-m[1]*m[6]*m[11]-m[2]*m[7]*m[9]-m[3]*m[5]*m[10]);
 
		invm[4]  = inv_det*(m[4]*m[11]*m[14]+m[6]*m[8]*m[15]+m[7]*m[10]*m[12]-m[4]*m[10]*m[15]-m[6]*m[11]*m[12]-m[7]*m[8]*m[14]);
		invm[5]  = inv_det*(m[0]*m[10]*m[15]+m[2]*m[11]*m[12]+m[3]*m[8]*m[14]-m[0]*m[11]*m[14]-m[2]*m[8]*m[15]-m[3]*m[10]*m[12]);
		invm[6]  = inv_det*(m[0]*m[7]*m[14]+m[2]*m[4]*m[15]+m[3]*m[6]*m[12]-m[0]*m[6]*m[15]-m[2]*m[7]*m[12]-m[3]*m[4]*m[14]);
		invm[7]  = inv_det*(m[0]*m[6]*m[11]+m[2]*m[7]*m[8]+m[3]*m[4]*m[10]-m[0]*m[7]*m[10]-m[2]*m[4]*m[11]-m[3]*m[6]*m[8]);
 
		invm[8]  = inv_det*(m[4]*m[9]*m[15]+m[5]*m[11]*m[12]+m[7]*m[8]*m[13]-m[4]*m[11]*m[13]-m[5]*m[8]*m[15]-m[7]*m[9]*m[12]);
		invm[9]  = inv_det*(m[0]*m[11]*m[13]+m[1]*m[8]*m[15]+m[3]*m[9]*m[12]-m[0]*m[9]*m[15]-m[1]*m[11]*m[12]-m[3]*m[8]*m[13]);
		invm[10] = inv_det*(m[0]*m[5]*m[15]+m[1]*m[7]*m[12]+m[3]*m[4]*m[13]-m[0]*m[7]*m[13]-m[1]*m[4]*m[15]-m[3]*m[5]*m[12]);
		invm[11] = inv_det*(m[0]*m[7]*m[9]+m[1]*m[4]*m[11]+m[3]*m[5]*m[8]-m[0]*m[5]*m[11]-m[1]*m[7]*m[8]-m[3]*m[4]*m[9]);
 
		invm[12] = inv_det*(m[4]*m[10]*m[13]+m[5]*m[8]*m[14]+m[6]*m[9]*m[12]-m[4]*m[9]*m[14]-m[5]*m[10]*m[12]-m[6]*m[8]*m[13]);
		invm[13] = inv_det*(m[0]*m[9]*m[14]+m[1]*m[10]*m[12]+m[2]*m[8]*m[13]-m[0]*m[10]*m[13]-m[1]*m[8]*m[14]-m[2]*m[9]*m[12]);
		invm[14] = inv_det*(m[0]*m[6]*m[13]+m[1]*m[4]*m[14]+m[2]*m[5]*m[12]-m[0]*m[5]*m[14]-m[1]*m[6]*m[12]-m[2]*m[4]*m[13]);
		invm[15] = inv_det*(m[0]*m[5]*m[10]+m[1]*m[6]*m[8]+m[2]*m[4]*m[9]-m[0]*m[6]*m[9]-m[1]*m[4]*m[10]-m[2]*m[5]*m[8]);
 
		return true;
	}
}


#endif // #ifdef _RX_SHADOWMAP_H_