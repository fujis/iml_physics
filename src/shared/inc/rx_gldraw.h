/*!
  @file rx_gldraw.h

  @brief OpenGLによる描画関数

  @author Makoto Fujisawa
  @date 2020-07
*/
// FILE --rx_gldraw.h--

#ifndef _RX_GLDRAW_H_
#define _RX_GLDRAW_H_


//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
#include <cmath>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>


// OpenGLのシェーダ
#include "rx_shaders.h"

// ユーティリティ
#include "rx_utility.h"

// 画像読み書き
#include "rx_bitmap.h"




using namespace std;



//-----------------------------------------------------------------------------
// OpenGL関数のラッパー
//-----------------------------------------------------------------------------
/*!
* double版glMaterial
* @param[in] face 材質を指定する面(GL_FRONT,GL_BACK,GL_FRONT_AND_BACK)
* @param[in] pname 指定する材質の種類(GL_AMBIENT,GL_DIFFUSE,GL_SPECULAR,GL_EMISSION,GL_SHININESS,GL_AMBIENT_AND_DIFFUSE,GL_COLOR_INDEXES)
* @param[in] params 設定する材質の値
*/
inline void glMaterialdv(GLenum face, GLenum pname, const GLdouble* params)
{
	GLfloat col[4];
	col[0] = (GLfloat)params[0];
	col[1] = (GLfloat)params[1];
	col[2] = (GLfloat)params[2];
	col[3] = (GLfloat)params[3];
	glMaterialfv(face, pname, col);
}

