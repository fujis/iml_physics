/*!
  @file main.cpp

  @brief メインファイル(GLFWウィンドウ生成など)

  @author Makoto Fujisawa
  @date   2021-11
*/

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glfw3dll.lib")

#ifdef _DEBUG
#pragma comment (lib, "LinearMath_vs2010_x64_debug.lib")
#pragma comment (lib, "BulletCollision_vs2010_x64_debug.lib")
#pragma comment (lib, "BulletDynamics_vs2010_x64_debug.lib")
#pragma comment (lib, "BulletSoftBody_vs2010_x64_debug.lib")
#else
#pragma comment (lib, "LinearMath_vs2010_x64_release.lib")
#pragma comment (lib, "BulletCollision_vs2010_x64_release.lib")
#pragma comment (lib, "BulletDynamics_vs2010_x64_release.lib")
#pragma comment (lib, "BulletSoftBody_vs2010_x64_release.lib")
#endif


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define GL_SILENCE_DEPRECATION	// mac環境でgluを使っている場合の非推奨warningの抑制

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include "utils.h"

// ImGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

using namespace std;


//-----------------------------------------------------------------------------
// 定数・グローバル変数
//-----------------------------------------------------------------------------
const glm::vec4 LIGHT0_POS(0.5f, 4.0f, 1.5f, 0.0f);
const glm::vec4 LIGHT1_POS(-1.0f, -10.0f, -1.0f, 0.0f);
const glm::vec4 LIGHT_AMBI(0.3f, 0.3f, 0.3f, 1.0f);
const glm::vec4 LIGHT_DIFF(1.0f, 1.0f, 1.0f, 1.0f);
const glm::vec4 LIGHT_SPEC(1.0f, 1.0f, 1.0f, 1.0f);
const GLfloat FOV = 45.0f;

// グローバル変数
int g_winw = 1024;							//!< 描画ウィンドウの幅
int g_winh = 1024;							//!< 描画ウィンドウの高さ
rxTrackball g_view;							//!< 視点移動用トラックボール
float g_bgcolor[3] = { 1, 1, 1 };			//!< 背景色
bool g_animation_on = false;				//!< アニメーションON/OFF
int g_currentstep = 0;						//!< 現在のステップ数

// シャドウマッピング
ShadowMap g_shadowmap;
int g_shadowmap_res = 1024;

// 物理シミュレーション関連定数/変数
float g_dt = 0.002;	//!< 時間ステップ幅

// Bullet
btDynamicsWorld* g_dynamicsworld;	//!< Bulletワールド
btAlignedObjectArray<btCollisionShape*>	g_collisionshapes;		//!< 剛体オブジェクトの形状を格納する動的配列

// マウスピック
btVector3 g_pickpos;
btRigidBody *g_pickbody = 0;
btPoint2PointConstraint *g_pickconstraint = 0;
btSoftBody::Node *g_picknode = 0;
double g_pickdist = 0.0;


//-----------------------------------------------------------------------------
// Bullet用関数
//-----------------------------------------------------------------------------
/*!
* Bullet剛体(btRigidBody)の作成
* @param[in] mass 質量
* @param[in] init_tras 初期位置・姿勢
* @param[in] shape 形状
* @return 作成したbtRigidBody
*/
btRigidBody* CreateRigidBody(double mass, const btTransform& init_trans, btCollisionShape* shape, btDynamicsWorld* world = 0, int index = 0)
{
	//btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

	// 質量が0ならば静的な(static)オブジェクトとして設定，
	bool isDynamic = (mass != 0.0);

	btVector3 inertia(0, 0, 0);
	if(isDynamic)
		shape->calculateLocalInertia(mass, inertia);

	btDefaultMotionState* motion_state = new btDefaultMotionState(init_trans);

	btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, shape, inertia);

	btRigidBody* body = new btRigidBody(rb_info);

	body->setUserIndex(index);

	if(mass <= 1e-10){
		// Kinematicオブジェクトとして設定(stepSimulationしても運動の計算を行わないようにする)
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		// 常にスリープ状態にする
		body->setActivationState(DISABLE_DEACTIVATION);
	}

	if(world){
		world->addRigidBody(body);
	}

	return body;
}

