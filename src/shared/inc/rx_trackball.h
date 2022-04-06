/*!
  @file rx_trackball.h
	
  @brief 簡易トラックボール処理
 
  @author Makoto Fujisawa
  @date 2008
*/
// FILE --rx_trackball.h--

#ifndef _RX_TRACKBALL_H_
#define _RX_TRACKBALL_H_


#include <math.h>
#include <GLFW/glfw3.h>


//-----------------------------------------------------------------------------
// トラックボール関数
//-----------------------------------------------------------------------------
class rxTrackball
{
	template<class T>
	inline bool RX_FEQ2(const T& a, const T& b, const T& eps){ return (fabs(a-b) < eps); }

	const double TB_PI = 3.14159265358979323846;
	const double TB_ROT_SCALE = 2.0*TB_PI;	//!< マウスの相対位置→回転角の換算係数

	const double RX_FEQ_EPS = 1.0e-10;

	// deg->rad の変換(pi/180.0)
	const double RX_DEGREES_TO_RADIANS = 0.0174532925199432957692369076848;
	// rad->deg の変換(180.0/pi)
	const double RX_RADIANS_TO_DEGREES = 57.295779513082320876798154814114;

	template<class T>
	inline T RX_TO_RADIANS(const T& x){ return static_cast<T>((x)*RX_DEGREES_TO_RADIANS); }
	template<class T>
	inline T RX_TO_DEGREES(const T& x){ return static_cast<T>((x)*RX_RADIANS_TO_DEGREES); }

	inline int IDX(int row, int col){ return (row | (col<<2)); }
	//inline int IDX(int row, int col){ return (4*row+col); }

public:
	rxTrackball()
	{
		m_fTransScale = 10.0;
		m_iVeloc[0] = 0; m_iVeloc[1] = 0;

		initq(m_fQuatRot);		// 回転(クォータニオン)
		initq(m_fQuatIncRot);	// ドラッグ中の回転増分量(クォータニオン)
		identity(m_fMatRot);		// 回転の変換行列

		m_fTransDist[0] = 0.0; m_fTransDist[1] = 0.0;// カメラパン
		m_fScaleDist = 0.0;			// スケーリング

		m_iDrag = 0;		// ドラッグ中か否か(1:左ドラッグ, 2:右ドラッグ, 3:ミドルドラッグ)

	}
	~rxTrackball(){}

	//-----------------------------------------------------------------------------
	// 初期化と設定
	//-----------------------------------------------------------------------------
	/*
	 * トラックボール処理の初期化 - プログラムの初期化処理のところで実行する
	 */
	void Init(void)
	{
		m_iDrag = 0;						// ドラッグ中かどうかのフラグのリセット
		initq(m_fQuatIncRot);			// 単位クォーターニオンで初期化
		q2m(m_fMatRot, m_fQuatRot);		// 回転行列の初期化
		m_iVeloc[0] = 0; m_iVeloc[1] = 0;	// カーソル速度の初期化
	}

	/*
	 * トラックボールする領域(マウスポインタ位置のウィンドウ内の相対的位置への換算用)
	 *  - Reshape コールバック (resize) の中で実行する
	 * @param[in] w,h ビューポートの大きさ
	 */
	void SetRegion(int w, int h){ m_fW = (double)w; m_fH = (double)h; }

	/*
	 * 3D空間領域の設定
	 * @param[in] l 代表長さ
	 */
	void SetSpace(double l){ m_fTransScale = 10.0*l; }


	//-----------------------------------------------------------------------------
	// マウスドラッグ
	//-----------------------------------------------------------------------------
	/*
	 * ドラッグ開始 - マウスボタンを押したときに実行する
	 * @param[in] x,y マウス位置
	 */
	void Start(int x, int y, int button)
	{
		if(x < 0 || y < 0 || x >(int)m_fW || y >(int)m_fH) return;
		m_iSx = x; m_iSy = y;  m_iPx = x; m_iPy = y;	// ドラッグ開始点を記録
		m_iDrag = button;	// ドラッグ開始
	}

