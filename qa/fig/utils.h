/*! @file utils.h
	
	@brief 様々な関数
 
	@author Makoto Fujisawa
	@date 2012
*/

#ifndef _RX_UTILS_H_
#define _RX_UTILS_H_

#pragma warning (disable: 4101)

//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <string>

//#include <GL/glew.h>
//#include <GL/glut.h>
#include <GLUT/GLUT.h>

#include "rx_utility.h"		// Vec3や様々な関数
#include "rx_trackball.h"	// 回転，平行移動

#include "rx_shadow_empty.h"	// シャドウマップ
//#include "rx_bitmap.h"		// BMP画像読み書き

// Bullet関連
#include <btBulletDynamicsCommon.h>  

#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBody.h>


using namespace std;

//-----------------------------------------------------------------------------
// 定数・定義
//-----------------------------------------------------------------------------
#define RX_MAX_CONTACTS 1024

inline std::ostream &operator<<(std::ostream &out, const btVector3 &a)
{
	return out << "(" << a[0] << ", " << a[1] << ", " << a[2] << ")" ;
}


inline std::istream &operator>>(std::istream &in, btVector3& a)
{
	return in >> a[0] >> a[1] >> a[2] ;
}

//-----------------------------------------------------------------------------
// OpenGL
//-----------------------------------------------------------------------------
/*!
 * xyz軸描画(x軸:赤,y軸:緑,z軸:青)
 * @param[in] len 軸の長さ
 */
inline int DrawAxis(double len, double line_width = 5.0)
{
	glLineWidth(line_width);

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
 * 円筒描画
 * @param[in] rad,len 半径と中心軸方向長さ
 * @param[in] axis 軸方向
 * @param[in] slices  ポリゴン近似する際の分割数
 */
static void DrawCylinder(double rad, double len, int axis, int slices)
{
	GLUquadricObj *qobj;
	qobj = gluNewQuadric();

	glPushMatrix();
	switch(axis){
	case 0:
		glRotatef(-90.0, 0.0, 1.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 1:
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 2:
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	default:
		glTranslatef(0.0, 0.0, -0.5*len);
	}

	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	gluCylinder(qobj, rad, rad, len, slices, slices);

	glPushMatrix();
	glRotatef(180.0, 1.0, 0.0, 0.0);
	gluDisk(qobj, 0.0, rad, slices, slices);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, 0.0, len);
	gluDisk(qobj, 0.0, rad, slices, slices);
	glPopMatrix();

	glPopMatrix();
}

/*!
 * カプセル描画(円筒の両端に半球をつけた形)
 * @param[in] rad,len 半径と中心軸方向長さ
 * @param[in] axis 軸方向
 * @param[in] slices  ポリゴン近似する際の分割数
 */
static void DrawCapsule(double rad, double len, int axis, int slices)
{
	GLUquadricObj *qobj;
	qobj = gluNewQuadric();

	glPushMatrix();
	switch(axis){
	case 0:
		glRotatef(-90.0, 0.0, 1.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 1:
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 2:
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	default:
		glTranslatef(0.0, 0.0, -0.5*len);
	}

	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	gluCylinder(qobj, rad, rad, len, slices, slices);

	glPushMatrix();
	glutSolidSphere(rad, slices, slices);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, 0.0, len);
	glutSolidSphere(rad, slices, slices);
	glPopMatrix();

	glPopMatrix();

}

/*!
 * 画像読込み -> OpenGLテクスチャ登録
 * @param[in] fn ファイル名
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
//static int LoadTexture(const string& fn, GLuint& tex_name, bool mipmap, bool compress)
//{
//	// 画像読み込み
//	int w, h, c, wstep;
//	unsigned char* pimg;
//	pimg = ReadBitmapFile(fn, w, h, c, wstep, true, false);
//	if(!pimg){
//		return 0;
//	}
//
//	GLuint iformat, format;
//
//	// 画像フォーマット
//	format = GL_RGBA;
//	if(c == 1){
//		format = GL_LUMINANCE;
//	}
//	else if(c == 3){
//		format = GL_RGB;
//	}
//
//	// OpenGL内部の格納フォーマット
//	if(compress){
//		iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
//		if(c == 1){
//			iformat = GL_COMPRESSED_LUMINANCE_ARB;
//		}
//		else if(c == 3){
//			iformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
//		}
//	}
//	else{
//		iformat = GL_RGBA;
//		if(c == 1){
//			iformat = GL_LUMINANCE;
//		}
//		else if(c == 3){
//			iformat = GL_RGB;
//		}
//	}
//
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//
//	// テクスチャ作成
//	if(tex_name == 0){
//		glGenTextures(1, &tex_name);
//
//		glBindTexture(GL_TEXTURE_2D, tex_name);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//		if(mipmap){
//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 6);
//		}
//
//		glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, GL_UNSIGNED_BYTE, pimg);
//
//		if(mipmap){
//			glGenerateMipmapEXT(GL_TEXTURE_2D);
//		}
//	}
//	else{
//		glBindTexture(GL_TEXTURE_2D, tex_name);
//		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, pimg);
//		glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, GL_UNSIGNED_BYTE, pimg);
//
//		if(mipmap){
//			glGenerateMipmapEXT(GL_TEXTURE_2D);
//		}
//	}
//
//	glBindTexture(GL_TEXTURE_2D, 0);
//
//	delete[] pimg;
//
//	return 1;
//}

/*!
 * チェックボードテクスチャの生成とOpenGLへの登録
 * @param[in] fn ファイル名
 * @param[inout] tex_name テクスチャ名(0なら新たに生成)
 * @param[in] mipmap ミップマップ使用フラグ
 * @param[in] compress テクスチャ圧縮使用フラグ
 */
static int MakeCheckerBoardTexture(GLuint& tex_name, const int size = 72, bool mipmap = false, bool compress = false)
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
	} else{
		glBindTexture(GL_TEXTURE_2D, tex_name);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, format, GL_UNSIGNED_BYTE, pimg);
		glTexImage2D(GL_TEXTURE_2D, 0, iformat, size, size, 0, format, GL_UNSIGNED_BYTE, pimg);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pimg;

	return 1;
}