/*!
* 剛体オブジェクトの追加
*/
void SetRigidBodies(void)
{
	btTransform trans;	// 剛体オブジェクトの位置姿勢を格納する変数(行列)
	trans.setIdentity();// 位置姿勢行列の初期化

	const btScalar CUBE_HALF_EXTENTS = 0.2;	// 立方体の変の長さの半分(中心から辺までの距離)
	const btScalar GROUND_HEIGHT = 0.0;		// 地面の高さ

	// ----- 地面(質量0のx-z平面上で平べったい直方体で表現)の追加 -----
	btCollisionShape *ground_shape = new btBoxShape(btVector3(20, CUBE_HALF_EXTENTS, 20));	// 形状
	ground_shape->setUserIndex(99); // 99とした場合のみ描画時にテクスチャ付き平面として描画．床を傾けたい等の場合は99にしないこと．
	trans.setOrigin(btVector3(0, GROUND_HEIGHT-CUBE_HALF_EXTENTS, 0));	// 上の面がy=0になるように設定
	
	// 剛体オブジェクト(Static)生成
	btRigidBody* body0 = CreateRigidBody(0.0, trans, ground_shape, g_dynamicsworld, 99);
	
	g_collisionshapes.push_back(ground_shape); // 最後に破棄(delete)するために形状データを格納しておく
	// ----- ここまで (地面の追加) -----


	// ----- 立方体オブジェクト追加 -----
	// 形状設定
	btCollisionShape *box_shape = new btBoxShape(btVector3(CUBE_HALF_EXTENTS, CUBE_HALF_EXTENTS, CUBE_HALF_EXTENTS));

	// 初期位置・姿勢
	btQuaternion qrot(0, 0, 0, 1);
	trans.setIdentity();// 位置姿勢行列の初期化
	trans.setOrigin(btVector3(0, GROUND_HEIGHT+10.0*CUBE_HALF_EXTENTS, 0));
	trans.setRotation(qrot);	// 四元数を行列に変換して姿勢行列に掛け合わせる

	// 剛体オブジェクト生成
	btRigidBody* body1 = CreateRigidBody(1.0, trans, box_shape, g_dynamicsworld, 0);
	
	g_collisionshapes.push_back(box_shape); // 最後に破棄(delete)するために形状データを格納しておく
	// ----- ここまで (立方体オブジェクト追加) -----


	// すり抜け防止用Swept sphereの設定(CCD:Continuous Collision Detection)
	body1->setCcdMotionThreshold(CUBE_HALF_EXTENTS);
	body1->setCcdSweptSphereRadius(0.05*CUBE_HALF_EXTENTS);
}

/*!
* Bullet初期化
*/
void InitBullet(void)
{
	// 衝突検出方法の選択(デフォルトを選択)
	btDefaultCollisionConfiguration *config = new btDefaultCollisionConfiguration();
	btCollisionDispatcher *dispatcher = new btCollisionDispatcher(config);

	// ブロードフェーズ法の設定(Dynamic AABB tree method)
	btDbvtBroadphase *broadphase = new btDbvtBroadphase();

	// 拘束(剛体間リンク)のソルバ設定
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();

	// Bulletのワールド作成
	g_dynamicsworld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, config);

	// 重力加速度の設定(OpenGLに合わせてy軸方向を上下方向にする)
	g_dynamicsworld->setGravity(btVector3(0, -9.8, 0));

	SetRigidBodies();
}


/*!
* 設定したBulletの剛体オブジェクト，ワールドの破棄
*/
void CleanBullet(void)
{
	// 剛体オブジェクトの破棄
	for(int i = g_dynamicsworld->getNumCollisionObjects()-1; i >= 0; --i){
		btCollisionObject* obj = g_dynamicsworld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if(body && body->getMotionState()){
			delete body->getMotionState();
		}
		g_dynamicsworld->removeCollisionObject( obj );
		delete obj;
	}

	// 形状の破棄
	for(int j = 0; j < (int)g_collisionshapes.size(); ++j){
		btCollisionShape* shape = g_collisionshapes[j];
		g_collisionshapes[j] = 0;
		delete shape;
	}
	g_collisionshapes.clear();

	// ワールド破棄
	delete g_dynamicsworld->getBroadphase();
	delete g_dynamicsworld;
}




//-----------------------------------------------------------------------------
// アプリケーション制御関数
//-----------------------------------------------------------------------------
/*!
* アニメーションN/OFF
* @param[in] on trueでON, falseでOFF
*/
bool switchanimation(int on)
{
	g_animation_on = (on == -1) ? !g_animation_on : (on ? true : false);
	return g_animation_on;
}