	/*
	 * ドラッグ中
	 *  - マウスのドラッグ中に実行する
	 * @param[in] x,y マウス位置
	 */
	void Motion(int x, int y, bool last = false)
	{
		if(x < 0 || y < 0 || x >(int)m_fW || y >(int)m_fH) return;
		if(m_iDrag == 0){		// 回転
			// マウスポインタの位置のドラッグ開始位置からの変位
			trackballRot((double)(x-m_iPx), (double)(y-m_iPy));
		}
		else if(m_iDrag & 0x01){	// パン
		 // マウスポインタの位置の前回位置からの変位
			double dx = (x-m_iPx)/m_fW;
			double dy = (y-m_iPy)/m_fH;
			m_fTransDist[0] += m_fTransScale*dx;
			m_fTransDist[1] -= m_fTransScale*dy;
		}
		else if(m_iDrag & 0x02){	// スケーリング
		 // マウスポインタの位置の前回位置からの変位
			double dx = (x-m_iPx)/m_fW;
			double dy = (y-m_iPy)/m_fH;
			m_fScaleDist += m_fTransScale*dy;
		}

		if(!last){
			m_iVeloc[0] = x-m_iPx; m_iVeloc[1] = y-m_iPy;
		}
		m_iPx = x; m_iPy = y;
	}

	/*
	 * 停止
	 *  - マウスボタンを離したときに実行する
	 * @param[in] x,y マウス位置
	 */
	void Stop(int x, int y)
	{
		if(x < 0 || y < 0 || x >(int)m_fW || y >(int)m_fH || !m_iDrag) return;
		// ドラッグ終了点における回転を求める
		trackballRot(x-m_iPx, y-m_iPy);
		m_iDrag = 0;		// ドラッグ終了
	}



	//-----------------------------------------------------------------------------
	// パラメータ取得
	//-----------------------------------------------------------------------------
	//! 回転の変換行列を返す - 返値を glMultMatrixd() などで使用してオブジェクトを回転する
	double* GetRotation(void){ return m_fMatRot; }
	void GetQuaternion(double q[4]){ copyq(q, m_fQuatRot); }

	//! スケーリング量を返す - 返値を glTranslatef() などで使用して視点移動
	double GetScaling(void) const { return m_fScaleDist; }
	void GetScaling(double& s) const { s = m_fScaleDist; }

	//! 平行移動量を返す(i=0でx,i=1でy要素を返す) - 返値を glTranslatef() などで使用して視点移動
	double GetTranslation(int i) const { return m_fTransDist[i]; }
	void GetTranslation(double t[2]) const { t[0] = m_fTransDist[0]; t[1] = m_fTransDist[1]; }

	//! 回転・平行移動・スケーリングの値を直接編集するための関数(各変数へのポインタを返す)
	double* GetQuaternionR(void){ return m_fQuatRot; }
	double* GetTranslationR(void){ return m_fTransDist; }
	double* GetScalingR(void){ return &m_fScaleDist; }

	//! 全変換行列を引数で返す
	void GetTransform(double m[16])
	{
		identity(m);
		for(int i = 0; i < 3; ++i){
			for(int j = 0; j < 3; ++j){
				m[IDX(i, j)] = m_fMatRot[IDX(i, j)];
			}
		}
		m[IDX(0, 3)] = m_fTransDist[0];
		m[IDX(1, 3)] = m_fTransDist[1];
		m[IDX(2, 3)] = m_fScaleDist;
	}


	/*!
	 * トラックボール速度
	 * @param[out] vx,vy 速度(ピクセル/フレーム)
	 */
	void GetLastVeloc(int& vx, int& vy){ vx = m_iVeloc[0]; vy = m_iVeloc[1]; }

	/*!
	 * グローバル座標からオブジェクトローカル座標への変換
	 * @param[out] dst ローカル座標
	 * @param[in]  src グローバル座標
	 */
	void CalLocalPos(glm::vec3 &dst, const glm::vec3 &src)
	{
		// 平行移動
		glm::vec3 pos;
		pos[0] = src[0]-m_fTransDist[0];
		pos[1] = src[1]-m_fTransDist[1];

		// スケーリング
		pos[2] = src[2]-m_fScaleDist;

		// 回転
		double qc[4];
		qinverse(m_fQuatRot, qc);
		rotvec(dst, qc, pos);
	}


