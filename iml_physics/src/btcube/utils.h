/*!
  @file rx_utils.h

  @brief 各種便利な関数や定数など

  @author Makoto Fujisawa
  @date 2020-07
*/
// FILE --rx_utils.h--

#ifndef _RX_UTILS_H_
#define _RX_UTILS_H_

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
// C標準
#include <ctime>
#include <cmath>
//#include <cctype>
#include <cstdio>
#include <cassert>

#ifdef WIN32
#include <direct.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

// STL
#include <vector>
#include <string>
#include <map>
#include <bitset>
#include <algorithm>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext.hpp"	// for glm::to_string()

// 画像読み書き
#include "rx_bitmap.h"

// OpenGLのシェーダ
#include "rx_shaders.h"

// 設定ファイル
#include "rx_atom_ini.h"

// トラックボール＆テクスチャ
#include "rx_trackball.h"
#include "rx_texture.h"

// メッシュファイル読み込み関連
#include "rx_obj.h"

// シャドウマップ
//#include "rx_shadow_gl.h"	
#include "rx_shadow_glsl.h"

// Bullet
#include <btBulletDynamicsCommon.h>  

#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/Gimpact/btGImpactShape.h>

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBody.h>


using namespace std;


//-----------------------------------------------------------------------------
// 定義
//-----------------------------------------------------------------------------
const double RX_PI = 3.14159265358979323846;	// 円周率
const double RX_FEQ_EPS = 1.0e-8;				// 許容誤差
const double RX_FEQ_INF = 1.0e10;				// 大きい数の初期値設定用
const double RX_DEGREES_TO_RADIANS = 0.0174532925199432957692369076848;	//! degree -> radian の変換係数(pi/180.0)
const double RX_RADIANS_TO_DEGREES = 57.295779513082320876798154814114;	//! radian -> degree の変換係数(180.0/pi)

//! 許容誤差を含めた等値判定
template<class T>
inline bool RX_FEQ(const T &a, const T &b){ return (fabs(a-b) < RX_FEQ_EPS); }

//! 値のクランプ(クランプした値を返す)
template<class T>
inline T RX_CLAMP(const T &x, const T &a, const T &b){ return ((x < a) ? a : (x > b) ? b : x); }

//! 1次元線型補間
template<class T>
inline T RX_LERP(const T &a, const T &b, const T &t){ return a + t*(b-a); }

//! 乱数
inline double RX_RAND(const double &_min, const double &_max){ return (_max-_min)*(double(rand())/(1.0+RAND_MAX))+_min; }

//! 最小値判定(3値)
template<class T> 
inline T RX_MIN3(const T &a, const T &b, const T &c){ return ( (a < b) ? ( (a < c) ? a : c) : ( (b < c) ? b : c)); }

//! スワップ
template<class T>
inline void RX_SWAP(T &a, T &b){ T c; c = a; a = b; b = c; }

//! btVector3用のストリーム入出力の定義
inline std::ostream &operator<<(std::ostream &out, const btVector3 &a)
{
	return out << "(" << a[0] << ", " << a[1] << ", " << a[2] << ")";
}
inline std::istream &operator>>(std::istream &in, btVector3& a)
{
	return in >> a[0] >> a[1] >> a[2];
}

/*!
 * GLFWのエラーコールバック関数
 * @param[in] error エラーコード
 * @param[in] description エラー内容
 */
static inline void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


//-----------------------------------------------------------------------------
// プリミティブのポリゴンメッシュデータ生成
//-----------------------------------------------------------------------------
/*!
* 立方体形状のポリゴンメッシュを生成
* - 立方体の中心は原点(0,0,0)
* - 四角形メッシュバージョン
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] len 辺の長さ
*/
static inline int MakeCube(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	float len)
{
	nvrts = 0; ntris = 0;
	vrts.clear(); nrms.clear(); tris.clear();

	// 頂点座標
	float sl = 0.5f*len;	// 辺の長さの半分
	vrts.push_back(glm::vec3(-sl, -sl, -sl));
	vrts.push_back(glm::vec3( sl, -sl, -sl));
	vrts.push_back(glm::vec3( sl, -sl,  sl));
	vrts.push_back(glm::vec3(-sl, -sl,  sl));
	vrts.push_back(glm::vec3(-sl,  sl, -sl));
	vrts.push_back(glm::vec3( sl,  sl, -sl));
	vrts.push_back(glm::vec3( sl,  sl,  sl));
	vrts.push_back(glm::vec3(-sl,  sl,  sl));
	nvrts = static_cast<int>(vrts.size());

	// 頂点法線
	for(const glm::vec3 &v : vrts){
		nrms.push_back(glm::normalize(v));
	}

	// 四角形メッシュ作成
	int f[6][4] = { {0,1,2,3}, {4,7,6,5}, {0,3,7,4}, {1,5,6,2}, {0,4,5,1}, {3,2,6,7} };
	for(int j = 0; j < 6; ++j){
		for(int i = 0; i < 4; ++i){
			tris.push_back(f[j][i]);
		}
	}
	ntris = static_cast<int>(tris.size()/4);

	return 1;
}

/*!
* 立方体形状のポリゴンメッシュを生成
* - 立方体の中心は原点(0,0,0)
* - 面毎に頂点を分けて面法線を適用するバージョン
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] len 辺の長さ
*/
static inline int MakeCubeWithFaceNormal(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	float len)
{
	nvrts = 0; ntris = 0;
	vrts.clear(); nrms.clear(); tris.clear();

	nvrts = 6*4;
	ntris = 6;
	int f[6][4] = { {0,1,2,3}, {4,7,6,5}, {0,3,7,4}, {1,5,6,2}, {0,4,5,1}, {3,2,6,7} };

	// 頂点座標
	float sl = 0.5f*len;	// 辺の長さの半分
	vector<glm::vec3> v0(8);
	v0[0] = glm::vec3(-sl, -sl, -sl);
	v0[1] = glm::vec3( sl, -sl, -sl);
	v0[2] = glm::vec3( sl, -sl,  sl);
	v0[3] = glm::vec3(-sl, -sl,  sl);
	v0[4] = glm::vec3(-sl,  sl, -sl);
	v0[5] = glm::vec3( sl,  sl, -sl);
	v0[6] = glm::vec3( sl,  sl,  sl);
	v0[7] = glm::vec3(-sl,  sl,  sl);
	
	// 頂点法線
	vector<glm::vec3> n0(6);
	n0[0] = glm::vec3(0, -1, 0);
	n0[1] = glm::vec3(0,  1, 0);
	n0[2] = glm::vec3(-1, 0, 0);
	n0[3] = glm::vec3( 1, 0, 0);
	n0[4] = glm::vec3(0, 0, -1);
	n0[5] = glm::vec3(0, 0,  1);

	vrts.resize(nvrts);
	nrms.resize(nvrts);
	tris.resize(ntris*4);
	for(int j = 0; j < 6; ++j){
		for(int i = 0; i < 4; ++i){
			int idx = i+4*j;
			vrts[idx] = (v0[f[j][i]]);
			nrms[idx] = n0[j];
			tris[idx] = idx;
		}
	}

	return 1;
}

/*!
* y軸上方向を法線とした平面描画のためのポリゴンメッシュを生成
* - 平面は原点(0,0,0)を通り，法線が(0,1,0)
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] len 辺の長さ
*/
static inline int MakePlaneY(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	vector<glm::vec2> &texcoords, float len)
{
	nvrts = 0; ntris = 0;
	vrts.clear(); nrms.clear(); tris.clear();

	nvrts = 4;
	ntris = 1;

	// 頂点座標
	float sl = 0.5f*len;	// 辺の長さの半分
	vrts.resize(4);
	vrts[0] = glm::vec3(-sl, 0, -sl);
	vrts[1] = glm::vec3(-sl, 0,  sl);
	vrts[2] = glm::vec3( sl, 0,  sl);
	vrts[3] = glm::vec3( sl, 0, -sl);
	
	// 頂点法線
	nrms.resize(4);
	nrms[0] = glm::vec3(0, 1, 0);
	nrms[1] = glm::vec3(0, 1, 0);
	nrms[2] = glm::vec3(0, 1, 0);
	nrms[3] = glm::vec3(0, 1, 0);

	// テクスチャ座標
	texcoords.resize(4);
	texcoords[0] = glm::vec2(0, 0);
	texcoords[1] = glm::vec2(0, 1);
	texcoords[2] = glm::vec2(1, 1);
	texcoords[3] = glm::vec2(1, 0);

	// メッシュ頂点インデックス
	tris.resize(4);
	for(int i = 0; i < 4; ++i) tris[i] = i;

	return 1;
}