/*!
 * y方向を法線とする平面ポリゴン描画
 *  - GLSLでテクスチャ付き＆柔らかいスポットライトで照らされているような床を描画
 * @param[in] light_pos,light_color 光源位置と色
 * @param[in] y,s 床の高さと水平方向の長さ
 * @return
 */
static void DrawStaticPlane(double y = 0.0, double s = 20.0, bool use_tex = true)
{
	static GLuint texFloor = 0;				//!< 床のテクスチャ
	if(use_tex && texFloor == 0){
		// 床テクスチャ生成 (任意画像を使いたい場合はここで読み込み)
		//loadTexture("floortile.bmp", texFloor, true, false);
		MakeCheckerBoardTexture(texFloor, 72, true, false);
		glBindTexture(GL_TEXTURE_2D, texFloor);

	}

	if(use_tex){
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, texFloor);
	}

	glEnable(GL_LIGHTING);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);	// 裏面をカリング

	// 床用ポリゴン描画
	glColor3d(1.0, 1.0, 1.0);
	glNormal3d(0.0, 1.0, 0.0);
	glBegin(GL_QUADS);
	{
		glTexCoord2d(0.0, 0.0);	glVertex3d(-s, y, -s);
		glTexCoord2d(0.0, 1.0);	glVertex3d(-s, y, s);
		glTexCoord2d(1.0, 1.0);	glVertex3d(s, y, s);
		glTexCoord2d(1.0, 0.0);	glVertex3d(s, y, -s);
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}


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


static inline void DrawBulletSoftBody(btSoftBody* sbody)
{
	if(sbody->m_faces.size() == 0){
		glBegin(GL_LINE_STRIP);
		for(int i = 0; i < sbody->m_nodes.size(); ++i){
			const btSoftBody::Node &node = sbody->m_nodes[i];
			btVector3 p = node.m_x;
			glVertex3f(p.x(), p.y(), p.z());
		}
		glEnd();
	}

	for(int i = 0; i < sbody->m_faces.size(); ++i){
		const btSoftBody::Face &face = sbody->m_faces[i];

		glBegin(GL_POLYGON);
		for(int j = 0; j < 3; ++j){
			const btSoftBody::Node* node = face.m_n[j];
			btVector3 p = node->m_x;
			btVector3 n = node->m_n;
			glNormal3f(n.x(), n.y(), n.z());
			glVertex3f(p.x(), p.y(), p.z());

		}
		glEnd();
	}
}