	/*!
	 * オブジェクトローカル座標からグローバル座標への変換
	 * @param[out] dst グローバル座標
	 * @param[in]  src ローカル座標
	 */
	void CalGlobalPos(glm::vec3 &dst, const glm::vec3 &src)
	{
		// 回転
		rotvec(dst, m_fQuatRot, src);

		// 平行移動
		dst[0] += m_fTransDist[0];
		dst[1] += m_fTransDist[1];

		// スケーリング
		dst[2] += m_fScaleDist;
	}

	/*!
	 * 視点位置を算出(視点位置が原点(0,0,0)からの変換で求められていた場合)
	 * @param[out] pos 視点位置
	 */
	void GetViewPosition(glm::vec3 &pos)
	{
		glm::vec3 src(0.0);
		glm::vec3 dst;
		CalLocalPos(dst, src);
		pos[0] = dst[0]; pos[1] = dst[1]; pos[2] = dst[2];
	}

	/*!
	 * グローバル座標からオブジェクトローカル座標への変換(回転のみ)
	 * @param[out] dst ローカル座標
	 * @param[in]  src グローバル座標
	 */
	void CalLocalRot(glm::vec3 &dst, const glm::vec3 &src)
	{
		double qc[4];
		qinverse(m_fQuatRot, qc);
		rotvec(dst, qc, src);
	}

	/*!
	 * オブジェクトローカル座標からグローバル座標への変換(回転のみ)
	 * @param[out] dst グローバル座標
	 * @param[in]  src ローカル座標
	 */
	void CalGlobalRot(glm::vec3 &dst, const glm::vec3 &src)
	{
		rotvec(dst, m_fQuatRot, src);
	}

	/*!
	 * 視線方向を算出
	 * @param[out] dir 視線方向
	 */
	void GetViewDirection(glm::vec3 &dir)
	{
		glm::vec3 src(0.0f, 0.0f, -1.0f);
		glm::vec3 dst;
		CalLocalRot(dst, src);
		dir[0] = dst[0];
		dir[1] = dst[1];
		dir[2] = dst[2];
	}


	//-----------------------------------------------------------------------------
	// 描画時の指定
	//-----------------------------------------------------------------------------
	// それぞれOpenGLに回転の変換行列・スケーリング・平行移動を設定する関数
	void ApplyRotation(void) const { glMultMatrixd(m_fMatRot); }
	void ApplyScaling(void) const { glTranslated(0.0, 0.0, m_fScaleDist); }
	void ApplyTranslation(void) const { glTranslated(m_fTransDist[0], m_fTransDist[1], 0.0); }

	//! OpenGLに回転の変換行列を設定(四元数指定版)
	void ApplyQuaternion(const double q[4])
	{
		double m[16];
		q2m(m, q);
		glMultMatrixd(m);
	}

	//! OpenGLのオブジェクト変換
	void Apply(void)
	{
		ApplyScaling();
		ApplyTranslation();
		ApplyRotation();
	}


	//-----------------------------------------------------------------------------
	// パラメータ設定
	//-----------------------------------------------------------------------------
	//! スケーリング量(z)を設定
	void SetScaling(double z){ m_fScaleDist = z; }

	//! 平行移動量(x,y)を設定
	void SetTranslation(double x, double y){ m_fTransDist[0] = x; m_fTransDist[1] = y; }

	//! 回転量(rot:4x4行列)を設定
	void SetRotation(double rot[16])
	{
		for(int i = 0; i < 16; ++i){
			m_fMatRot[i] = rot[i];
			m2q(m_fQuatRot, m_fMatRot);	// 回転の変換行列からクォータニオンを求める
		}
	}


	/*
	 * 回転量を設定
	 * @param[in] q 四元数
	 */
	void SetQuaternion(double q[4])
	{
		// クォータニオンから回転の変換行列を求める
		q2m(m_fMatRot, q);

		// 回転の保存
		copyq(m_fQuatRot, q);
	}

	/*
	 * 回転量を設定
	 * @param[in] ang 回転量[deg]
	 * @param[in] x,y,z 回転軸
	 */
	void SetRotation(double ang, double x, double y, double z)
	{
		double a = sqrt(x*x+y*y+z*z);

		if(!RX_FEQ2(a, 0.0, RX_FEQ_EPS)){
			double ar = 0.5*RX_TO_RADIANS(ang);
			double as = sin(ar)/a;
			double dq[4] = { cos(ar), x*as, y*as, z*as };

			// クォータニオンから回転の変換行列を求める
			q2m(m_fMatRot, dq);

			// 回転の保存
			copyq(m_fQuatRot, dq);
		}
	}