/*!
* 球体形状のポリゴンメッシュを生成
* - 球の中心は原点(0,0,0)
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] rad 球の半径
* @param[in] slices,stacks 緯度方向(360度)と傾度方向(180度)のポリゴン分割数
*/
static inline int MakeSphere(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	float rad, int slices = 16, int stacks = 8)
{
	const float pi = glm::pi<float>();
	nvrts = 0;
	ntris = 0;
	vrts.clear();
	nrms.clear();
	tris.clear();

	for(int j = 0; j <= stacks; ++j){
		float t = float(j)/float(stacks);
		float y = rad*cos(pi*t);
		float rj = rad*sin(pi*t);	// 高さyでの球の断面円半径
		for(int i = 0; i <= slices; ++i){
			float s = float(i)/float(slices);
			float x = rj*sin(2*pi*s);
			float z = rj*cos(2*pi*s);
			vrts.push_back(glm::vec3(x, y, z));
			nrms.push_back(glm::normalize(glm::vec3(x, y, z)));
		}
	}
	nvrts = static_cast<int>(vrts.size());
	// メッシュ作成
	int nx = slices+1;
	for(int j = 0; j < stacks; ++j){
		for(int i = 0; i < slices; ++i){
			tris.push_back((i)+(j)*nx);
			tris.push_back((i+1)+(j+1)*nx);
			tris.push_back((i+1)+(j)*nx);

			tris.push_back((i)+(j)*nx);
			tris.push_back((i)+(j+1)*nx);
			tris.push_back((i+1)+(j+1)*nx);
		}
	}
	ntris = static_cast<int>(tris.size()/3);

	return 1;
}

/*!
* 円筒形状のポリゴンメッシュを生成してVAOとして登録
* - 円筒の中心は原点(0,0,0)
* - 円筒の軸方向はz軸方向(0,0,1) - gluCylinderに合わせている
* - 法線を別にするために側面と端面の頂点を別にしている
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] rad,len 円筒の半径と長さ
* @param[in] slices 円筒の円に沿ったポリゴン分割数
*/
static inline int MakeCylinder(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	float rad1, float rad2, float len, int slices = 16, bool disk = true)
{
	const float pi = glm::pi<float>();

	// 側面用頂点
	for(int i = 0; i <= slices; ++i){
		float t = float(i)/float(slices);
		float x1 = rad1*cos(2*pi*t);
		float y1 = rad1*sin(2*pi*t);
		float x2 = rad2*cos(2*pi*t);
		float y2 = rad2*sin(2*pi*t);
		vrts.push_back(glm::vec3(x1, y1, -0.5*len));
		vrts.push_back(glm::vec3(x2, y2, 0.5*len));
		glm::vec3 n = glm::vec3(x1, y1, 0.0);
		if(rad1 < 1e-6) n = glm::vec3(x2, y2, 0.0);
		if(rad1 > rad2){
			float l = glm::length(n);
			n += l*(rad1-rad2)/len*glm::vec3(0, 0, 1);
		}
		else if(rad1 < rad2){
			float l = glm::length(n);
			n += l*(rad2-rad1)/len*glm::vec3(0, 0, -1);
		}
		if(glm::length2(n) > 1e-6) n = glm::normalize(n);
		nrms.push_back(n); nrms.push_back(n);
	}
	// 端面用頂点(座標値は↑と同じだが法線が異なる)
	int voffset = vrts.size();
	for(int i = 0; i <= slices; ++i){
		vrts.push_back(vrts[2*i]);
		nrms.push_back(glm::vec3(0.0, 0.0, -1.0));
		vrts.push_back(vrts[2*i+1]);
		nrms.push_back(glm::vec3(0.0, 0.0, 1.0));
	}

	// メッシュ作成
	for(int i = 0; i < 2*slices; i += 2){
		tris.push_back(i);
		tris.push_back((i+2 >= 2*slices ? 0 : i+2));
		tris.push_back(i+1);

		tris.push_back(i+1);
		tris.push_back((i+2 >= 2*slices ? 0 : i+2));
		tris.push_back((i+2 >= 2*slices ? 1 : i+3));
	}

	// 両端面にポリゴンを貼る
	if(disk){
		// 端面中心頂点
		vrts.push_back(glm::vec3(0, 0, -0.5*len));
		nrms.push_back(glm::normalize(glm::vec3(0, 0, -1)));
		int c1 = vrts.size()-1;
		vrts.push_back(glm::vec3(0, 0,  0.5*len));
		nrms.push_back(glm::normalize(glm::vec3(0, 0,  1)));
		int c2 = vrts.size()-1;

		for(int i = 0; i < slices; ++i){
			tris.push_back(c1);
			tris.push_back(2*i+voffset);
			tris.push_back((i == slices-1 ? 0 : 2*i+2)+voffset);

			tris.push_back(c2);
			tris.push_back(2*i+1+voffset);
			tris.push_back((i == slices-1 ? 1 : 2*i+3)+voffset);
		}
	}

	nvrts = static_cast<int>(vrts.size());
	ntris = static_cast<int>(tris.size()/3);

	return 1;
}


/*!
* カプセル形状(円筒の両端が半球)のポリゴンメッシュを生成してVAOとして登録
* - 形状の中心は原点(0,0,0)
* - カプセル形状の軸方向はz軸方向(0,0,1) - gluCylinderに合わせている
* @param[out] nvrts,ntris 生成したメッシュの頂点数とポリゴン数を返す
* @param[in] rad,len 円筒の半径と長さ
* @param[in] slices 円筒の円に沿ったポリゴン分割数
*/
static inline int MakeCapsule(int &nvrts, vector<glm::vec3> &vrts, vector<glm::vec3> &nrms, int &ntris, vector<int> &tris,
	float rad, float len, int slices = 16, int stacks = 8)
{
	const float pi = glm::pi<float>();
	int offset = 0;

	// 球体の中心断面(赤道部分)に沿った頂点を2重にして，
	// その部分を円筒長さ分z方向に伸ばすことでカプセル形状を生成
	for(int j = 0; j <= stacks; ++j){
		float t = float(j)/float(stacks);
		float z = rad*cos(pi*t);
		float rj = rad*sin(pi*t);	// 高さyでの球の断面円半径
		float zlen = (j < stacks/2 ? 0.5*len : -0.5*len);	// z方向のオフセット量

		if(j == stacks/2){	// 中心断面(赤道部分)に頂点を追加
			for(int i = 0; i <= slices; ++i){
				float s = float(i)/float(slices);
				float x = rj*sin(2*pi*s);
				float y = rj*cos(2*pi*s);
				vrts.push_back(glm::vec3(x, y, z-zlen));
				nrms.push_back(glm::normalize(glm::vec3(x, y, z)));
			}
		}
		for(int i = 0; i <= slices; ++i){
			float s = float(i)/float(slices);
			float x = rj*sin(2*pi*s);
			float y = rj*cos(2*pi*s);
			vrts.push_back(glm::vec3(x, y, z+zlen));
			nrms.push_back(glm::normalize(glm::vec3(x, y, z)));
		}
	}

	// メッシュ作成
	int nx = slices+1;
	for(int j = 0; j < stacks+1; ++j){
		for(int i = 0; i < slices; ++i){
			tris.push_back((i)+(j)*nx+offset);
			tris.push_back((i+1)+(j)*nx+offset);
			tris.push_back((i+1)+(j+1)*nx+offset);

			tris.push_back((i)+(j)*nx+offset);
			tris.push_back((i+1)+(j+1)*nx+offset);
			tris.push_back((i)+(j+1)*nx+offset);
		}
	}

	nvrts = static_cast<int>(vrts.size());
	ntris = static_cast<int>(tris.size()/3);

	return 1;
}