/*!
* Bulletの衝突形状を描画
* @param[in] shape 衝突形状
*/
static void DrawBulletShape(const btCollisionShape *shape, btVector3 &world_min, btVector3 &world_max)
{
	int shapetype = shape->getShapeType();

	glPushMatrix();

	// 形状の種類ごとに描画
	if(shapetype == STATIC_PLANE_PROXYTYPE){
		// 平面
		const btStaticPlaneShape* plane = static_cast<const btStaticPlaneShape*>(shape);
		DrawStaticPlane(plane->getPlaneConstant(), 40.0, true);
	} else if(shapetype == BOX_SHAPE_PROXYTYPE){
		// ボックス形状
		const btBoxShape* box = static_cast<const btBoxShape*>(shape);
		btVector3 half_extent = box->getHalfExtentsWithMargin();
		if(box->getUserIndex() == 99){
			DrawStaticPlane(half_extent[1], 40.0, true);
		}
		else{
			glScaled(2*half_extent[0], 2*half_extent[1], 2*half_extent[2]);
			glutSolidCube(1.0);
		}
	} else if(shapetype == SPHERE_SHAPE_PROXYTYPE){
		// 球形状
		const btSphereShape* sphere = static_cast<const btSphereShape*>(shape);
		double rad = sphere->getRadius();
		glutSolidSphere(rad, 32, 32);
	} else if(shapetype == CYLINDER_SHAPE_PROXYTYPE){
		// 円筒形状
		const btCylinderShape* cylinder = static_cast<const btCylinderShape*>(shape);
		double rad = cylinder->getRadius();
		int up_axis = cylinder->getUpAxis();
		double len = cylinder->getHalfExtentsWithMargin()[up_axis]*2;
		DrawCylinder(rad, len, up_axis, 8);
	} else if(shapetype == CAPSULE_SHAPE_PROXYTYPE){
		// カプセル形状
		const btCapsuleShape* capsule = static_cast<const btCapsuleShape*>(shape);
		double rad = capsule->getRadius();
		int up_axis = capsule->getUpAxis();
		double len = capsule->getHalfHeight()*2;
		DrawCapsule(rad, len, up_axis, 16);
	} else if(shapetype == TRIANGLE_MESH_SHAPE_PROXYTYPE){
		// 三角形メッシュ
		const btBvhTriangleMeshShape* mesh = static_cast<const btBvhTriangleMeshShape*>(shape);
		TriangleDrawCallback draw_callback;
		mesh->processAllTriangles(&draw_callback, world_min, world_max);
	} else if(shapetype == GIMPACT_SHAPE_PROXYTYPE){
		// 三角形メッシュ(GIMPACT)
		const btGImpactMeshShape* mesh = static_cast<const btGImpactMeshShape*>(shape);
		TriangleDrawCallback draw_callback;
		mesh->processAllTriangles(&draw_callback, world_min, world_max);
	} else if(shapetype == COMPOUND_SHAPE_PROXYTYPE){
		// 複合形状
		const btCompoundShape* compound = static_cast<const btCompoundShape*>(shape);
		int num_child = compound->getNumChildShapes();
		btScalar mc[16];
		for(int j = 0; j < num_child; ++j){
			// Compound Shapeを構成するchild shapeを再帰的にDrawBulletShapeに渡す
			compound->getChildTransform(j).getOpenGLMatrix(mc);
			glPushMatrix();
			glMultMatrixf(mc);
			DrawBulletShape(compound->getChildShape(j), world_min, world_max);
			glPopMatrix();
		}
	} else{
		glutSolidSphere(1.0, 10, 10);
	}

	glPopMatrix();
}




#endif // #ifndef _RX_UTILS_H_