	/*
	 * 回転を追加
	 * @param[in] ang 回転量[deg]
	 * @param[in] x,y,z 回転軸
	 */
	void AddRotation(double ang, double x, double y, double z)
	{
		double a = sqrt(x*x+y*y+z*z);

		if(!RX_FEQ2(a, 0.0, RX_FEQ_EPS)){
			double ar = 0.5*RX_TO_RADIANS(ang);
			double as = sin(ar)/a;
			double dq[4] = { cos(ar), x*as, y*as, z*as };

			m_fLastRot[0] = cos(ar);
			m_fLastRot[1] = x*as;
			m_fLastRot[2] = y*as;
			m_fLastRot[3] = 0.0;

			copyq(m_fQuatIncRot, dq);
			copyq(dq, m_fQuatRot);

			// クォータニオンを掛けて回転を合成
			mulqq(m_fQuatRot, m_fQuatIncRot, dq);

			// クォータニオンから回転の変換行列を求める
			q2m(m_fMatRot, m_fQuatRot);

		}
	}


	/*!
	 * 最後の回転情報を取り出す
	 */
	void GetLastRotation(double& ang, double& x, double& y, double& z)
	{
		ang = m_fLastRot[0];
		x = m_fLastRot[1];
		y = m_fLastRot[2];
		z = m_fLastRot[3];
	}




	/*!
	 * マウスクリックした位置への方向ベクトルを返す
	 * @param[in] x,y マウスクリック座標
	 * @param[in] fov 設定しているカメラの視野角[deg]
	 * @param[out] 
	 */
	void GetRayTo(int x, int y, double fov, glm::vec3 &ray_to)
	{
		glm::vec3 eye_pos, eye_dir, up_dir(0, 1, 0);
		glm::vec3 init_pos(0, 0, 0);
		glm::vec3 init_dir(0, 0, -1);
		glm::vec3 init_up(0, 1, 0);
		CalLocalPos(eye_pos, init_pos);
		CalLocalRot(eye_dir, init_dir);
		CalLocalRot(up_dir, init_up);

		eye_dir = glm::normalize(eye_dir);

		float far_d = 10.0f;
		eye_dir[0] *= far_d; eye_dir[1] *= far_d; eye_dir[2] *= far_d;

		up_dir = glm::normalize(up_dir);

		// 視線に垂直な平面の左右，上下方向
		glm::vec3 hor, ver;
		hor = glm::normalize(glm::cross(eye_dir, up_dir));
		ver = glm::normalize(glm::cross(hor, eye_dir));

		float tanfov = tan(0.5*RX_TO_RADIANS(fov));

		float d = 2.0f*far_d*tanfov;
		float aspect = m_fW/m_fH;
		hor[0] *= d*aspect; hor[1] *= d*aspect; hor[2] *= d*aspect;
		ver[0] *= d; ver[1] *= d; ver[2] *= d;

		// 描画平面の中心
		ray_to[0] = eye_pos[0]+eye_dir[0];
		ray_to[1] = eye_pos[1]+eye_dir[1];
		ray_to[2] = eye_pos[2]+eye_dir[2];

		// 中心から視点に垂直な平面の左右，上下方向にマウスの座標分移動させる
		float dx = (x-0.5*m_fW)/m_fW, dy = (0.5*m_fH-y)/m_fH;
		ray_to[0] += dx*hor[0]; ray_to[1] += dx*hor[1]; ray_to[2] += dx*hor[2];
		ray_to[0] += dy*ver[0]; ray_to[1] += dy*ver[1]; ray_to[2] += dy*ver[2];
	}



private:
	double m_fTransScale;		//!< マウスの相対位置→スケーリング，平行移動量の換算係数

	int m_iSx, m_iSy;	//!< ドラッグ開始位置
	int m_iPx, m_iPy;	//!< ドラッグ開始位置
	double m_fW, m_fH;	//!< マウスの絶対位置→ウィンドウ内での相対位置の換算係数