//-----------------------------------------------------------------------------
// VBO関連
//-----------------------------------------------------------------------------
struct MeshVBO
{
	GLuint vrts, vrts_attrib;
	GLuint tcds, tcds_attrib;
	GLuint nrms, nrms_attrib;
	GLuint cols, cols_attrib;
	GLuint tris;
	int nvrts, ntris;
	MeshVBO() :vrts(0),tcds(0),nrms(0),cols(0),tris(0),nvrts(0),ntris(0),vrts_attrib(0),tcds_attrib(1),nrms_attrib(2),cols_attrib(3) {}
	~MeshVBO()
	{
		glDeleteBuffers(1, &vrts);
		glDeleteBuffers(1, &tcds);
		glDeleteBuffers(1, &nrms);
		glDeleteBuffers(1, &cols);
		glDeleteBuffers(1, &tris);
	}
};

/*!
 * VBOの作成
 * - 最低限頂点情報があれば良い．必要のない引数は0を入れておく．
 * @param[in] vrts,nvrts 頂点座標配列と頂点数
 * @param[in] dim 次元(2or3)
 * @param[in] tris,ntris ポリゴンを構成する頂点インデックス列を格納した配列とポリゴン数
 * @param[in] nelem 1ポリゴンの頂点数(GL_TRIANGLESなら3,GL_QUADSなら4)
 * @param[in] nrms,nnrms 各頂点の法線情報配列と法線数(=頂点数)
 * @param[in] cols,ncols 各頂点の色情報配列と色情報数(=頂点数)
 * @param[in] tcds,ntcds 各頂点のテクスチャ座標情報配列とテクスチャ座標数(=頂点数)
 * @return VBO
 */
static inline int CreateVBO(MeshVBO &vbo, 
	const GLfloat* vrts, GLuint nvrts, int dim = 3,
	const int* tris = 0, GLuint ntris = 0, GLuint nelem = 3, 
	const GLfloat* nrms = 0, GLuint nnrms = 0,
	const GLfloat* cols = 0, GLuint ncols = 0,
	const GLfloat* tcds = 0, GLuint ntcds = 0)
{
	if(!nvrts) return 0;

	// VBO:頂点座標
	glGenBuffers(1, &vbo.vrts);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * dim * nvrts, vrts, GL_STATIC_DRAW);
	vbo.nvrts = nvrts;

	// VBO:頂点テクスチャ座標
	if (tcds) {
		glGenBuffers(1, &vbo.tcds);
		glBindBuffer(GL_ARRAY_BUFFER, vbo.tcds);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * ntcds, tcds, GL_STATIC_DRAW);
	}

	// VBO:頂点法線
	if (nrms) {
		glGenBuffers(1, &vbo.nrms);
		glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * dim * nnrms, nrms, GL_STATIC_DRAW);
	}

	// VBO:頂点描画色
	if (cols) {
		glGenBuffers(1, &vbo.cols);
		glBindBuffer(GL_ARRAY_BUFFER, vbo.cols);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * ncols, cols, GL_STATIC_DRAW);
	}

	// VBO:三角形ポリゴンインデックス
	if (tris) {
		glGenBuffers(1, &vbo.tris);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * ntris * nelem, tris, GL_STATIC_DRAW);
		vbo.ntris = ntris;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return 1;
}
/*!
 * VBOの破棄
 */
static inline void DeleteVBO(MeshVBO vbo)
{
	if(vbo.vrts == 0) return;
	glDeleteBuffers(1, &vbo.vrts); vbo.vrts = 0;

	if(vbo.tcds != 0){ glDeleteBuffers(1, &vbo.tcds); vbo.tcds = 0; }
	if(vbo.nrms != 0){ glDeleteBuffers(1, &vbo.nrms); vbo.nrms = 0; }
	if(vbo.cols != 0){ glDeleteBuffers(1, &vbo.tcds); vbo.cols = 0; }
	if(vbo.tris != 0){ glDeleteBuffers(1, &vbo.tris); vbo.tris = 0; }

}

/*!
 * VBOによるメッシュ描画
 * @param[in] mode ポリゴン描画モード(GL_TRIANGLES,GL_QUADS,GL_POLYGONなど)
 * @param[in] nelem 1要素の頂点数(GL_TRIANGLESなら3,GL_QUADSなら4)
 * @param[in] vao VAOオブジェクト
 * @param[in] draw 描画フラグ
 * @param[in] nvrts,ntris 頂点数,三角形ポリゴン数
 * @param[in] col[3] 頂点,エッジ,面の色
 */
static inline void DrawMeshVBO(GLenum mode, int nelem, MeshVBO vbo, int draw, glm::vec3 col[3])
{
	if (!vbo.vrts) return;

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);


	if (draw & 0x04) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_LIGHTING);

		glColor3fv(glm::value_ptr(col[2]));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
		glDrawElements(mode, vbo.ntris * nelem, GL_UNSIGNED_INT, 0);
	}
	if (draw & 0x02) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LIGHTING);

		glLineWidth(1.0);
		glColor3fv(glm::value_ptr(col[1]));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
		glDrawElements(mode, vbo.ntris * nelem, GL_UNSIGNED_INT, 0);
	}
	if (draw & 0x01) {
		glDisable(GL_LIGHTING);

		glPointSize(8.0);
		glColor3fv(glm::value_ptr(col[0]));
		glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
		glDrawArrays(GL_POINTS, 0, vbo.nvrts);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}




//-----------------------------------------------------------------------------
// VAO関連
//-----------------------------------------------------------------------------
/*!
 * 頂点配列オブジェクトの作成
 * - 最低限頂点情報があれば良い．必要のない引数は0を入れておく．
 * @param[in] vrts,nvrts 頂点座標配列と頂点数
 * @param[in] dim 次元(2or3)
 * @param[in] tris,ntris 三角形ポリゴンを構成する頂点インデックス列を格納した配列と三角形ポリゴン数
 * @param[in] nrms,nnrms 各頂点の法線情報配列と法線数(=頂点数)
 * @param[in] cols,ncols 各頂点の色情報配列と色情報数(=頂点数)
 * @param[in] tcds,ntcds 各頂点のテクスチャ座標情報配列とテクスチャ座標数(=頂点数)
 * @return VAOオブジェクト
 */