/*!
* 現在の画面描画を画像ファイルとして保存(連番)
* @param[in] stp 現在のステップ数(ファイル名として使用)
*/
void savedisplay(const int &stp)
{
	static int nsave = 1;
	string fn = CreateFileName("img_", ".bmp", (stp == -1 ? nsave++ : stp), 5);
	saveFrameBuffer(fn, g_winw, g_winh);
	std::cout << "saved the screen image to " << fn << std::endl;
}
/*!
* 視点の初期化
*/
void resetview(void)
{
	double q[4] = { 1, 0, 0, 0 };
	g_view.SetQuaternion(q);
	//g_view.SetRotation(20.0, 1.0, 0.0, 0.0);
	g_view.SetScaling(-7.0);
	g_view.SetTranslation(0.0, -2.0);
}
/*!
* シミュレーションのリセット
*/
void reset(void)
{
	CleanBullet();
	InitBullet();
	g_currentstep = 0;
}


/*!
* 初期化関数
*/
void Init(void)
{
	// OpenGLのバージョンチェック
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;

	// GLEWの初期化
	GLenum err = glewInit();
	if(err != GLEW_OK) cout << "GLEW Error : " << glewGetErrorString(err) << endl;

	// 描画系フラグ設定(アンチエイリアス,デプステスト,隠面除去,法線計算,点描画)
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_POINT_SMOOTH);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(LIGHT0_POS));
	glLightfv(GL_LIGHT0, GL_DIFFUSE, glm::value_ptr(LIGHT_DIFF));
	glLightfv(GL_LIGHT0, GL_SPECULAR, glm::value_ptr(LIGHT_SPEC));
	glLightfv(GL_LIGHT0, GL_AMBIENT, glm::value_ptr(LIGHT_AMBI));

	// 視点初期化
	resetview();
	
	// シャドウマップ初期化
	g_shadowmap.InitShadow(g_shadowmap_res, g_shadowmap_res);

	// Bullet初期化
	InitBullet();

	switchanimation(1);
}


//-----------------------------------------------------------------------------
// OpenGL/GLFWコールバック関数
//-----------------------------------------------------------------------------
/*!
* Bulletのオブジェクトの描画シーン描画
* @param[in] x クラスのメンバ関数(static)を渡すときに用いるポインタ(グローバル関数の場合は使わないので0でOK)
*/
void DrawBulletObjects(void* x = 0)
{
	static const GLfloat difr[] = { 1.0, 0.4, 0.4, 1.0 };	// 拡散色 : 赤
	static const GLfloat difg[] = { 0.4, 0.6, 0.4, 1.0 };	// 拡散色 : 緑
	static const GLfloat difb[] = { 0.4, 0.4, 1.0, 1.0 };	// 拡散色 : 青
	static const GLfloat spec[] = { 0.4, 0.4, 0.4, 1.0 };	// 鏡面反射色
	static const GLfloat ambi[] = { 0.3, 0.3, 0.3, 1.0 };	// 環境光

	glDisable(GL_COLOR_MATERIAL);

	// 光源/材質設定
	glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(LIGHT0_POS));
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialfv(GL_FRONT, GL_AMBIENT,  ambi);
	glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(0.0, 0.0, 1.0);

	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	if(g_dynamicsworld){
		btScalar m[16];
		btMatrix3x3	rot;
		rot.setIdentity();

		// Bulletワールドから剛体オブジェクト情報を取得してOpenGLで描画
		const int n = g_dynamicsworld->getNumCollisionObjects();	// オブジェクト数の取得
		for(int i = 0; i < n; ++i){

			// btCollisionObject → btRigidBodyへのキャストで剛体オブジェクトを取得
			btCollisionObject* obj = g_dynamicsworld->getCollisionObjectArray()[i];

			// 形状取得
			btCollisionShape* shape = obj->getCollisionShape();
			int shapetype = shape->getShapeType();

			if(shapetype == SOFTBODY_SHAPE_PROXYTYPE){
				btSoftBody* body = btSoftBody::upcast(obj);

				glMaterialfv(GL_FRONT, GL_DIFFUSE, difb);

				// draw a softbody
				DrawBulletSoftBody(body);
			}
			else{
				btRigidBody* body = btRigidBody::upcast(obj);
				if(body && body->getMotionState()){
					// btRigidBodyからMotion Stateを取得して，OpenGLの変換行列として位置・姿勢情報を得る
					btDefaultMotionState* ms = (btDefaultMotionState*)body->getMotionState();
					ms->m_graphicsWorldTrans.getOpenGLMatrix(m);
					rot = ms->m_graphicsWorldTrans.getBasis();
				}
				else{
					obj->getWorldTransform().getOpenGLMatrix(m);
					rot = obj->getWorldTransform().getBasis();
				}

				if(body && body->getInvMass() > 1e-6){
					// Dynamicボディは青で描画
					glMaterialfv(GL_FRONT, GL_DIFFUSE, difb);
				}
				else{	// Kinematicボディの場合は緑で描画
					glMaterialfv(GL_FRONT, GL_DIFFUSE, difg);
				}

				btVector3 world_min, world_max;
				g_dynamicsworld->getBroadphase()->getBroadphaseAabb(world_min, world_max);

				glPushMatrix();
#ifdef BT_USE_DOUBLE_PRECISION
				glMultMatrixd(m);
#else
				glMultMatrixf(m);
#endif

				// 形状描画
				DrawBulletShape(shape, world_min, world_max);

				glPopMatrix();
			}
		}
	}

}