	int m_iVeloc[2];

	double m_fQuatRot[4];		//!< 回転(クォータニオン)
	double m_fQuatIncRot[4];	//!< ドラッグ中の回転増分量(クォータニオン)
	double m_fMatRot[16];		//!< 回転の変換行列
	
	double m_fTransDist[2];		//!< カメラパン
	double m_fScaleDist;		//!< スケーリング
	int m_iDrag;				//!< ドラッグ中か否か(1:左ドラッグ, 2:右ドラッグ, 3:ミドルドラッグ)

	double m_fLastRot[4];		//!< 最後の回転



private:
	//-----------------------------------------------------------------------------
	// ベクトル演算
	//-----------------------------------------------------------------------------
	//! 内積
	inline double dot(const double a[3], const double b[3])
	{
		return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
	}

	//! 外積
	inline void cross(const double a[3], const double b[3], double c[3])
	{
		c[0] = a[1]*b[2]-a[2]*b[1];
		c[1] = a[2]*b[0]-a[0]*b[2];
		c[2] = a[0]*b[1]-a[1]*b[0];
	}

	//! 正規化
	inline double normalize(double a[3])
	{
		double d = dot(a, a);
		if(d > 0.0){
			double l = sqrt(1.0/d);
			a[0] *= l;
			a[1] *= l;
			a[2] *= l;
		}
		return d;
	}


	//-----------------------------------------------------------------------------
	// 行列関数
	//-----------------------------------------------------------------------------
	/*!
	 * 行列とベクトルのかけ算(d = m x v)
	 * @param[out] d 結果のベクトル
	 * @param[in] m 4x4行列
	 * @param[in] v 4次元ベクトル
	 */
	inline void mulmv(double d[4], const double m[16], const double v[4])
	{
		d[0] = (v[0]*m[IDX(0, 0)]+v[1]*m[IDX(0, 1)]+v[2]*m[IDX(0, 2)]+v[3]*m[IDX(0, 3)]);
		d[1] = (v[0]*m[IDX(1, 0)]+v[1]*m[IDX(1, 1)]+v[2]*m[IDX(1, 2)]+v[3]*m[IDX(1, 3)]);
		d[2] = (v[0]*m[IDX(2, 0)]+v[1]*m[IDX(2, 1)]+v[2]*m[IDX(2, 2)]+v[3]*m[IDX(2, 3)]);
		d[3] = (v[0]*m[IDX(3, 0)]+v[1]*m[IDX(3, 1)]+v[2]*m[IDX(3, 2)]+v[3]*m[IDX(3, 3)]);
	}

	/*!
	 * ベクトルと行列のかけ算(d = v x m)
	 * @param[out] d 結果のベクトル
	 * @param[in] v 4次元ベクトル
	 * @param[in] m 4x4行列
	 */
	inline void mulvm(double d[4], const double v[4], const double m[16])
	{
		d[0] = (v[0]*m[IDX(0, 0)]+v[1]*m[IDX(1, 0)]+v[2]*m[IDX(2, 0)]+v[3]*m[IDX(3, 0)]);
		d[1] = (v[0]*m[IDX(0, 1)]+v[1]*m[IDX(1, 1)]+v[2]*m[IDX(2, 1)]+v[3]*m[IDX(3, 1)]);
		d[2] = (v[0]*m[IDX(0, 2)]+v[1]*m[IDX(1, 2)]+v[2]*m[IDX(2, 2)]+v[3]*m[IDX(3, 2)]);
		d[3] = (v[0]*m[IDX(0, 3)]+v[1]*m[IDX(1, 3)]+v[2]*m[IDX(2, 3)]+v[3]*m[IDX(3, 3)]);
	}

	/*!
	 * 単位行列生成
	 * @param[out] m 単位行列
	 */
	inline void identity(double m[16])
	{
		for(int i = 0; i < 4; ++i){
			for(int j = 0; j < 4; ++j){
				if(i == j){
					m[IDX(i, j)] = 1.0;
				} else{
					m[IDX(i, j)] = 0.0;
				}
			}
		}
	}