static inline GLuint CreateVAO(
	const GLfloat *vrts, GLuint nvrts, int dim = 3,
	const int     *tris = 0, GLuint ntris = 0,
	const GLfloat *nrms = 0, GLuint nnrms = 0,
	const GLfloat *cols = 0, GLuint ncols = 0,
	const GLfloat *tcds = 0, GLuint ntcds = 0)
{
	// VAOの生成
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// VBO:頂点座標
	GLuint vvbo;
	glGenBuffers(1, &vvbo);
	glBindBuffer(GL_ARRAY_BUFFER, vvbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*dim*nvrts, vrts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, 0);

	// VBO:頂点テクスチャ座標
	if(tcds){
		GLuint tvbo;
		glGenBuffers(1, &tvbo);
		glBindBuffer(GL_ARRAY_BUFFER, tvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*2*ntcds, tcds, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// VBO:頂点法線
	if(nrms){
		GLuint nvbo;
		glGenBuffers(1, &nvbo);
		glBindBuffer(GL_ARRAY_BUFFER, nvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*dim*nnrms, nrms, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, dim, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// VBO:頂点描画色
	if(cols){
		GLuint cvbo;
		glGenBuffers(1, &cvbo);
		glBindBuffer(GL_ARRAY_BUFFER, cvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*ncols, cols, GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// VBO:三角形ポリゴンインデックス
	if(tris){
		GLuint pvbo;
		glGenBuffers(1, &pvbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pvbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*ntris*3, tris, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return vao;
}


/*!
 * VAOに関連づけられたデータの更新
 * @param[in] vao VAOオブジェクト
 * @param[in] index glVertexAttribPointerで関連づけられたインデックス
 * @param[in] data データ配列
 * @param[in] n データ数
 * @return VBOオブジェクト
 */
static inline GLuint UpdateDataVAO(GLuint vao, GLuint index, const GLfloat *data, GLuint n)
{
	glBindVertexArray(vao);
	GLuint vbo;
	glGetVertexAttribIuiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*n, data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return vbo;
}

/*!
 * VAOの破棄
 *  - リンクされているVBOなどは破棄されないようなのでそれらは個別に行う必要あり
 * @param[in] vao VAOオブジェクト
 */
static inline void DeleteVAO(GLuint vao)
{
	glDeleteVertexArrays(1, &vao);
}

/*!
 * VAOによるメッシュ描画
 * @param[in] vao VAOオブジェクト
 * @param[in] draw 描画フラグ
 * @param[in] nvrts,ntris 頂点数,三角形ポリゴン数
 * @param[in] col[3] 頂点,エッジ,面の色
 */
static inline void DrawMeshVAO(GLuint vao, int draw, int nvrts, int ntris, glm::vec3 col[3])
{
	if(!vao) return;

	glBindVertexArray(vao);
	if(draw & 0x04){
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_LIGHTING);
		
		glColor3fv(glm::value_ptr(col[2]));
		glDrawElements(GL_TRIANGLES, ntris*3, GL_UNSIGNED_INT, 0);
	}
	if(draw & 0x02){
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LIGHTING);

		glLineWidth(1.0);
		glColor3fv(glm::value_ptr(col[1]));
		glDrawElements(GL_TRIANGLES, ntris*3, GL_UNSIGNED_INT, 0);
	}
	if(draw & 0x01){
		glDisable(GL_LIGHTING);

		glPointSize(8.0);
		glColor3fv(glm::value_ptr(col[0]));
		glDrawArrays(GL_POINTS, 0, nvrts);
	}
	glBindVertexArray(0);
}





/*!
 * VAOによるプリミティブ描画
 * @param[in] obj 頂点数,ポリゴン数情報を含むVAO
 * @param[in] draw 描画フラグ
 */
inline static void DrawPrimitive(const GLuint vao, const int nvrts, const int ntris, int draw)
{
	// エッジ描画における"stitching"をなくすためのオフセットの設定
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.0);

	// 図形の描画
	glDisable(GL_LIGHTING);
	glBindVertexArray(vao);
	if(draw & 0x01){
		glColor3d(1.0, 0.3, 0.3);
		glPointSize(5.0);
		glDrawArrays(GL_POINTS, 0, nvrts);
	}
	if(draw & 0x02){
		glColor3d(0.5, 0.9, 0.9);
		glLineWidth(4.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, ntris*3, GL_UNSIGNED_INT, 0);
	}
	if(draw & 0x04){
		glEnable(GL_LIGHTING);
		//glDisable(GL_CULL_FACE);
		glEnable(GL_AUTO_NORMAL);
		glEnable(GL_NORMALIZE);
		glColor3d(0.1, 0.5, 1.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, ntris*3, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
}





//-----------------------------------------------------------------------------
// glm関係の関数
//-----------------------------------------------------------------------------
inline int IDX4(int row, int col){ return (row | (col<<2)); }

/*!
 * 1次元配列に格納された4x4行列とglm::vec3ベクトルの掛け算
 *  | m[0]  m[1]  m[2]  m[3]  |
 *  | m[4]  m[5]  m[6]  m[7]  |
 *  | m[8]  m[9]  m[10] m[11] |
 *  | m[12] m[13] m[14] m[15] |
 * @param[in] m 元の行列
 * @param[in] v ベクトル
 * @return 積の結果のベクトル(m*v)
 */
inline glm::vec3 MulMat4Vec3(GLfloat* m, const glm::vec3& v)
{
	glm::vec3 d(0.0);
	d[0] = (v[0]*m[IDX4(0, 0)]+v[1]*m[IDX4(0, 1)]+v[2]*m[IDX4(0, 2)]);
	d[1] = (v[0]*m[IDX4(1, 0)]+v[1]*m[IDX4(1, 1)]+v[2]*m[IDX4(1, 2)]);
	d[2] = (v[0]*m[IDX4(2, 0)]+v[1]*m[IDX4(2, 1)]+v[2]*m[IDX4(2, 2)]);
	return d;
}



//-----------------------------------------------------------------------------
// 文字列処理
//-----------------------------------------------------------------------------
/*!
 * 様々な型のstringへの変換(stringstreamを使用)
 * @param[in] x 入力
 * @return string型に変換したもの
 */
template<typename T>
inline std::string RX_TO_STRING(const T &x)
{
	std::stringstream ss;
	ss << x;
	return ss.str();
}

//! string型に<<オペレータを設定
template<typename T>
inline std::string &operator<<(std::string &cb, const T &a)
{
	cb += RX_TO_STRING(a);
	return cb;
}

/*!
 * "\n"が含まれるstringを複数のstringに分解する
 * @param[in] org 元の文字列
 * @param[in] div 分解後の文字列配列
 */
static inline void divideString(const string& org, vector<string>& div)
{
	size_t pos0 = 0, pos1 = 0;
	while(pos1 != string::npos){
		pos1 = org.find("\n", pos0);

		div.push_back(org.substr(pos0, pos1-pos0));

		pos0 = pos1+1;
	}
}

/*!
 * "(x, y, z)"の形式の文字列からglm::vec3型へ変換
 *  - (x)となっていたら(x, x, x)とする．
 * @param[in] s 文字列
 * @param[out] v 値
 * @return 要素記述数
 */
inline int StringToVec3(const string &s, glm::vec3 &v)
{
	int vcount = 0;
	size_t pos;
	v = glm::vec3(0.0);
	if((pos = s.find('(')) != string::npos){
		while(pos != string::npos && vcount < 3){
			size_t pos1 = pos;
			if((pos1 = s.find(',', pos+1)) != string::npos){
				v[vcount] = atof(s.substr(pos+1, (pos1-(pos+1))).c_str());
				vcount++;
				pos = pos1;
			}
			else if((pos1 = s.find(')', pos+1)) != string::npos){
				v[vcount] = atof(s.substr(pos+1, (pos1-(pos+1))).c_str());
				vcount++;
				break;
			}
			else{
				break;
			}
		}
	}
	if(vcount < 3){
		for(int i = vcount; i < 3; ++i){
			v[i] = v[vcount-1];
		}
	}

	return vcount;
}

/*!
 * 整数値の下一桁を返す
 * @param[in] x 整数値
 * @return xの下一桁
 */
inline int LowOneDigit(const int &x)
{
	int x1 = (x < 0) ? -x : x;
	float a = 10;

	//INT_MAX = 2147483647
	for(int i = 10; i >= 1; i--){
		a = pow(10.0, (float)i);
		while(x1 > a){
			x1 -= (int)a;
		}
	}

	return x1;
}

/*!
 * 0付きの数字を生成
 * @param[in] n 数字
 * @param[in] d 桁数
 * @return 0付きの数字(string)
 */
inline string GenZeroNo(int n, const int &d)
{
	string zero_no = "";
	int dn = d-1;
	if(n > 0){
		dn = (int)(log10((float)n))+1;
	}
	else if(n == 0){
		dn = 1;
	}
	else{
		n = 0;
		dn = 1;
	}

	for(int i = 0; i < d-dn; ++i){
		zero_no += "0";
	}

	zero_no += RX_TO_STRING(n);

	return zero_no;
}

/*!
 * 秒数を hh:mm:ss の形式に変換
 * @param[in] sec 秒数
 * @param[in] use_msec ミリ秒まで含める(hh:mm:ss.msec)
 * @return hh:mm:ss形式の文字列
 */
inline string GenTimeString(float sec, bool use_msec = false)
{
	long value = (int)(1000*sec+0.5);	// ミリ秒

	unsigned int h = (unsigned int)(value/3600000);	// 時間
	value -= h*3600000;
	unsigned int m = (unsigned int)(value/60000);		// 分
	value -= m*60000;
	unsigned int s = (unsigned int)(value/1000);		// 秒
	value -= s*1000;
	unsigned int ms = (unsigned int)(value);			// ミリ秒

	stringstream ss;
	if(h > 0) ss << GenZeroNo(h, 2) << ":";
	ss << GenZeroNo(m, 2) << ":";
	ss << GenZeroNo(s, 2);
	if(use_msec) ss << "." << GenZeroNo(ms, 3);

	return ss.str();
}

/*!
 * 時刻を hh:mm:ss の形式に変換
 * @param[in] h,m,s 時,分,秒
 * @return hh:mm:ss形式の文字列
 */
inline string GenTimeString(int h, int m, int s)
{
	stringstream ss;
	if(h > 0) ss << GenZeroNo(h, 2) << ":";
	ss << GenZeroNo(m, 2) << ":";
	ss << GenZeroNo(s, 2);
	return ss.str();
}


//-----------------------------------------------------------------------------
// ファイル・フォルダ処理
//-----------------------------------------------------------------------------
/*!
 * ファイル，フォルダの存在確認
 * @param[in] path ファイル・フォルダパス
 */
inline int ExistFile(const string fn)
{
	FILE *fp;

	if((fp = fopen(fn.c_str(), "r")) == NULL){
		return 0;
	}

	fclose(fp);
	return 1;
}

/*!
 * フォルダ区切りの検索
 * @param[in] str ファイル・フォルダパス
 * @param[out] pos 見つかった位置
 */
inline bool FindPathBound(const string &str, string::size_type &pos)
{
	if((pos = str.find_last_of("/")) == string::npos){
		if((pos = str.find_last_of("\\")) == string::npos){
			return false;
		}
	}

	return true;
}

/*!
 * ファイル名比較関数(拡張子)
 * @param[in] fn 比較したいファイル名
 * @param[in] ext 拡張子
 * @return fnの拡張子がextと同じならtrue
 */
inline bool SearchCompExt(const string &fn, const string &ext)
{
	return (fn.find(ext, 0) != string::npos);
}


/*!
 * ファイル名生成
 * @param head : 基本ファイル名
 * @param ext  : 拡張子
 * @param n    : 連番
 * @param d    : 連番桁数
 * @return 生成したファイル名
 */
inline string CreateFileName(const string &head, const string &ext, int n, const int &d)
{
	string file_name = head;
	int dn = d-1;
	if(n > 0){
		dn = (int)(log10((float)n))+1;
	}
	else if(n == 0){
		dn = 1;
	}
	else{
		n = 0;
		dn = 1;
	}

	for(int i = 0; i < d-dn; ++i){
		file_name += "0";
	}

	file_name += RX_TO_STRING(n);
	if(!ext.empty() && ext[0] != '.') file_name += ".";
	file_name += ext;

	return file_name;
}




/*!
 * パスからファイル名のみ取り出す
 * @param[in] path パス
 * @return ファイル名
 */
inline string GetFileName(const string &path)
{
	size_t pos1;

	pos1 = path.rfind('\\');
	if(pos1 != string::npos){
		return path.substr(pos1+1, path.size()-pos1-1);
	}

	pos1 = path.rfind('/');
	if(pos1 != string::npos){
		return path.substr(pos1+1, path.size()-pos1-1);
	}

	return path;
}

/*!
 * パスから拡張子を小文字にして取り出す
 * @param[in] path ファイルパス
 * @return (小文字化した)拡張子
 */
inline string GetExtension(const string &path)
{
	string ext;
	size_t pos1 = path.rfind('.');
	if(pos1 != string::npos){
		ext = path.substr(pos1+1, path.size()-pos1);
		string::iterator itr = ext.begin();
		while(itr != ext.end()){
			*itr = tolower(*itr);
			itr++;
		}
		itr = ext.end()-1;
		while(itr != ext.begin()){	// パスの最後に\0やスペースがあったときの対策
			if(*itr == 0 || *itr == 32){
				ext.erase(itr--);
			}
			else{
				itr--;
			}
		}
	}

	return ext;
}

/*!
 * ファイルストリームを開く
 * @param[out] file ファイルストリーム
 * @param[in] path  ファイルパス
 * @param[in] rw    入出力フラグ (1:読込, 2:書込, 4:追記)
 * @return ファイルオープン成功:1, 失敗:0
 */
static inline int OpenFileStream(fstream &file, const string &path, int rw = 1)
{
	file.open(path.c_str(), (rw & 0x01 ? ios::in : 0)|(rw & 0x02 ? ios::out : 0)|(rw & 0x04 ? ios::app : 0));
	if(!file || !file.is_open() || file.bad() || file.fail()){
		return 0;
	}
	return 1;
}

/*!
 * ディレクトリ作成(多階層対応) - Windows only
 * @param[in] dir 作成ディレクトリパス
 * @return 成功で1,失敗で0 (ディレクトリがすでにある場合も1を返す)
 */
static int MkDir(string dir)
{
#ifdef WIN32
	if(_mkdir(dir.c_str()) != 0){
		char cur_dir[512];
		_getcwd(cur_dir, 512);	// カレントフォルダを確保しておく
		if(_chdir(dir.c_str()) == 0){	// chdirでフォルダ存在チェック
			cout << "MkDir : " << dir << " is already exist." << endl;
			_chdir(cur_dir);	// カレントフォルダを元に戻す
			return 1;
		}
		else{
			size_t pos = dir.find_last_of("\\/");
			if(pos != string::npos){	// 多階層の可能性有り
				int parent = MkDir(dir.substr(0, pos+1));	// 親ディレクトリを再帰的に作成
				if(parent){
					if(_mkdir(dir.c_str()) == 0){
						return 1;
					}
					else{
						return 0;
					}
				}
			}
			else{
				return 0;
			}
		}
	}
	return 1;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// テクスチャ関連
//-----------------------------------------------------------------------------
/*!
 * 画像読込み -> OpenGLテクスチャ登録
 * @param[in] fn ファイル名
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
static int loadTexture(const string& fn, GLuint& tex_name, bool mipmap, bool compress)
{
	// 画像読み込み
	int w, h, c, wstep;
	unsigned char* pimg;
	pimg = ReadBitmapFile(fn, w, h, c, wstep, false, true);
	if(!pimg){
		return 0;
	}

	GLuint iformat, format;

	// 画像フォーマット
	format = GL_RGBA;
	if(c == 1){
		format = GL_LUMINANCE;
	}
	else if(c == 3){
		format = GL_RGB;
	}

	// OpenGL内部の格納フォーマット
	if(compress){
		iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		if(c == 1){
			iformat = GL_COMPRESSED_LUMINANCE_ARB;
		}
		else if(c == 3){
			iformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
	}
	else{
		iformat = GL_RGBA;
		if(c == 1){
			iformat = GL_LUMINANCE;
		}
		else if(c == 3){
			iformat = GL_RGB;
		}
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// テクスチャ作成
	if(tex_name == 0){
		glGenTextures(1, &tex_name);

		glBindTexture(GL_TEXTURE_2D, tex_name);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if(mipmap){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 6);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, GL_UNSIGNED_BYTE, pimg);

		if(mipmap){
			glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	}
	else{
		glBindTexture(GL_TEXTURE_2D, tex_name);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, pimg);
		glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, GL_UNSIGNED_BYTE, pimg);

		if(mipmap){
			glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pimg;

	return 1;
}

/*!
 * 画像読込み -> OpenGLテクスチャ登録
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
static int makeCheckerBoardTexture(GLuint& tex_name, const int size = 8, bool mipmap = false, bool compress = false)
{
	// ピクセルデータ生成
	unsigned char* pimg = new unsigned char[size*size*4];
	if(!pimg) return 0;

	int cw = size/4;

	// チェッカーボードパターン生成
	for(int i = 0; i < size; ++i){
		for(int j = 0; j < size; ++j){
			if((i+j%cw)%cw == 0){
				// dark
				pimg[(i*size+j)*4 + 0] = static_cast<GLubyte>(170);
				pimg[(i*size+j)*4 + 1] = static_cast<GLubyte>(200);
				pimg[(i*size+j)*4 + 2] = static_cast<GLubyte>(170);
				pimg[(i*size+j)*4 + 3] = static_cast<GLubyte>(255);
			}
			else {
				// light
				pimg[(i*size+j)*4 + 0] = static_cast<GLubyte>(205);
				pimg[(i*size+j)*4 + 1] = static_cast<GLubyte>(245);
				pimg[(i*size+j)*4 + 2] = static_cast<GLubyte>(205);
				pimg[(i*size+j)*4 + 3] = static_cast<GLubyte>(255);
			}
		}
	}


	GLuint iformat, format;

	// 画像フォーマット
	format = GL_RGBA;

	// OpenGL内部の格納フォーマット
	if(compress){
		iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	}
	else{
		iformat = GL_RGBA;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// テクスチャ作成
	if(tex_name == 0){
		glGenTextures(1, &tex_name);
		glBindTexture(GL_TEXTURE_2D, tex_name);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));

		if(mipmap){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 6);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, pimg);

		if(mipmap){
			glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	}
	else{
		glBindTexture(GL_TEXTURE_2D, tex_name);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, format, GL_UNSIGNED_BYTE, pimg);
		glTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, pimg);

		if(mipmap){
			glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pimg;

	return 1;
}

/*!
 * GLSLでテクスチャ色とシェーディング色のブレンドを行う場合に，
 * テクスチャが貼られていないオブジェクトでテクスチャ色が得られないことを防ぐための単色テクスチャ
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
static int makePlainTexture(GLuint& tex_name, glm::vec3 col)
{
	int size = 2;
	// ピクセルデータ生成
	unsigned char* pimg = new unsigned char[size*size*4];
	if(!pimg) return 0;

	for(int i = 0; i < size; ++i){
		for(int j = 0; j < size; ++j){
			pimg[(i*size+j)*4 + 0] = static_cast<GLubyte>(col[0]*255);
			pimg[(i*size+j)*4 + 1] = static_cast<GLubyte>(col[1]*255);
			pimg[(i*size+j)*4 + 2] = static_cast<GLubyte>(col[2]*255);
			pimg[(i*size+j)*4 + 3] = static_cast<GLubyte>(0);
		}
	}

	GLuint iformat, format;

	// 画像フォーマット
	format = GL_RGBA;
	iformat = GL_RGBA;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// テクスチャ作成
	if(tex_name == 0){
		glGenTextures(1, &tex_name);
		glBindTexture(GL_TEXTURE_2D, tex_name);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, pimg);
	}
	else{
		glBindTexture(GL_TEXTURE_2D, tex_name);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, format, GL_UNSIGNED_BYTE, pimg);
		glTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, pimg);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pimg;

	return 1;
}
static void BindPlainTexture(glm::vec3 col)
{
	glActiveTexture(GL_TEXTURE0);
	static GLuint tex = 0;
	if(tex == 0){
		makePlainTexture(tex, col);
	}
	glBindTexture(GL_TEXTURE_2D, tex);
}

/*!
 * フレームバッファのRGB情報を一時的なバッファに確保
 * @retval true 保存成功
 * @retval false 保存失敗
 */
static inline bool saveFrameBuffer(string fn, int w, int h)
{
	string ext = GetExtension(fn);
	if(ext != "bmp") fn += ".bmp";

	int c = 3;
	int wstep = (((w+1)*c)/4)*4;
	vector<unsigned char> imm_buf(wstep*h);

	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, &imm_buf[0]);
	WriteBitmapFile(fn, &imm_buf[0], w, h, c, RX_BMP_WINDOWS_V3, wstep, false, true);
	return true;
}




//-----------------------------------------------------------------------------
// 各種オブジェクト描画関数
//-----------------------------------------------------------------------------
/*!
 * xyz軸描画(x軸:赤,y軸:緑,z軸:青)
 * @param[in] len 軸の長さ
 */
static inline int DrawAxis(float len, float line_width)
{
	glLineWidth((GLfloat)line_width);

	// x軸
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(len, 0.0, 0.0);
	glEnd();

	// y軸
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, len, 0.0);
	glEnd();

	// z軸
	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, len);
	glEnd();

	return 1;
}


/*!
 * 直方体のワイヤーフレーム描画
 * @param[in] min 最小座標値
 * @param[in] max 最大座標値
 * @param[in] color 描画色
 */
static inline void DrawWireCuboid(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, float line_width = 1.0)
{
	glLineWidth((GLfloat)line_width);

	glPushMatrix();
	glColor3d(color[0], color[1], color[2]);

	glBegin(GL_LINES);

	// x軸平行
	glVertex3f(min[0], min[1], min[2]);	glVertex3f(max[0], min[1], min[2]);
	glVertex3f(min[0], min[1], max[2]); glVertex3f(max[0], min[1], max[2]);
	glVertex3f(min[0], max[1], min[2]);	glVertex3f(max[0], max[1], min[2]);
	glVertex3f(min[0], max[1], max[2]);	glVertex3f(max[0], max[1], max[2]);

	// z軸平行
	glVertex3f(min[0], min[1], min[2]);	glVertex3f(min[0], min[1], max[2]);
	glVertex3f(min[0], max[1], min[2]);	glVertex3f(min[0], max[1], max[2]);
	glVertex3f(max[0], min[1], min[2]);	glVertex3f(max[0], min[1], max[2]);
	glVertex3f(max[0], max[1], min[2]);	glVertex3f(max[0], max[1], max[2]);

	// z軸平行
	glVertex3f(min[0], min[1], min[2]);	glVertex3f(min[0], max[1], min[2]);
	glVertex3f(min[0], min[1], max[2]);	glVertex3f(min[0], max[1], max[2]);
	glVertex3f(max[0], min[1], min[2]);	glVertex3f(max[0], max[1], min[2]);
	glVertex3f(max[0], min[1], max[2]);	glVertex3f(max[0], max[1], max[2]);

	glEnd();

	glPopMatrix();
}



/*!
 * 原点中心の円のワイヤーフレーム描画
 * @param rad 円の半径
 * @param n 分割数
 */
static inline void DrawWireCircle(const float& rad, const int& n)
{
	float t = 0.0;
	float dt = 2.0*RX_PI/(float)n;

	glBegin(GL_LINE_LOOP);
	do{
		glVertex3f(rad*cos(t), rad*sin(t), 0.0);
		t += dt;
	} while(t < 2.0*RX_PI);
	glEnd();
}

/*!
 * 原点中心の円のワイヤーフレーム描画(XZ平面)
 * @param rad 円の半径
 * @param n 分割数
 */
static inline void DrawWireCircleXZ(const float& rad, const int& n)
{
	float t = 0.0;
	float dt = 2.0*RX_PI/(float)n;

	glBegin(GL_LINE_LOOP);
	do{
		glVertex3f(rad*cos(t), 0.0, rad*sin(t));
		t += dt;
	} while(t < 2.0*RX_PI);
	glEnd();
}

/*!
 * 球のワイヤーフレーム描画
 * @param cen 球の中心
 * @param rad 球の半径
 * @param col 描画色
 */
static inline void DrawWireSphere(const glm::vec3& cen, const float& rad, const glm::vec3& col)
{
	glDisable(GL_LIGHTING);
	glPushMatrix();
	glTranslatef(cen[0], cen[1], cen[2]);
	glRotatef(90, 1.0, 0.0, 0.0);
	glColor3f(col[0], col[1], col[2]);

	// 緯度(x-y平面に平行)
	float z, dz;
	dz = 2.0*rad/8.0f;
	z = -(rad-dz);
	do{
		glPushMatrix();
		glTranslatef(0.0, 0.0, z);
		DrawWireCircle(sqrt(rad*rad-z*z), 32);
		glPopMatrix();
		z += dz;
	} while(z < rad);

	// 経度(z軸まわりに回転)
	float t, dt;
	t = 0.0f;
	dt = 180.0/8.0;
	do{
		glPushMatrix();
		glRotatef(t, 0.0, 0.0, 1.0);
		DrawWireCircleXZ(rad, 32);
		glPopMatrix();

		t += dt;
	} while(t < 180);

	//glutWireSphere(rad, 10, 5);
	glPopMatrix();
}

/*!
* 原点中心の立方体形状描画
* - 立方体の中心は原点(0,0,0)
* - 四角形メッシュバージョン
* @param[in] len 辺の長さ
*/
static inline void DrawCube(float len)
{
	// 頂点座標
	float sl = 0.5f*len;	// 辺の長さの半分
	vector<glm::vec3> vrts(6);
	vrts[0] = glm::vec3(-sl, -sl, -sl);
	vrts[1] = glm::vec3( sl, -sl, -sl);
	vrts[2] = glm::vec3( sl, -sl,  sl);
	vrts[3] = glm::vec3(-sl, -sl,  sl);
	vrts[4] = glm::vec3(-sl,  sl, -sl);
	vrts[5] = glm::vec3( sl,  sl, -sl);
	vrts[6] = glm::vec3( sl,  sl,  sl);
	vrts[7] = glm::vec3(-sl,  sl,  sl);

	// 四角形メッシュ描画
	int f[6][4] = { {0,1,2,3}, {4,7,6,5}, {0,3,7,4}, {1,5,6,2}, {0,4,5,1}, {3,2,6,7} };
	for(int j = 0; j < 6; ++j){
		glBegin(GL_QUADS);
		for(int i = 0; i < 4; ++i){
			glVertex3fv(glm::value_ptr(vrts[f[j][i]]));
		}
		glEnd();
	}
}


/*!
* 原点中心の立方体形状描画
* - 立方体の中心は原点(0,0,0)
* - 辺の長さは1で固定 (glScaleで調整する)
* - VBO使用バージョン
*/
static inline void DrawCubeVBO(void)
{
	static MeshVBO vbo;
	if(vbo.vrts == 0){
		// ポリゴンデータ作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<int> tris;
		MakeCubeWithFaceNormal(nvrts, vrts, nrms, ntris, tris, 1.0);

		// VBOの作成
		CreateVBO(vbo, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 4, (GLfloat*)&nrms[0], nvrts);
	}
	glShadeModel(GL_SMOOTH);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
	glDrawElements(GL_QUADS, vbo.ntris*4, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
/*!
* 原点中心の球形状描画
* - 中心は原点(0,0,0)
* - 半径は0.5で固定 (glScaleで調整する)
* - VBO使用バージョン
*/
static inline void DrawSphereVBO(void)
{
	static MeshVBO vbo;
	if(vbo.vrts == 0){
		// ポリゴンデータ作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<int> tris;
		MakeSphere(nvrts, vrts, nrms, ntris, tris, 0.5, 32, 16);

		// VBOの作成
		CreateVBO(vbo, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 3, (GLfloat*)&nrms[0], nvrts);
	}
	glShadeModel(GL_SMOOTH);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
	glDrawElements(GL_TRIANGLES, vbo.ntris*3, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!
* 原点中心の円筒形状描画
* - 中心は原点(0,0,0)
* - 半径0.5(直径1)/長さ1で固定 (glScaleで調整する)
* - VBO使用バージョン
*/
static inline void DrawCylinderVBO(void)
{
	static MeshVBO vbo;
	if(vbo.vrts == 0){
		// ポリゴンデータ作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<int> tris;
		MakeCylinder(nvrts, vrts, nrms, ntris, tris, 0.5, 0.5, 1.0, 16, true);

		// VBOの作成
		CreateVBO(vbo, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 3, (GLfloat*)&nrms[0], nvrts);
	}
	glShadeModel(GL_SMOOTH);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
	glDrawElements(GL_TRIANGLES, vbo.ntris*3, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!
* 原点中心のカプセル形状描画
* - 中心は原点(0,0,0)
* - 円筒の両端面に半球が付いた形状
* - VBO使用バージョン
*/
static inline void DrawCapsuleVBO(float rad, float len)
{
	static MeshVBO vbo_c, vbo_s;
	if(vbo_c.vrts == 0){
		// 円筒部分ポリゴンデータ/VBO作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<int> tris;
		MakeCylinder(nvrts, vrts, nrms, ntris, tris, 0.5, 0.5, 1.0, 16, 8);
		CreateVBO(vbo_c, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 3, (GLfloat*)&nrms[0], nvrts);

		// 球部分ポリゴンデータ/VBO作成
		vrts.clear(), nrms.clear(); tris.clear();
		MakeSphere(nvrts, vrts, nrms, ntris, tris, 0.5, 16, 8);
		CreateVBO(vbo_s, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 3, (GLfloat*)&nrms[0], nvrts);
	}
	glShadeModel(GL_SMOOTH);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// 円筒部分
	glPushMatrix();
	glScalef(2*rad, 2*rad, len-2*rad);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_c.vrts);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_c.nrms);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_c.tris);
	glDrawElements(GL_TRIANGLES, vbo_c.ntris*3, GL_UNSIGNED_INT, 0);
	glPopMatrix();

	// 端面球1(z-)
	glPushMatrix();
	glTranslatef(0, 0, -0.5*len+rad);
	glScalef(2*rad, 2*rad, 2*rad);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_s.vrts);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_s.nrms);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_s.tris);
	glDrawElements(GL_TRIANGLES, vbo_s.ntris*3, GL_UNSIGNED_INT, 0);
	glPopMatrix();

	// 端面球2(z+)
	glPushMatrix();
	glTranslatef(0, 0, 0.5*len-rad);
	glScalef(2*rad, 2*rad, 2*rad);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_s.vrts);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_s.nrms);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_s.tris);
	glDrawElements(GL_TRIANGLES, vbo_s.ntris*3, GL_UNSIGNED_INT, 0);
	glPopMatrix();



	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!
* 原点中心の円錐形状描画
* - 中心は原点(0,0,0)
* - 半径0.5(直径1)/長さ1で固定 (glScaleで調整する)
* - 基本的にCylinderと同じ，片方の端面の半径が0の円筒と考える
* - VBO使用バージョン
*/
static inline void DrawConeVBO(void)
{
	static MeshVBO vbo;
	if(vbo.vrts == 0){
		// ポリゴンデータ作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<int> tris;
		MakeCylinder(nvrts, vrts, nrms, ntris, tris, 0.0, 0.5, 1.0, 16, true);

		// VBOの作成
		CreateVBO(vbo, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 3, (GLfloat*)&nrms[0], nvrts);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
	glDrawElements(GL_TRIANGLES, vbo.ntris*3, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!
* y方向を法線とする平面ポリゴン描画
* @param[in] y,s 床の高さと水平方向の長さ
* @param[in] use_tex テクスチャON/OFF
*/
static void DrawPlaneVBO(double y = 0.0, double s = 20.0, bool use_tex = true)
{
	static MeshVBO vbo;
	if(vbo.vrts == 0){
		// ポリゴンデータ作成
		int nvrts, ntris;
		vector<glm::vec3> vrts, nrms;
		vector<glm::vec2> texcoords;
		vector<int> tris;
		MakePlaneY(nvrts, vrts, nrms, ntris, tris, texcoords, s);

		// VBOの作成
		CreateVBO(vbo, (GLfloat*)&vrts[0], nvrts, 3, &tris[0], ntris, 4, (GLfloat*)&nrms[0], nvrts, 0, 0, (GLfloat*)&texcoords[0], nvrts);
	}

	static GLuint texFloor = 0;				//!< 床のテクスチャ
	if(use_tex && texFloor == 0){
		// 床テクスチャ読み込み
		glActiveTexture(GL_TEXTURE0);
		//loadTexture("floortile.bmp", texFloor, true, false);
		makeCheckerBoardTexture(texFloor, 8, true, false);
		glBindTexture(GL_TEXTURE_2D, texFloor);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	}

	if(use_tex){
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texFloor);
	}

	glPushMatrix();

	glTranslatef(0, y, 0);
	//glScalef(s, 1, s);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.vrts);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.nrms);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.tcds);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.tris);
	glDrawElements(GL_QUADS, vbo.ntris*4, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glPopMatrix();

	if(use_tex){
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

/*!
* y方向を法線とする平面ポリゴン描画
* @param[in] y,s 床の高さと水平方向の長さ
* @param[in] use_tex テクスチャON/OFF
*/
static void DrawStaticPlane(double y = 0.0, double s = 20.0, bool use_tex = true)
{
	static GLuint texFloor = 0;				//!< 床のテクスチャ
	if(use_tex && texFloor == 0){
		// 床テクスチャ読み込み
		glActiveTexture(GL_TEXTURE0);
		//loadTexture("floortile.bmp", texFloor, true, false);
		makeCheckerBoardTexture(texFloor, 8, true, false);
		glBindTexture(GL_TEXTURE_2D, texFloor);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	}

	if(use_tex){
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texFloor);
	}

	// 床用ポリゴン描画
	glNormal3d(0.0, 1.0, 0.0);
	glBegin(GL_QUADS);
	{
		glTexCoord2d(0.0, 0.0);	glVertex3d(-s, y, -s);
		glTexCoord2d(0.0, 1.0);	glVertex3d(-s, y, s);
		glTexCoord2d(1.0, 1.0);	glVertex3d(s, y, s);
		glTexCoord2d(1.0, 0.0);	glVertex3d(s, y, -s);
	}
	glEnd();

	if(use_tex){
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


/*!
 * Bulletの三角形メッシュ描画時に必要なコールバック関数定義用のクラス
 */
class TriangleDrawCallback : public btTriangleCallback
{
public:
	TriangleDrawCallback(){}
	virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex)
	{
		btVector3 n = ((triangle[0]-triangle[1]).cross(triangle[0]-triangle[2])).normalize();
		glNormal3f(n[0], n[1], n[2]);

		glBegin(GL_TRIANGLES);
		glVertex3d(triangle[0][0], triangle[0][1], triangle[0][2]);
		glVertex3d(triangle[1][0], triangle[1][1], triangle[1][2]);
		glVertex3d(triangle[2][0], triangle[2][1], triangle[2][2]);
		glEnd();
	}
};


/*!
* Bulletの衝突形状を描画
* @param[in] shape 衝突形状
* @param[in] world_min,world_max Bulletワールドの大きさ(ポリゴン描画時に必要)
*/
static void DrawBulletShape(const btCollisionShape *shape, btVector3 &world_min, btVector3 &world_max)
{
	int shapetype = shape->getShapeType();

	glPushMatrix();
	bool use_tex = false;

#ifdef USE_GLSL_SHADOW
	use_tex = true;
	BindPlainTexture(glm::vec3(0.0));
#endif

	// 形状の種類ごとに描画
	if(shapetype == STATIC_PLANE_PROXYTYPE){
		// 平面
		const btStaticPlaneShape* plane = static_cast<const btStaticPlaneShape*>(shape);
		DrawPlaneVBO(plane->getPlaneConstant(), 40.0, use_tex);
	}
	else if(shapetype == BOX_SHAPE_PROXYTYPE){
		// ボックス形状
		const btBoxShape* box = static_cast<const btBoxShape*>(shape);
		btVector3 half_extent = box->getHalfExtentsWithMargin();
		if(box->getUserIndex() == 99){
			DrawPlaneVBO(half_extent[1], 40.0, use_tex);
		}
		else{
			glScaled(2*half_extent[0], 2*half_extent[1], 2*half_extent[2]);
			DrawCubeVBO();
		}
	}
	else if(shapetype == SPHERE_SHAPE_PROXYTYPE){
		// 球形状
		const btSphereShape* sphere = static_cast<const btSphereShape*>(shape);
		double rad = sphere->getRadius();
		glScalef(2*rad, 2*rad, 2*rad);
		DrawSphereVBO();
	}
	else if(shapetype == CYLINDER_SHAPE_PROXYTYPE){
		// 円筒形状
		const btCylinderShape* cylinder = static_cast<const btCylinderShape*>(shape);
		double rad = cylinder->getRadius();
		int up_axis = cylinder->getUpAxis();
		double len = cylinder->getHalfExtentsWithMargin()[up_axis]*2;
		if(up_axis == 0){ // x軸方向
			glRotatef(90.0f, 0, 1, 0);
		}
		else if(up_axis == 1){ // y軸方向
			glRotatef(90.0f, 1, 0, 0);
		}
		glScalef(2*rad, 2*rad, len);
		DrawCylinderVBO();
	}
	else if(shapetype == CAPSULE_SHAPE_PROXYTYPE){
		// カプセル形状
		const btCapsuleShape* capsule = static_cast<const btCapsuleShape*>(shape);
		double rad = capsule->getRadius();
		int up_axis = capsule->getUpAxis();
		double len = capsule->getHalfHeight()*2;
		if(up_axis == 0){ // x軸方向
			glRotatef(90.0f, 0, 1, 0);
		}
		else if(up_axis == 1){ // y軸方向
			glRotatef(90.0f, 1, 0, 0);
		}
		DrawCapsuleVBO(rad, len);
	}
	else if(shapetype == CONE_SHAPE_PROXYTYPE){
		// 円錐形状
		const btConeShape* cone = static_cast<const btConeShape*>(shape);
		double rad = cone->getRadius();
		int up_axis = cone->getConeUpIndex();
		double len = cone->getHeight();
		if(up_axis == 0){ // x軸方向
			glRotatef(90.0f, 0, 1, 0);
		}
		else if(up_axis == 1){ // y軸方向
			glRotatef(90.0f, 1, 0, 0);
		}
		glScalef(2*rad, 2*rad, len);
		DrawConeVBO();
	}
	else if(shapetype == TRIANGLE_MESH_SHAPE_PROXYTYPE){
		// 三角形メッシュ
		const btBvhTriangleMeshShape* mesh = static_cast<const btBvhTriangleMeshShape*>(shape);
		TriangleDrawCallback draw_callback;
		mesh->processAllTriangles(&draw_callback, world_min, world_max);
	}
	else if(shapetype == GIMPACT_SHAPE_PROXYTYPE){
		// 三角形メッシュ(GIMPACT)
		const btGImpactMeshShape* mesh = static_cast<const btGImpactMeshShape*>(shape);
		TriangleDrawCallback draw_callback;
		mesh->processAllTriangles(&draw_callback, world_min, world_max);
	}
	else if(shapetype == COMPOUND_SHAPE_PROXYTYPE){
		// 複合形状
		const btCompoundShape* compound = static_cast<const btCompoundShape*>(shape);
		int num_child = compound->getNumChildShapes();
		btScalar mc[16];
		for(int j = 0; j < num_child; ++j){
			// Compound Shapeを構成するchild shapeを再帰的にDrawBulletShapeに渡す
			compound->getChildTransform(j).getOpenGLMatrix(mc);
			glPushMatrix();
#ifdef BT_USE_DOUBLE_PRECISION
			glMultMatrixd(mc);
#else
			glMultMatrixf(mc);
#endif
			DrawBulletShape(compound->getChildShape(j), world_min, world_max);
			glPopMatrix();
		}
	}
	else{
		DrawSphereVBO();
	}

	glPopMatrix();
}


static inline void DrawBulletSoftBody(btSoftBody* sbody)
{
#ifdef USE_GLSL_SHADOW
	BindPlainTexture(glm::vec3(0.0));
#endif

	if(sbody->m_faces.size() == 0){
		glBegin(GL_LINE_STRIP);
		for(int i = 0; i < sbody->m_nodes.size(); ++i){
			const btSoftBody::Node &node = sbody->m_nodes[i];
			btVector3 p = node.m_x;
			glVertex3f(p.x(), p.y(), p.z());
		}
		glEnd();
		return;
	}

	for(int i = 0; i < sbody->m_faces.size(); ++i){
		const btSoftBody::Face &face = sbody->m_faces[i];

		glBegin(GL_TRIANGLES);
		for(int j = 0; j < 3; ++j){
			const btSoftBody::Node* node = face.m_n[j];
			btVector3 n = node->m_n;
			btVector3 p = node->m_x;
			glNormal3d(n.x(), n.y(), n.z());
			glVertex3d(p.x(), p.y(), p.z());
		}
		glEnd();
	}
}


#endif // #ifndef _UTILS_H_