/*!
* 再描画イベントコールバック関数
*/
void Display(void)
{
	// ビューポート,透視変換行列,モデルビュー変換行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glm::mat4 mp = glm::perspective(FOV, (float)g_winw/g_winh, 0.2f, 1000.0f);
	glMultMatrixf(glm::value_ptr(mp));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// 描画バッファのクリア
	glClearColor((GLfloat)g_bgcolor[0], (GLfloat)g_bgcolor[1], (GLfloat)g_bgcolor[2], 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	// マウスによる回転・平行移動の適用
	g_view.Apply();

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);

	// 影なしでのオブジェクト描画
	//DrawBulletObjects();

	// シャドウマップを使って影付きでオブジェクト描画
	glm::vec3 light_pos(LIGHT0_POS[0], LIGHT0_POS[1], LIGHT0_POS[2]);
	ShadowMap::Frustum light = CalFrustum(80, 0.02, 20.0, g_shadowmap_res, g_shadowmap_res, light_pos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	g_shadowmap.RenderSceneWithShadow(light, DrawBulletObjects, 0);

	glPopMatrix();
}

/*!
* タイマーコールバック関数
*/
void Timer(void)
{
	if(g_animation_on){
		if(g_dynamicsworld){
			// シミュレーションを1ステップ進める
			g_dynamicsworld->stepSimulation(g_dt, 1);
		}
		g_currentstep++;
	}
}


/*!
* キーボードイベント処理関数
* @param[in] window コールバック関数を呼んだウィンドウハンドル
* @param[in] key キーの種類 -> https://www.glfw.org/docs/latest/group__keys.html
* @param[in] scancode キーのスキャンコード(プラットフォーム依存)
* @param[in] action アクション(GLFW_PRESS:キーを押す, GLFW_RELESE:キーを離す，GLFW_REPEAT:キーリピート機能時)
* @param[in] mods 修飾キー(CTRL,SHIFT,ALT) -> https://www.glfw.org/docs/latest/group__mods.html
*/
void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(ImGui::GetIO().WantCaptureKeyboard) return;	// ImGUIウィンドウ上でのキーボードイベント時
	if(action == GLFW_PRESS || action == GLFW_REPEAT){
		switch(key){
		case GLFW_KEY_ESCAPE:	// ESC,Qキーでアプリケーション終了
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;

		case GLFW_KEY_S: // SキーでアニメーションON/OFF
			switchanimation(-1);
			break;
		case GLFW_KEY_SPACE: // スペースキーでアニメーションを1ステップだけ進める
			g_animation_on = true; Timer(); g_animation_on = false;
			break;
			break;

		case GLFW_KEY_R: // Rキーでシーン(シミュレーション)リセット
			reset();
			break;

		default:
			break;
		}
	}
}