	/*!
	 * 行列コピー
	 * @param[out] src コピー元行列
	 * @param[out] dst コピー先行列
	 */
	inline void copym(const double src[16], double dst[16])
	{
		for(int i = 0; i < 4; ++i){
			for(int j = 0; j < 4; ++j){
				dst[IDX(i, j)] = src[IDX(i, j)];
			}
		}
	}

	/*!
	 * 逆行列(4x4)計算
	 * @param[out] inv_m 逆行列
	 * @param[in] m 元の行列
	 */
	inline void inverse(double inv_m[16], const double m[16])
	{
		identity(inv_m);

		double r1[8], r2[8], r3[8], r4[8];
		double* s[4], * tmprow;

		s[0] = &r1[0];
		s[1] = &r2[0];
		s[2] = &r3[0];
		s[3] = &r4[0];

		int i, j, p, jj;
		for(i = 0; i < 4; ++i){
			for(j = 0; j < 4; ++j){
				s[i][j] = m[IDX(i, j)];

				if(i==j){
					s[i][j+4] = 1.0;
				} else{
					s[i][j+4] = 0.0;
				}
			}
		}

		double scp[4];
		for(i = 0; i < 4; ++i){
			scp[i] = double(fabs(s[i][0]));
			for(j = 1; j < 4; ++j)
				if(double(fabs(s[i][j])) > scp[i]) scp[i] = double(fabs(s[i][j]));
			if(scp[i] == 0.0) return; // singular matrix!
		}

		int pivot_to;
		double scp_max;
		for(i = 0; i < 4; ++i){
			// select pivot row
			pivot_to = i;
			scp_max = double(fabs(s[i][i]/scp[i]));
			// find out which row should be on top
			for(p = i+1; p < 4; ++p)
				if(double(fabs(s[p][i]/scp[p])) > scp_max){
					scp_max = double(fabs(s[p][i]/scp[p])); pivot_to = p;
				}
			// Pivot if necessary
			if(pivot_to != i)
			{
				tmprow = s[i];
				s[i] = s[pivot_to];
				s[pivot_to] = tmprow;
				double tmpscp;
				tmpscp = scp[i];
				scp[i] = scp[pivot_to];
				scp[pivot_to] = tmpscp;
			}

			double mji;
			// perform gaussian elimination
			for(j = i+1; j < 4; ++j)
			{
				mji = s[j][i]/s[i][i];
				s[j][i] = 0.0;
				for(jj = i+1; jj<8; jj++)
					s[j][jj] -= mji*s[i][jj];
			}
		}

		if(s[3][3] == 0.0) return; // singular matrix!

		//
		// Now we have an upper triangular matrix.
		//
		//  x x x x | y y y y
		//  0 x x x | y y y y 
		//  0 0 x x | y y y y
		//  0 0 0 x | y y y y
		//
		//  we'll back substitute to get the inverse
		//
		//  1 0 0 0 | z z z z
		//  0 1 0 0 | z z z z
		//  0 0 1 0 | z z z z
		//  0 0 0 1 | z z z z 
		//

		double mij;
		for(i = 3; i > 0; --i){
			for(j = i-1; j > -1; --j){
				mij = s[j][i]/s[i][i];
				for(jj = j+1; jj < 8; ++jj){
					s[j][jj] -= mij*s[i][jj];
				}
			}
		}

		for(i = 0; i < 4; ++i){
			for(j = 0; j < 4; ++j){
				inv_m[IDX(i, j)] = s[i][j+4]/s[i][i];
			}
		}
	}


	//-----------------------------------------------------------------------------
	// 四元数関数
	//-----------------------------------------------------------------------------
	/*!
	 * 四元数のかけ算(r = p x q)の計算
	 * @param[out] r 積の結果
	 * @param[in] p,q 元の四元数
	 */
	inline void mulqq(double r[], const double p[], const double q[])
	{
		r[0] = p[0]*q[0] - p[1]*q[1] - p[2]*q[2] - p[3]*q[3];
		r[1] = p[0]*q[1] + p[1]*q[0] + p[2]*q[3] - p[3]*q[2];
		r[2] = p[0]*q[2] - p[1]*q[3] + p[2]*q[0] + p[3]*q[1];
		r[3] = p[0]*q[3] + p[1]*q[2] - p[2]*q[1] + p[3]*q[0];
	}
	/*!
	 * 四元数の初期化
	 * @param[inout] r 初期化する四元数
	 */
	inline void initq(double r[])
	{
		r[0] = 1.0;
		r[1] = 0.0;
		r[2] = 0.0;
		r[3] = 0.0;
	}
	/*!
	 * 四元数のコピー(r = p)
	 * @param[out] r コピー先の四元数
	 * @param[in]  p コピー元の四元数
	 */
	inline void copyq(double r[], const double p[])
	{
		r[0] = p[0];
		r[1] = p[1];
		r[2] = p[2];
		r[3] = p[3];
	}

