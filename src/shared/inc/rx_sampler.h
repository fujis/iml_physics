/*! 
  @file rx_sampler.h
	
  @brief サンプル点生成
 
  @author Makoto Fujisawa
  @date 2013-06
*/

#ifndef _RX_SAMPLER_
#define _RX_SAMPLER_


//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
// STL
#include <vector>
#include <list>

#include "glm/glm.hpp"

using namespace std;


//-----------------------------------------------------------------------------
// 他のサンプラの親クラス
//-----------------------------------------------------------------------------
class rxSampler
{
protected:
	glm::vec2 m_v2MinPos, m_v2MaxPos;	//!< サンプリング範囲
	glm::vec2 m_v2Dim;					//!< サンプリング範囲の大きさ
	int m_iNumMax;						//!< 最大サンプリング数

	double m_fDmin;						//!< サンプリング点間の最小距離
	int m_iNumAround;					//!< 点周囲のサンプリング数

public:
	/*!
	 * コンストラクタ
	 * @param[in] minp,maxp 生成範囲
	 * @param[in] num_max 最大点数
	 */
	rxSampler(glm::vec2 minp, glm::vec2 maxp, int num_max)
	{
		m_v2MinPos = minp;
		m_v2MaxPos = maxp;
		m_v2Dim = maxp-minp;
		m_iNumMax = num_max;
	}

	//! デストラクタ
	virtual ~rxSampler()
	{
	}
	
	//! 点のサンプリング
	virtual void Generate(vector<glm::vec2> &points) = 0;

protected:
	/*!
	 * 座標値からその点が含まれるセル位置を返す
	 * @param[in] p 座標値
	 * @param[out] i,j セル位置
	 * @param[in] h グリッド幅
	 */
	void calGridPos(glm::vec2 p, int &i, int &j, double h)
	{
		glm::vec2 dp = (p-m_v2MinPos)/float(h);
		i = (int)(dp[0]);
		j = (int)(dp[1]);
	}

	/*!
	 * 中心点から一定距離内のランダム点を生成
	 * @param[in] cen 中心点座標
	 * @param[in] d_min 最小距離(2*d_minが最大)
	 * @return ランダム点
	 */
	glm::vec2 genRandomAroundPoint(glm::vec2 cen, double d_min)
	{
		// 追加する点までの距離(半径)と角度を乱数で設定
		const double PI = 3.14159265359;
		double rad = d_min*(1.0+glm::linearRand(0.0, 1.0));
		double ang = 2*PI*(glm::linearRand(0.0, 1.0));

		// cenから距離[d_min, 2*d_min]内の点
		glm::vec2 sc = glm::vec2(sin(ang), cos(ang));
		glm::vec2 new_p = cen+sc*float(rad);

		return new_p;
	}

	/*!
	 * 2次元での点xがmin_x,max_yで囲まれた矩形内にあるかどうかの判別
	 * @param
	 * @return
	 */
	bool inRange(const glm::vec2 &x, const glm::vec2 &min_x, const glm::vec2 &max_x)
	{
		if(x[0] < min_x[0]) return false;
		if(x[1] < min_x[1]) return false;
		if(x[0] > max_x[0]) return false;
		if(x[1] > max_x[1]) return false;
		return true;
	}


	inline int IDX(int i, int j, int nx) const { return i+j*nx; }

	//! 最大値判定(2値)
	template<class T> inline T MAX(const T &a, const T &b){ return ((a > b) ? a : b); }
	//! 最小値判定(2値)
	template<class T> inline T MIN(const T &a, const T &b){ return ((a < b) ? a : b); }
	//! 乱数
	inline int RAND(const int &_min, const int &_max){ return int((_max-_min)*(double(rand())/(1.0+RAND_MAX))+_min); }
};




//-----------------------------------------------------------------------------
// ランダムサンプラ
//  - 乱数を使ったサンプル点の生成
//-----------------------------------------------------------------------------
class rxRandomSampler : public rxSampler
{
protected:
	rxRandomSampler() : rxSampler(glm::vec2(0.0), glm::vec2(1.0), 100){}

public:
	/*!
	 * コンストラクタ
	 * @param[in] minp,maxp 生成範囲
	 * @param[in] num_max 最大点数
	 */
	rxRandomSampler(glm::vec2 minp, glm::vec2 maxp, int num_max) : rxSampler(minp, maxp, num_max)
	{
	}