/*!
* マウスイベント処理関数
* @param[in] window コールバック関数を呼んだウィンドウハンドル
* @param[in] button マウスボタン(GLFW_MOUSE_BUTTON_LEFT,GLFW_MOUSE_BUTTON_MIDDLE,GLFW_MOUSE_BUTTON_RIGHT)
* @param[in] action マウスボタンの状態(GLFW_PRESS, GLFW_RELEASE)
* @param[in] mods 修飾キー(CTRL,SHIFT,ALT) -> https://www.glfw.org/docs/latest/group__mods.html
*/
void Mouse(GLFWwindow* window, int button, int action, int mods)
{
	if(ImGui::GetIO().WantCaptureMouse) return;	// ImGUIウィンドウ上でのマウスイベント時
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	if(button == GLFW_MOUSE_BUTTON_LEFT){
		if(action == GLFW_PRESS){
			g_picknode = 0;
			glm::vec3 ray_from0, ray_to0;
			glm::vec3 init_pos(0, 0, 0);
			g_view.CalLocalPos(ray_from0, init_pos);
			g_view.GetRayTo(x, y, FOV, ray_to0);

			btVector3 ray_from = btVector3(ray_from0[0], ray_from0[1], ray_from0[2]);
			btVector3 ray_to = btVector3(ray_to0[0], ray_to0[1], ray_to0[2]);

			btCollisionWorld::ClosestRayResultCallback ray_callback(ray_from, ray_to);
			g_dynamicsworld->rayTest(ray_from, ray_to, ray_callback);

			if(ray_callback.hasHit()){
				const btCollisionObject* obj = ray_callback.m_collisionObject;

				// 光線と衝突した剛体
				btRigidBody* body = const_cast<btRigidBody*>(btRigidBody::upcast(obj));

				// 衝突点座標(ジョイントになる位置座標)
				btVector3 picked_pos = ray_callback.m_hitPointWorld;

				if(body){
					if(!(body->isStaticObject() || body->isKinematicObject())){
						g_pickbody = body;
						g_pickpos = picked_pos;

						// 選択された剛体の座標系でのピック位置
						btVector3 local_pos = body->getCenterOfMassTransform().inverse()*picked_pos;

						g_pickbody->setActivationState(DISABLE_DEACTIVATION); // 必要！

						if(g_pickconstraint){
							g_dynamicsworld->removeConstraint(g_pickconstraint);
							delete g_pickconstraint;
						}
						g_pickconstraint = new btPoint2PointConstraint(*body, local_pos);
						g_dynamicsworld->addConstraint(g_pickconstraint, true);

						g_pickconstraint->m_setting.m_impulseClamp = 30.0;
						g_pickconstraint->m_setting.m_tau = 0.001f;

						g_pickdist = (g_pickpos-ray_from).length();
					}
				} 
				else{
					// 光線と衝突したbtSoftBody
					btSoftBody* body = const_cast<btSoftBody*>(btSoftBody::upcast(obj));
					btSoftBody::sRayCast res;
					body->rayTest(ray_from, ray_to, res);
					if(res.fraction < 1.0){
						btVector3 impact = ray_from+(ray_to-ray_from)*res.fraction;
						cout << impact << endl;
						if(res.feature == btSoftBody::eFeature::Face){
							btSoftBody::Face& face = res.body->m_faces[res.index];

							// 衝突点に最も近いノードを探索
							btSoftBody::Node* node = face.m_n[0];
							for(int i = 1; i < 3; ++i){
								if((node->m_x-impact).length2() >(face.m_n[i]->m_x-impact).length2()){
									node = face.m_n[i];
								}
							}
							g_picknode = node;
							g_pickdist = (g_picknode->m_x - ray_from).length();
						}
					}
				}
			}

			if(!g_pickconstraint && !g_picknode){
				// マウスドラッグによる視点移動
				g_view.Start(x, y, mods);
			}
		}
		else if(action == GLFW_RELEASE){
			if(g_pickconstraint){
				g_dynamicsworld->removeConstraint(g_pickconstraint);
				delete g_pickconstraint;
				g_pickconstraint = 0;
				g_pickbody = 0;
			}
			else if(g_picknode){
				g_picknode = 0;
			}
			else{
				g_view.Stop(x, y);
			}
		}
	}
}
/*!
* モーションイベント処理関数(マウスボタンを押したままドラッグ)
* @param[in] window コールバック関数を呼んだウィンドウハンドル
* @param[in] x,y マウス座標(スクリーン座標系)
*/
void Motion(GLFWwindow* window, double x, double y)
{
	if(ImGui::GetIO().WantCaptureMouse) return;	// ImGUIウィンドウ上でのマウスイベント時
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE){
		return;
	}

	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
		if(g_pickconstraint || g_picknode){
			glm::vec3 ray_from0, ray_to0;
			glm::vec3 init_pos(0, 0, 0);
			g_view.CalLocalPos(ray_from0, init_pos);
			g_view.GetRayTo(x, y, FOV, ray_to0);

			btVector3 ray_from = btVector3(ray_from0[0], ray_from0[1], ray_from0[2]);
			btVector3 new_ray_to = btVector3(ray_to0[0], ray_to0[1], ray_to0[2]);

			btVector3 dir = new_ray_to-ray_from;
			dir.normalize();

			btVector3 new_pivot = ray_from+dir*g_pickdist;

			if(g_pickconstraint){
				g_pickconstraint->setPivotB(new_pivot);
			}
			else if(g_picknode){
				//g_picknode->m_x = new_pivot;
				g_picknode->m_f += (new_pivot-g_picknode->m_x)*10.0;
			}

			g_pickpos = new_pivot;
		}
		else{
			g_view.Motion(x, y);
		}
	}
}