	/*
	 * 四元数 -> 回転変換行列
	 * @param[out] r 回転変換行列
	 * @param[in] q 四元数
	 */
	inline void q2m(double r[], const double q[])
	{
		double x2 = q[1]*q[1]*2.0;
		double y2 = q[2]*q[2]*2.0;
		double z2 = q[3]*q[3]*2.0;
		double xy = q[1]*q[2]*2.0;
		double yz = q[2]*q[3]*2.0;
		double zx = q[3]*q[1]*2.0;
		double xw = q[1]*q[0]*2.0;
		double yw = q[2]*q[0]*2.0;
		double zw = q[3]*q[0]*2.0;

		r[0] = 1.0 - y2 - z2;
		r[1] = xy + zw;
		r[2] = zx - yw;
		r[4] = xy - zw;
		r[5] = 1.0 - z2 - x2;
		r[6] = yz + xw;
		r[8] = zx + yw;
		r[9] = yz - xw;
		r[10] = 1.0 - x2 - y2;
		r[3] = r[7] = r[11] = r[12] = r[13] = r[14] = 0.0;
		r[15] = 1.0;
	}
	/*
	 * 回転変換行列 -> 四元数
	 * @param[in] q 四元数
	 * @param[out] r 回転変換行列(4x4)
	 */
	inline void m2q(double q[], double r[])
	{
		double tr, s;
		const int nxt[3] = { 1, 2, 0 };

		tr = r[0]+r[5]+r[10];

		if(tr > 0.0){
			s = sqrt(tr+r[15]);
			q[3] = s*0.5;
			s = 0.5/s;

			q[0] = (r[6]-r[9])*s;
			q[1] = (r[8]-r[2])*s;
			q[2] = (r[1]-r[4])*s;
		} else{
			int i, j, k;
			i = 0;
			if(r[5] > r[0])
				i = 1;

			if(r[10] > r[IDX(i, i)])
				i = 2;

			j = nxt[i];
			k = nxt[j];

			s = sqrt((r[IDX(i, j)]-(r[IDX(j, j)]+r[IDX(k, k)]))+1.0);

			q[i] = s*0.5;
			s = 0.5/s;

			q[3] = (r[IDX(j, k)]-r[IDX(k, j)])*s;
			q[j] = (r[IDX(i, j)]+r[IDX(j, i)])*s;
			q[k] = (r[IDX(i, k)]+r[IDX(k, i)])*s;
		}
	}

	/*
	 * 球面線形補間
	 * @param[out] p 補間結果の四元数
	 * @param[in] q0,q1 補間対象2ベクトル
	 * @param[in] t パラメータ
	 */
	inline void qlerp(double p[], const double q0[], const double q1[], const double t)
	{
		double qr = q0[0]*q1[0]+q0[1]*q1[1]+q0[2]*q1[2]+q0[3]*q1[3];
		double ss = 1.0-qr*qr;

		if(ss == 0.0){
			p[0] = q0[0];
			p[1] = q0[1];
			p[2] = q0[2];
			p[3] = q0[3];
		} else{
			double sp = sqrt(ss);
			double ph = acos(qr);	// ベクトル間の角度θ
			double pt = ph*t;		// θt
			double t1 = sin(pt)/sp;
			double t0 = sin(ph-pt)/sp;

			p[0] = q0[0]*t0+q1[0]*t1;
			p[1] = q0[1]*t0+q1[1]*t1;
			p[2] = q0[2]*t0+q1[2]*t1;
			p[3] = q0[3]*t0+q1[3]*t1;
		}
	}