/*!
* double版glLight
* @param[in] light 光源番号(GL_LIGHT0 ～ GL_LIGHT7)
* @param[in] pname 指定する光源パラメータの種類(GL_POSITION,GL_DIFFUSE,GL_AMBIENT,GL_SPECULAR,GL_LINEAR_ATTENUATION,GL_SPOT_DIRECTION,GL_SPOT_CUTOFF,GL_SPOT_EXPONENT)
* @param[in] params 設定する値
*/
inline void glLightdv(GLenum light, GLenum pname, const GLdouble* params)
{
	GLfloat fparams[4];
	fparams[0] = (GLfloat)fparams[0];
	fparams[1] = (GLfloat)fparams[1];
	fparams[2] = (GLfloat)fparams[2];
	fparams[3] = (GLfloat)fparams[3];
	glLightfv(light, pname, fparams);
}
inline void glLightdv3(GLenum light, GLenum pname, const GLdouble* params)
{
	GLfloat fparams[4];
	fparams[0] = (GLfloat)fparams[0];
	fparams[1] = (GLfloat)fparams[1];
	fparams[2] = (GLfloat)fparams[2];
	fparams[3] = 1.0f;
	glLightfv(light, pname, fparams);
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
	} else if(c == 3){
		format = GL_RGB;
	}

	// OpenGL内部の格納フォーマット
	if(compress){
		iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		if(c == 1){
			iformat = GL_COMPRESSED_LUMINANCE_ARB;
		} else if(c == 3){
			iformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
	} else{
		iformat = GL_RGBA;
		if(c == 1){
			iformat = GL_LUMINANCE;
		} else if(c == 3){
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
	} else{
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
 * @param[in] fn ファイル名
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
static int makeCheckerBoardTexture(GLuint& tex_name, const int size = 72, bool mipmap = false, bool compress = false)
{
	// ピクセルデータ生成
	unsigned char* pimg = new unsigned char[size*size*4];
	if(!pimg) return 0;
	for(int i = 0; i < size; ++i){
		for(int j = 0; j < size; ++j){
			if((i+j%2)%2 == 0){
				// dark
				pimg[i*size*4 + j*4 + 0] = (GLubyte)195;
				pimg[i*size*4 + j*4 + 1] = (GLubyte)188;
				pimg[i*size*4 + j*4 + 2] = (GLubyte)207;
				pimg[i*size*4 + j*4 + 3] = (GLubyte)255;
			} else {
				// light
				pimg[i*size*4 + j*4 + 0] = (GLubyte)220;
				pimg[i*size*4 + j*4 + 1] = (GLubyte)213;
				pimg[i*size*4 + j*4 + 2] = (GLubyte)232;
				pimg[i*size*4 + j*4 + 3] = (GLubyte)255;
			}
		}
	}


	GLuint iformat, format;

	// 画像フォーマット
	format = GL_RGBA;

	// OpenGL内部の格納フォーマット
	if(compress){
		iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	} else{
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
	} else{
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
// VAO関連
//-----------------------------------------------------------------------------
/*!
 * 頂点配列オブジェクトの作成
 * @param[in] vrts 頂点座標配列
 * @param[in] nvrts 頂点数
 * @param[in] dim 次元(2or3)
 * @return VAOオブジェクト
 */
static inline GLuint CreateVAO(const GLfloat *vrts,     GLuint nvrts, int dim = 3,
							   const GLuint  *tris = 0, GLuint ntris = 0, 
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
	glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// VBO:三角形ポリゴンインデックス
	if(tris){
		GLuint pvbo;
		glGenBuffers(1, &pvbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pvbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*ntris*3, tris, GL_STATIC_DRAW);
	}

	// VBO:頂点法線
	if(nrms){
		GLuint nvbo;
		glGenBuffers(1, &nvbo);
		glBindBuffer(GL_ARRAY_BUFFER, nvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*dim*nnrms, nrms, GL_STATIC_DRAW);
		glVertexAttribPointer(2, dim, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}

	// VBO:頂点描画色
	if(cols){
		GLuint cvbo;
		glGenBuffers(1, &cvbo);
		glBindBuffer(GL_ARRAY_BUFFER, cvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*ncols, cols, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
	}

	// VBO:頂点テクスチャ座標
	if(tcds){
		GLuint tvbo;
		glGenBuffers(1, &tvbo);
		glBindBuffer(GL_ARRAY_BUFFER, tvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*2*ntcds, tcds, GL_STATIC_DRAW);
		glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(8);
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


//-----------------------------------------------------------------------------
// VBOによる粒子描画関連
//-----------------------------------------------------------------------------

/*!
 * 粒子速度をGL_LINESで描画
 * @param[in] prts 粒子位置
 * @param[in] vels 粒子速度
 * @param[in] n 粒子数
 * @param[in] offset 描画する粒子の最初のインデックス
 * @param[in] len 線の長さ
 */
static inline void drawParticleVector(double* prts, double* vels, int n, int offset, const double* c0, const double* c1, double len, double w = 1.0)
{
	glDisable(GL_LIGHTING);
	glLineWidth(w);
	glBegin(GL_LINES);
	int k = offset*4;
	for(int i = 0; i < n; ++i){
		glColor3dv(c0);
		glVertex3d(prts[k], prts[k+1], prts[k+2]);
		glColor3dv(c1);
		glVertex3d(prts[k]+len*vels[k], prts[k+1]+len*vels[k+1], prts[k+2]+len*vels[k+2]);
		k += 4;
	}
	glEnd();
}

/*!
 * 粒子をGL_POINTSで描画
 *  - VBOがあれば用いる
 * @param[in] vbo 粒子位置を格納したVBO
 * @param[in] n 粒子数
 * @param[in] color_vbo 粒子の色を格納したVBO
 * @param[in] data 粒子座標(ホスト側メモリ，vboが0の時に使用)
 * @param[in] offset 描画する粒子の最初のインデックス
 */
static inline void drawParticlePoints(unsigned int vbo, int n, unsigned int color_vbo, double* data, int offset = 0, 
									  Vec3 col = Vec3(0.0, 0.0, 1.0), GLfloat point_size = 1.0f)
{
	if(n <= 0) return;
	glColor4d(col[0], col[1], col[2], 1.0);
	glPointSize(point_size);
	if(!vbo){
		glBegin(GL_POINTS);
		int k = offset*4;
		for(int i = 0; i < n; ++i){
			glVertex3d(data[k], data[k+1], data[k+2]);
			k += 4;
		}
		glEnd();
	} else{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(4, GL_DOUBLE, 0, 0);

		if(color_vbo){
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, color_vbo);
			glColorPointer(4, GL_DOUBLE, 0, 0);
			glEnableClientState(GL_COLOR_ARRAY);
		}

		glDrawArrays(GL_POINTS, offset, n);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


/*!
 * 粒子をPointSpriteで描画
 *  - VBOがあれば用いる
 * @param[in] vbo 粒子位置を格納したVBO
 * @param[in] n 粒子数
 * @param[in] color_vbo 粒子の色を格納したVBO
 * @param[in] data 粒子座標(ホスト側メモリ，vboが0の時に使用)
 * @param[in] offset 描画する粒子の最初のインデックス
 * @param[in] pscale,prad PointSpriteのスケールと半径
 * @param[in] czf,czb 断面を描画する際の前方,後方クリッピング位置(z座標値)
 */
static inline void drawPointSprites(unsigned int vbo, int n, unsigned int color_vbo, double* data, int offset, 
									double pscale, double prad, double czf = 1000, double czb = -1000)
{
	static bool init = true;
	static rxGLSL glslPS;			//!< GLSLを使った描画
	if(init){
		// PointSpriteシェーダの初期化(最初の呼び出し時だけ実行)
		//glslPS = CreateGLSL(ps_vs, ps_fs, "pointsprite");
		glslPS = CreateGLSLFromFile("shaders/pointsprite.vp", "shaders/pointsprite.fp", "pointsprite");
		init = false;
	}

	// PointSpriteのための設定
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	// シェーダ変数の設定
	glUseProgram(glslPS.Prog);
	glUniform1f(glGetUniformLocation(glslPS.Prog, "pointScale"), pscale);
	glUniform1f(glGetUniformLocation(glslPS.Prog, "pointRadius"), prad);
	glUniform3f(glGetUniformLocation(glslPS.Prog, "lightDir"), 0.6, 0.6, 0.6);
	glUniform1f(glGetUniformLocation(glslPS.Prog, "zCrossFront"), czf);
	glUniform1f(glGetUniformLocation(glslPS.Prog, "zCrossBack"),  czb);

	// 点描画
	drawParticlePoints(vbo, n, color_vbo, data, offset);

	// 後処理
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);

}


/*!
 * 粒子をglutSolidSphereで描画
 * @param[in] n 粒子数
 * @param[in] offset 描画する粒子の最初のインデックス
 * @param[in] data 粒子座標(ホスト側メモリ)
 * @param[in] attr 粒子属性(ホスト側メモリ)
 * @param[in] rad  描画半径
 * @param[in] col  描画色
 * @param[in] draw_attr どの属性を描画するか
 * @param[in] czf,czb 断面を描画する際の前方,後方クリッピング位置(z座標値)
 */
static inline void drawParticleSpheres(int n, int offset, double* data, int* attr, double rad, Vec3 col = Vec3(0.4, 0.4, 1.0), 
									   int draw_attr = 0, double czf = 1000, double czb = -1000)
{
	glEnable(GL_LIGHTING);
	glColor3f(0.4, 0.4, 1.0);
	int k = offset*4;
	for(int i = 0; i < n; ++i){
		if(data[k+2] < czf && data[k+2] > czb && attr[i] == draw_attr){
			glPushMatrix();
			glTranslatef(data[k], data[k+1], data[k+2]);
			glRotated(90, 1.0, 0.0, 0.0);
			//glutSolidSphere(rad, 20, 10);
			glPopMatrix();
		}
		k += 4;
	}
}


/*!
 * 粒子のカラーバッファ値を計算
 * @param[out] cvbo カラーVBO
 * @param[in] hVal ホストメモリ上の配列
 * @param[in] d 配列のステップ
 * @param[in] n 要素数(配列のサイズはn*d)
 * @param[in] use_max 最大値で値を正規化
 * @param[in] vmax 手動設定最大値
 * @return 値の最大値
 */
static inline double setColorVBO(GLuint& cvbo, double* hVal, int d, int n, bool use_max, double vmax)
{
	double l = 1.0;
	double max_val = 0.0;
	for(int i = 0; i < n; ++i){
		if(hVal[d*i] > max_val) max_val = hVal[d*i];
	}

	l = max_val;
	if(!use_max) l = vmax;

	// 粒子カラーバッファ
	glBindBufferARB(GL_ARRAY_BUFFER, cvbo);
	double* data = (double*)glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//double *ptr = data;
	for(int i = 0; i < n; ++i){
		double value = hVal[d*i]/l;

		double col[3];
		Gradation(col, value, 0.0, 1.0);
		data[4*i+0] = col[0];
		data[4*i+1] = col[1];
		data[4*i+2] = col[2];
		data[4*i+3] = 1.0;
	}
	glUnmapBufferARB(GL_ARRAY_BUFFER);

	return max_val;
}


/*!
 * 粒子のカラーバッファ値を設定(一定値)
 * @param[out] cvbo カラーVBO
 * @param[in] n 要素数
 * @param[in] col 色
 */
static inline int setColorVBO(GLuint& cvbo, int n, Vec3 col)
{
	// カラーバッファに値を設定
	glBindBufferARB(GL_ARRAY_BUFFER, cvbo);
	double* data = (double*)glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	double* ptr = data;
	for(int i = 0; i < n; ++i){
		*ptr++ = col[0];
		*ptr++ = col[1];
		*ptr++ = col[2];
		*ptr++ = 1.0f;
	}
	glUnmapBufferARB(GL_ARRAY_BUFFER);
	return 0;
}

/*!
 * 粒子のカラーバッファ値を設定(グラデーション)
 * @param[out] cvbo カラーVBO
 * @param[in] n 要素数
 */
static inline int setRampColorVBO(GLuint& cvbo, int n)
{
	// カラーバッファに値を設定
	glBindBufferARB(GL_ARRAY_BUFFER, cvbo);
	double* data = (double*)glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	double* ptr = data;
	for(int i = 0; i < n; ++i){
		double t = i/(double)n;
		RX_COLOR_RAMP(t, ptr);
		ptr += 3;
		*ptr++ = 1.0f;
	}
	glUnmapBufferARB(GL_ARRAY_BUFFER);
	return 0;
}




//-----------------------------------------------------------------------------
// 各種オブジェクト描画関数
//-----------------------------------------------------------------------------
/*!
 * 2点からAABBを生成して描画
 * @param[in] A,B 直方体の2辺
 */
static inline void drawAABB(Vec3 A, Vec3 B)
{
	glBegin(GL_QUADS);

	//下
	glNormal3dv(Vec3(0.0, -1.0, 0.0));
	glVertex3d(A[0], A[1], B[2]);
	glVertex3d(A[0], A[1], A[2]);
	glVertex3d(B[0], A[1], A[2]);
	glVertex3d(B[0], A[1], B[2]);

	//左
	glNormal3dv(Vec3(-1.0, 0.0, 0.0));
	glVertex3d(A[0], B[1], A[2]);
	glVertex3d(A[0], A[1], A[2]);
	glVertex3d(A[0], A[1], B[2]);
	glVertex3d(A[0], B[1], B[2]);

	//手前
	glNormal3dv(Vec3(0.0, 0.0, 1.0));
	glVertex3d(A[0], B[1], B[2]);
	glVertex3d(A[0], A[1], B[2]);
	glVertex3d(B[0], A[1], B[2]);
	glVertex3d(B[0], B[1], B[2]);

	//右
	glNormal3dv(Vec3(1.0, 0.0, 0));
	glVertex3d(B[0], B[1], B[2]);
	glVertex3d(B[0], A[1], B[2]);
	glVertex3d(B[0], A[1], A[2]);
	glVertex3d(B[0], B[1], A[2]);

	//奥
	glNormal3dv(Vec3(0.0, -1.0, 0));
	glVertex3d(B[0], B[1], A[2]);
	glVertex3d(B[0], A[1], A[2]);
	glVertex3d(A[0], A[1], A[2]);
	glVertex3d(A[0], B[1], A[2]);

	//上
	glNormal3dv(Vec3(0.0, 1.0, 0));
	glVertex3d(A[0], B[1], A[2]);
	glVertex3d(A[0], B[1], B[2]);
	glVertex3d(B[0], B[1], B[2]);
	glVertex3d(B[0], B[1], A[2]);

	glEnd();

}

/*!
 * カップ形状の直方体(一部の面が空いているAABB)を描画
 * @param[in] minp,maxp 直方体の最小・最大座標
 * @param[in] w 壁面の厚さ
 * @param[in] drawface 描画しない面を指定する(下位ビットから手前(z+),奥(z-),右(x+),左(x-),上(y+),下(y-):0x01,02,04,08,10,20)
 */
inline void drawCubeCup(Vec3 minp, Vec3 maxp, double w, int drawface = 0x3F-0x10)
{
	if(drawface & 0x01) drawAABB(Vec3(minp[0], minp[1], maxp[2]), Vec3(maxp[0], maxp[1], maxp[2]+w));//手前
	if(drawface & 0x02) drawAABB(Vec3(minp[0], minp[1], minp[2]-w), Vec3(maxp[0], maxp[1], minp[2]));//奥

	double front_offset = (drawface & 0x01) ? 0.0 : -0.8*w;
	if(drawface & 0x04) drawAABB(Vec3(maxp[0], minp[1], minp[2]-w), Vec3(maxp[0]+w, maxp[1], maxp[2]+w+front_offset));//右
	if(drawface & 0x08) drawAABB(Vec3(minp[0]-w, minp[1], minp[2]-w), Vec3(minp[0], maxp[1], maxp[2]+w+front_offset));//左

	if(drawface & 0x10) drawAABB(Vec3(minp[0]-w, maxp[1]-w, minp[2]-w), Vec3(maxp[0]+w, maxp[1], maxp[2]+w+front_offset));//上
	if(drawface & 0x20) drawAABB(Vec3(minp[0]-w, minp[1]-w, minp[2]-w), Vec3(maxp[0]+w, minp[1], maxp[2]+w+front_offset));//下
}


/*!
 * 床描画
 *  - GLSLでテクスチャ付き＆柔らかいスポットライトで照らされているような床を描画
 * @param[in] light_pos,light_color 光源位置と色
 * @param[in] y,s 床の高さと水平方向の長さ
 * @return
 */
static inline void drawFloor(Vec3 light_pos, Vec3 light_color, double y = -1.0, double s = 20.0)
{
	static bool init = true;
	static rxGLSL glslFloor;			//!< GLSLを使った描画
	static GLuint texFloor = 0;				//!< 床のテクスチャ
	static GLuint texFloorShadow = 0;		//!< 床の影用テクスチャ(まだ未実装)
	if(init){
		// シェーダの初期化(最初の呼び出し時だけ実行)
		glslFloor = CreateGLSLFromFile("shaders/floor.vp", "shaders/floor.fp", "floor");
		init = false;

		// 床テクスチャ読み込み
		glActiveTexture(GL_TEXTURE0);
		//loadTexture("floortile.bmp", texFloor, true, false);
		makeCheckerBoardTexture(texFloor, 72, true, false);
		glBindTexture(GL_TEXTURE_2D, texFloor);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

	}

	// 視点座標系のモデルビュー行列取得と視点座標系での光源位置計算
	GLfloat eye_modelview[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, eye_modelview);
	Vec3 light_pos_eye = MulMat4Vec3(eye_modelview, light_pos);

	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// GLSLシェーダをセット
	glUseProgram(glslFloor.Prog);

	glUniform3f(glGetUniformLocation(glslFloor.Prog, "v3LightPosEye"), light_pos_eye[0], light_pos_eye[1], light_pos_eye[2]);
	glUniform3f(glGetUniformLocation(glslFloor.Prog, "v3LightColor"), light_color[0], light_color[1], light_color[2]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texFloor);
	glUniform1i(glGetUniformLocation(glslFloor.Prog, "texFloor"), 0);

	glEnable(GL_LIGHTING);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);	// 裏面をカリング

	// set shadow matrix as texture matrix
	//matrix4f shadowMatrix = renderer->getShadowMatrix();
	//glActiveTexture(GL_TEXTURE0);
	//glMatrixMode(GL_TEXTURE);
	//glLoadMatrixf((GLfloat *)shadowMatrix.get_value());

	// 床用ポリゴン描画
	glColor3d(1.0, 1.0, 1.0);
	glNormal3d(0.0, 1.0, 0.0);
	glBegin(GL_QUADS);
	{
		glTexCoord2d(0.0, 0.0);	glVertex3d(-s, y, -s);
		glTexCoord2d(0.0, 1.0);	glVertex3d(-s, y,  s);
		glTexCoord2d(1.0, 1.0);	glVertex3d( s, y,  s);
		glTexCoord2d(1.0, 0.0);	glVertex3d( s, y, -s);
	}
	glEnd();

	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


/*!
 * xyz軸描画(x軸:赤,y軸:緑,z軸:青)
 * @param[in] len 軸の長さ
 */
static inline int drawAxis(double len, double line_width)
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
static inline void drawWireCuboid(const Vec3& min, const Vec3& max, const Vec3& color, double line_width = 1.0)
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
static inline void DrawWireCircle(const double& rad, const int& n)
{
	double t = 0.0;
	double dt = 2.0*RX_PI/(double)n;

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
static inline void DrawWireCircleXZ(const double& rad, const int& n)
{
	double t = 0.0;
	double dt = 2.0*RX_PI/(double)n;

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
static inline void DrawWireSphere(const Vec3& cen, const float& rad, const Vec3& col)
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
 * 球描画
 * @param cen 球の中心
 * @param rad 球の半径
 * @param col 描画色
 */
static inline void drawSolidSphere(const Vec3& cen, const float& rad, const Vec3& col)
{
	glPushMatrix();
	glTranslatef(cen[0], cen[1], cen[2]);
	glRotatef(90, 1.0, 0.0, 0.0);
	glColor3f(col[0], col[1], col[2]);
	//glutSolidSphere(rad, 20, 10);
	glPopMatrix();
}

/*!
 * 直方体の8頂点座標を中心と辺の長さの1/2(side length)から計算
 * @param[in] cn,sl 立方体の中心と辺の長さの1/2(side length)
 * @param[out] v 8頂点座標
 */
inline void SetVerticesBox(const Vec3& cn, const Vec3& sl, Vec3 v[8])
{
	v[0] = cn+Vec3(-sl[0], -sl[1], -sl[2]);
	v[1] = cn+Vec3(-sl[0], sl[1], -sl[2]);
	v[2] = cn+Vec3(-sl[0], sl[1], sl[2]);
	v[3] = cn+Vec3(-sl[0], -sl[1], sl[2]);

	v[4] = cn+Vec3(sl[0], -sl[1], -sl[2]);
	v[5] = cn+Vec3(sl[0], sl[1], -sl[2]);
	v[6] = cn+Vec3(sl[0], sl[1], sl[2]);
	v[7] = cn+Vec3(sl[0], -sl[1], sl[2]);
}

/*!
 * 上が空いたボックス形状(壁面厚みあり)を表すポリゴンデータの生成
 * @param[in] sl0,sl1 ボックスの内側と外側のサイズ(side length)
 * @param[in] d 空ける方向
 * @param[out] vrts,idxs ポリゴン頂点座標と接続関係
 */
inline void CreateBoxPolygon(const Vec3& sl0, const Vec3& sl1, const int& d,
							 vector<Vec3>& vrts, vector< vector<int> >& idxs)
{
	if(d < 0 || d > 2) return;

	double h = sl1[d]-sl0[d];
	Vec3 cn(0.0);

	vrts.resize(16);

	// 外側の頂点
	SetVerticesBox(cn, sl1, &vrts[0]);

	// 内側の頂点
	cn[d] += h;
	SetVerticesBox(cn, sl0, &vrts[8]);


	int idxs0[5][4] = { {0, 3, 2, 1},
						{1, 2, 6, 5},
						{5, 6, 7, 4},
						{4, 7, 3, 0},
						{0, 1, 5, 4} };

	int idxs1[4][4] = { {2, 3, 11, 10},
						{3, 7, 15, 11},
						{7, 6, 14, 15},
						{6, 2, 10, 14} };

	// 三角形作成
	idxs.resize(28);
	for(int i = 0; i < 28; ++i) idxs[i].resize(3);

	int c = 0;

	// 外側の箱
	for(int i = 0; i < 5; ++i){
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs0[i][j];
		}
		c++;
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs0[i][((j+2 > 3) ? 0 : j+2)];
		}
		c++;
	}

	// 内側の箱
	for(int i = 0; i < 5; ++i){
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs0[i][2-j]+8;
		}
		c++;
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs0[i][(((2-j)+2 > 3) ? 0 : (2-j)+2)]+8;
		}
		c++;
	}

	// 上部
	for(int i = 0; i < 4; ++i){
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs1[i][j];
		}
		c++;
		for(int j = 0; j < 3; ++j){
			idxs[c][j] = idxs1[i][((j+2 > 3) ? 0 : j+2)];
		}
		c++;
	}

}






#endif // #ifndef _RX_GLDRAW_H_