/*!
* リサイズイベント処理関数
* @param[in] window コールバック関数を呼んだウィンドウハンドル
* @param[in] w キャンバス幅(ピクセル数)
* @param[in] h キャンバス高さ(ピクセル数)
*/
void Resize(GLFWwindow* window, int w, int h)
{
	g_winw = w; g_winh = h;
	g_view.SetRegion(w, h);
	glViewport(0, 0, g_winw, g_winh);
}

/*!
* ImGUIのウィジット配置
*  - ImGUI/imgui_demo.cppを参考に ( https://github.com/ocornut/imgui#demo )
* @param[in] window コールバック関数を呼んだウィンドウハンドル
*/
void SetImGUI(GLFWwindow* window)
{
	ImGui::Text("simulation:");
	if(ImGui::Button("start/stop")){ switchanimation(-1); } ImGui::SameLine();
	if(ImGui::Button("run a step")){ g_animation_on = true; Timer(); g_animation_on = false; }
	if(ImGui::Button("reset")){ reset(); }
	ImGui::Separator();
	ImGui::InputFloat("dt", &(g_dt), 0.001f, 0.01f, "%.3f");
	ImGui::Separator();
	if(ImGui::Button("reset viewpos")){ resetview(); } 
	if(ImGui::Button("save screenshot")){ savedisplay(-1); }
	if(ImGui::Button("quit")){ glfwSetWindowShouldClose(window, GL_TRUE); }
}

void Clean()
{
	CleanBullet();
}




/*!
 * メインルーチン
 * @param[in] argc コマンドライン引数の数
 * @param[in] argv コマンドライン引数
 */
int main(int argc, char *argv[])
{
	if(!glfwInit()) return 1;
	glfwSetErrorCallback(glfw_error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

	// Create window
	GLFWwindow* window = glfwCreateWindow(g_winw, g_winh, "OpenGL Application", NULL, NULL);
	if(window == NULL) return 1;

	// Set glfw window as current OpenGL rendering context
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glfwSwapInterval(0); // Disable vsync

	// Initilization for OpenGL
	Init();

	// Setup callback functions
	glfwSetCursorPosCallback(window, Motion);
	glfwSetMouseButtonCallback(window, Mouse);
	glfwSetKeyCallback(window, Keyboard);
	glfwSetFramebufferSizeCallback(window, Resize);
	Resize(window, g_winw, g_winh);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	// Settings for timer
	float cur_time = 0.0f, last_time = 0.0f, elapsed_time = 0.0f;
	glfwSetTime(0.0);	// Initialize the glfw timer

	// Main loop
	while(!glfwWindowShouldClose(window))
	{
		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// OpenGL Rendering & Animation function
		Display();

		// Timer
		cur_time = glfwGetTime();
		elapsed_time = cur_time-last_time;
		if(elapsed_time >= g_dt){
			Timer();
			last_time = glfwGetTime();
		}

		// Start the ImGui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// GUI
		ImGui::Begin("ImGui Window");
		ImGui::Text("Framerate: %.3f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Separator();
		SetImGUI(window);
		ImGui::End();

		// Rendering of the ImGUI frame in opengl canvas
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	Clean();
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();


	return 0;
}