	/*
	 * 複数のクォータニオン間の球面線形補間（折れ線）
	 *   p ← t[i] におけるクォータニオン q[i], 0 <= i < n に対する
	 *        u における補間値
	*/
	inline void qlerp_multi(double p[], const double t[], const double q[][4], const int n, const double u)
	{
		int i = 0, j = n-1;

		// u を含む t の区間 [t[i], t[i+1]) を二分法で求める
		while(i < j){
			int k = (i+j)/2;
			if(t[k] < u){
				i = k+1;
			} else{
				j = k;
			}
		}
		if(i > 0){
			--i;
		}

		qlerp(p, q[i], q[i+1], (u-t[i])/(t[i+1]-t[i]));
	}

	/*!
	 * 回転軸(ax,ay,az)と角度(ang[deg])から四元数(q)を計算
	 */
	inline void makeq(double q[], const double ang, const double ax, const double ay, const double az)
	{
		double as = ax*ax+ay*ay+az*az;
		if(as < RX_FEQ_EPS){
			q[0] = 1.0; q[1] = 0.0; q[2] = 0.0; q[3] = 0.0;
		} else{
			as = sqrt(as);
			double rd = 0.5*RX_TO_RADIANS(ang);
			double st = sin(rd);
			q[0] = cos(rd);
			q[1] = ax*st/as; q[2] = ay*st/as; q[3] = az*st/as;
		}

	}


	/*!
	 * 四元数によるベクトルの回転
	 * @param[out] dst 回転後のベクトル
	 * @param[in] q 四元数
	 * @param[in] src ベクトル
	 */
	inline void rotvec(glm::vec3 &dst, const double q[], const glm::vec3 src)
	{
		double v_coef = q[0]*q[0]-q[1]*q[1]-q[2]*q[2]-q[3]*q[3];
		double u_coef = 2.0*(src[0]*q[1]+src[1]*q[2]+src[2]*q[3]);
		double c_coef = 2.0*q[0];

		dst[0] = v_coef*src[0]+u_coef*q[1]+c_coef*(q[2]*src[2]-q[3]*src[1]);
		dst[1] = v_coef*src[1]+u_coef*q[2]+c_coef*(q[3]*src[0]-q[1]*src[2]);
		dst[2] = v_coef*src[2]+u_coef*q[3]+c_coef*(q[1]*src[1]-q[2]*src[0]);

	}

	/*!
	 * 共役四元数
	 * @param[in] q 元の四元数
	 * @param[out] qc 共役四元数
	 */
	inline void qinverse(const double q[], double qc[])
	{
		qc[0] = q[0];
		qc[1] = -q[1];
		qc[2] = -q[2];
		qc[3] = -q[3];
	}



	/*!
	 * マウスポインタの変位量から回転を計算
	 * @param[in] vx,vy マウスポインタの変位量
	 */
	void trackballRot(double vx, double vy)
	{
		if(m_fW < RX_FEQ_EPS || m_fH < RX_FEQ_EPS) return;
		double dx = vx/m_fW;
		double dy = vy/m_fH;
		double a = sqrt(dx*dx+dy*dy);

		// 回転処理
		if(a != 0.0){
			double ar = a*TB_ROT_SCALE*0.5;
			double as = sin(ar)/a;
			double dq[4] = { cos(ar), dy*as, dx*as, 0.0 };

			m_fLastRot[0] = cos(ar);
			m_fLastRot[1] = dy*as;
			m_fLastRot[2] = dx*as;
			m_fLastRot[3] = 0.0;

			copyq(m_fQuatIncRot, dq);
			copyq(dq, m_fQuatRot);

			// クォータニオンを掛けて回転を合成
			mulqq(m_fQuatRot, m_fQuatIncRot, dq);

			// クォータニオンから回転の変換行列を求める
			q2m(m_fMatRot, m_fQuatRot);
		} else{
			initq(m_fQuatIncRot);
		}
	}
	void trackballLastRot(void)
	{
		double dq[4];
		copyq(dq, m_fQuatRot);

		// クォータニオンを掛けて回転を合成
		mulqq(m_fQuatRot, m_fQuatIncRot, dq);

		// クォータニオンから回転の変換行列を求める
		q2m(m_fMatRot, m_fQuatRot);
	}



};






#endif // #ifndef _RX_TRACKBALL_H_