	//! デストラクタ
	virtual ~rxRandomSampler(){}

	/*!
	 * サンプリング点の生成
	 * @param[out] points サンプリングされた点
	 */
	virtual void Generate(vector<glm::vec2> &points)
	{
		points.resize(m_iNumMax);
		for(int i = 0; i < m_iNumMax; ++i){
			points[i] = glm::linearRand(m_v2MaxPos, m_v2MinPos);
		}
	}
};


//-----------------------------------------------------------------------------
// Uniform Poisson Disk Sampling
//  - R. Bridson, "Fast Poisson Disk Sampling in Arbitrary Dimensions", SIGGRAPH2007 sketches. 
//  - http://devmag.org.za/2009/05/03/poisson-disk-sampling/
//-----------------------------------------------------------------------------
class rxUniformPoissonDiskSampler : public rxSampler
{
	// サンプル点を格納するグリッドセル情報
	struct rxPointGrid
	{
		glm::vec2 point;		//!< 格納された点座標
		int num;		//!< 格納された点の数(0 or 1)
		rxPointGrid() : num(0){}
	};

	double m_fH;					//!< グリッド幅
	int m_iNx, m_iNy;				//!< グリッド数

	vector<rxPointGrid> m_vGrid;	//!< 点を格納するグリッド

protected:
	rxUniformPoissonDiskSampler() : rxSampler(glm::vec2(0.0), glm::vec2(1.0), 100){}

public:
	/*!
	 * コンストラクタ
	 * @param[in] minp,maxp 生成範囲
	 * @param[in] num_max 最大点数
	 * @param[in] num_around 点周囲のサンプリング数
	 * @param[in] d_min サンプリング点間の最小距離
	 */
	rxUniformPoissonDiskSampler(glm::vec2 minp, glm::vec2 maxp, int num_max, int num_around, double d_min)
		: rxSampler(minp, maxp, num_max)
	{
		// 点を格納するグリッド
		// 格子幅をr/√dとすることで1点/グリッドになるようにする
		m_fH = d_min/sqrt(2.0);
		m_iNx = (int)(m_v2Dim[0]/m_fH)+1;
		m_iNy = (int)(m_v2Dim[1]/m_fH)+1;

		m_vGrid.resize(m_iNx*m_iNy);

		m_fDmin = d_min;
		m_iNumAround = num_around;
	}

	//! デストラクタ
	virtual ~rxUniformPoissonDiskSampler(){}

protected:
	/*!
	 * 最初のサンプリング点の追加
	 * @param[in] grid 点を格納するグリッド
	 * @param[out] active サンプリング位置決定用基準点
	 * @param[out] point サンプリング点
	 */	
	void addFirstPoint(list<glm::vec2> &active, vector<glm::vec2> &point)
	{
		// ランダムな位置に点を生成
		glm::vec2 p = glm::linearRand(m_v2MaxPos, m_v2MinPos);

		// 点を含むグリッド位置
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].point = p;
		m_vGrid[g].num = 1;

		active.push_back(p);
		point.push_back(p);
	}


	/*!
	 * サンプリング点の追加
	 * @param[in] grid 点を格納するグリッド
	 * @param[out] active サンプリング位置決定用基準点
	 * @param[out] point サンプリング点
	 */	
	void addPoint(list<glm::vec2> &active, vector<glm::vec2> &point, glm::vec2 p)
	{
		// 点を含むグリッド位置
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].point = p;
		m_vGrid[g].num = 1;

		active.push_back(p);
		//point.push_back(p);
	}

	/*!
	 * サンプリング点の追加
	 * @param[in] grid 点を格納するグリッド
	 * @param[out] active サンプリング位置決定用基準点
	 * @param[out] point サンプリング点
	 */	
	bool addNextPoint(list<glm::vec2> &active, vector<glm::vec2> &point, glm::vec2 p)
	{
		bool found = false;

		glm::vec2 q = genRandomAroundPoint(p, m_fDmin);

		if(inRange(q, m_v2MinPos, m_v2MaxPos)){
			int x, y;	// qが含まれるグリッドセル位置
			calGridPos(q, x, y, m_fH);

			bool close = false;

			// すでに追加された点の周囲に逐次的に点を追加していく
			// 周囲のグリッドを探索して，近すぎる点(距離がd_minより小さい)がないか調べる
			for(int i = MAX(0, x-2); i < MIN(m_iNx, x+3) && !close; ++i){
				for(int j = MAX(0, y-2); j < MIN(m_iNy, y+3) && !close; ++j){
					int g = IDX(i, j, m_iNx);
					if(m_vGrid[g].num){	// グリッドにすでに点が含まれている場合
						double dist = glm::length(m_vGrid[g].point-q);
						if(dist < m_fDmin){
							close = true;
						}
					}
				}
			}

			// 近すぎる点がなければ新しい点として追加
			if(!close){
				found = true;
				active.push_back(q);
				point.push_back(q);
				int g = IDX(x, y, m_iNx);
				m_vGrid[g].point = q;
				m_vGrid[g].num = 1;
			}

		}

		return found;
	}

public:
	/*!
	 * サンプリング
	 * @param[out] points サンプリングされた点
	 */
	virtual void Generate(vector<glm::vec2> &points)
	{
		// 点を格納するグリッドの初期化
		int gnum = m_iNx*m_iNy;
		for(int k = 0; k < gnum; ++k){
			m_vGrid[k].point = glm::vec2(0.0);
			m_vGrid[k].num = 0;
		}

		list<glm::vec2> active;
		list<glm::vec2>::const_iterator itr;

		if(points.empty()){
			// 最初の点の追加(ランダムな位置)
			addFirstPoint(active, points);
		}
		else{
			// 事前に追加された点
			for(vector<glm::vec2>::iterator i = points.begin(); i != points.end(); ++i){
				addPoint(active, points, *i);
			}
		}

		// これ以上点が追加できない or 最大点数 までサンプリング
		while((!active.empty()) && ((int)points.size() <= m_iNumMax)){
			// アクティブリストからランダムに1点を取り出す
			int idx = RAND(0, int(active.size()));
			itr = active.begin();
			for(int i = 0; i < idx; ++i) itr++;
			glm::vec2 p = *(itr);

			// 点pから[d_min,2*d_min]の位置でかつ他の点に近すぎない位置に新しい点を追加
			bool found = false;
			for(int i = 0; i < m_iNumAround; ++i){
				found |= addNextPoint(active, points, p);
			}

			// 点pの周りにこれ以上点を追加できない場合はactiveリストから削除
			if(!found){
				active.erase(itr);
			}
		}
	}
};


//-----------------------------------------------------------------------------
// Poisson Disk Sampling with a distribution function
//  - R. Bridson, "Fast Poisson Disk Sampling in Arbitrary Dimensions", SIGGRAPH2007 sketches. 
//  - http://devmag.org.za/2009/05/03/poisson-disk-sampling/
//  - 分布関数の値により部分的に点の密度を変える
//-----------------------------------------------------------------------------
class rxPoissonDiskSampler : public rxSampler
{
	// サンプル点を格納するグリッドセル情報
	struct rxPointGrid
	{
		vector<glm::vec2> points;		//!< 格納された点座標
		int num;		//!< 格納された点の数(0 or 1)
		rxPointGrid() : num(0){}
	};

	double m_fH;					//!< グリッド幅
	int m_iNx, m_iNy;				//!< グリッド数

	vector<rxPointGrid> m_vGrid;	//!< 点を格納するグリッド

	double (*m_fpDist)(glm::vec2);		//!< 分布関数

protected:
	rxPoissonDiskSampler() : rxSampler(glm::vec2(0.0), glm::vec2(1.0), 100){}

public:
	/*!
	 * コンストラクタ
	 * @param[in] minp,maxp 生成範囲
	 * @param[in] num_max 最大点数
	 * @param[in] num_around 点周囲のサンプリング数
	 * @param[in] d_min サンプリング点間の最小距離
	 * @param[in] dist 分布関数
	 */
	rxPoissonDiskSampler(glm::vec2 minp, glm::vec2 maxp, int num_max, int num_around, double d_min, double (*dist)(glm::vec2))
		: rxSampler(minp, maxp, num_max)
	{
		// 点を格納するグリッド
		// 格子幅をr/√dとすることで1点/グリッドになるようにする
		m_fH = d_min/sqrt(2.0);
		m_iNx = (int)(m_v2Dim[0]/m_fH)+1;
		m_iNy = (int)(m_v2Dim[1]/m_fH)+1;

		m_vGrid.resize(m_iNx*m_iNy);

		m_fDmin = d_min;
		m_iNumAround = num_around;

		m_fpDist = dist;
	}

	//! デストラクタ
	virtual ~rxPoissonDiskSampler(){}

protected:
	/*!
	 * 最初のサンプリング点の追加
	 * @param[in] grid 点を格納するグリッド
	 * @param[out] active サンプリング位置決定用基準点
	 * @param[out] point サンプリング点
	 */	
	void addFirstPoint(list<glm::vec2> &active, vector<glm::vec2> &point)
	{
		// ランダムな位置に点を生成
		glm::vec2 p = glm::linearRand(m_v2MaxPos, m_v2MinPos);

		// 点を含むグリッド位置
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].points.push_back(p);
		m_vGrid[g].num++;

		active.push_back(p);
		point.push_back(p);
	}

	/*!
	 * サンプリング点の追加
	 * @param[in] grid 点を格納するグリッド
	 * @param[out] active サンプリング位置決定用基準点
	 * @param[out] point サンプリング点
	 */	
	bool addNextPoint(list<glm::vec2> &active, vector<glm::vec2> &point, glm::vec2 p)
	{
		bool found = false;

		double fraction = m_fpDist(p);
		glm::vec2 q = genRandomAroundPoint(p, m_fDmin);

		if(inRange(q, m_v2MinPos, m_v2MaxPos)){
			int x, y;	// qが含まれるグリッドセル位置
			calGridPos(q, x, y, m_fH);

			bool close = false;

			// すでに追加された点の周囲に逐次的に点を追加していく
			// 周囲のグリッドを探索して，近すぎる点(距離がd_minより小さい)がないか調べる
			for(int i = MAX(0, x-2); i < MIN(m_iNx, x+3) && !close; ++i){
				for(int j = MAX(0, y-2); j < MIN(m_iNy, y+3) && !close; ++j){
					int g = IDX(i, j, m_iNx);

					// グリッド内の点を調べていく
					for(int l = 0; l < m_vGrid[g].num; ++l){
						double dist = glm::length(m_vGrid[g].points[l]-q);
						if(dist < m_fDmin*fraction){
							close = true;
						}
					}
				}
			}

			// 近すぎる点がなければ新しい点として追加
			if(!close){
				found = true;
				active.push_back(q);
				point.push_back(q);
				int g = IDX(x, y, m_iNx);
				m_vGrid[g].points.push_back(q);
				m_vGrid[g].num++;
			}

		}

		return found;
	}

public:
	/*!
	 * サンプリング
	 * @param[out] points サンプリングされた点
	 */
	virtual void Generate(vector<glm::vec2> &points)
	{
		// 点を格納するグリッドの初期化
		int gnum = m_iNx*m_iNy;
		for(int k = 0; k < gnum; ++k){
			m_vGrid[k].points.clear();
			m_vGrid[k].num = 0;
		}

		list<glm::vec2> active;
		list<glm::vec2>::const_iterator itr;

		// 最初の点の追加(ランダムな位置)
		addFirstPoint(active, points);

		// これ以上点が追加できない or 最大点数 までサンプリング
		while((!active.empty()) && ((int)points.size() <= m_iNumMax)){
			// アクティブリストからランダムに1点を取り出す
			int idx = RAND(0, int(active.size()));
			itr = active.begin();
			for(int i = 0; i < idx; ++i) itr++;
			glm::vec2 p = *(itr);

			// 点pから[d_min,2*d_min]の位置でかつ他の点に近すぎない位置に新しい点を追加
			bool found = false;
			for(int i = 0; i < m_iNumAround; ++i){
				found |= addNextPoint(active, points, p);
			}

			// 点pの周りにこれ以上点を追加できない場合はactiveリストから削除
			if(!found){
				active.erase(itr);
			}
		}
	}
};

#endif // _RX_SAMPLER_