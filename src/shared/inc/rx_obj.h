/*!
  @file rx_obj.h
	
  @brief OBJ/MTL File Input/Output
 
  @author Makoto Fujisawa
  @date   2011
*/

#ifndef _RX_OBJ_H_
#define _RX_OBJ_H_


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include "rx_mesh.h"

//-----------------------------------------------------------------------------
// Name Space
//-----------------------------------------------------------------------------
using namespace std;



//-----------------------------------------------------------------------------
// rxOBJクラスの宣言 - OBJ形式の読み込み
//-----------------------------------------------------------------------------
class rxOBJ
{
	rxMTL m_mapMaterials;	//!< ラベルとデータのマップ
	string m_strCurrentMat;	//!< 現在のデータを示すラベル

	//vector<rxMaterialOBJ> m_vMaterials;
	//int m_iCurrentMat;

public:
	//! コンストラクタ
	rxOBJ() : m_strCurrentMat("") {}

	//! デストラクタ
	~rxOBJ(){}

	/*!
	 * OBJファイル読み込み
	 * @param[in] file_name ファイル名(フルパス)
	 * @param[out] vrts 頂点座標
	 * @param[out] vnms 頂点法線
	 * @param[out] poly ポリゴン
	 * @param[out] mats 材質情報
	 * @param[in] triangle ポリゴンの三角形分割フラグ
	 */
	bool Read(string file_name, vector<glm::vec3> &vrts, vector<glm::vec3> &vnms, vector<rxFace> &plys, rxMTL &mats, bool triangle = true);

	/*!
	 * OBJファイル書き込み
	 * @param[in] file_name ファイル名(フルパス)
	 * @param[in] vrts 頂点座標
	 * @param[in] vnms 頂点法線
	 * @param[in] plys ポリゴン
	 * @param[in] mats 材質情報
	 */
	bool Save(string file_name, const vector<glm::vec3> &vrts, const vector<glm::vec3> &vnms, const vector<rxFace> &plys, const rxMTL &mats);

	//! 材質リストの取得
	rxMTL GetMaterials(void){ return m_mapMaterials; }

private:
	int loadFace(string &buf, vector<int> &vidxs, vector<int> &nidxs, vector<int> &tidxs);
	int loadMTL(const string &mtl_fn);
	int saveMTL(const string &mtl_fn, const rxMTL &mats);
};


/*!
* OBJファイル読み込み
* @param[in] file_name ファイル名(フルパス)
* @param[out] vrts 頂点座標
* @param[out] vnms 頂点法線
* @param[out] poly ポリゴン
* @param[out] mats 材質情報
* @param[in] triangle ポリゴンの三角形分割フラグ
*/
inline bool rxOBJ::Read(string file_name, 
						vector<glm::vec3> &vrts, 
						vector<glm::vec3> &vnms, 
						vector<rxFace> &plys, 
						rxMTL &mats, bool triangle)
{
	ifstream file;

	file.open(file_name.c_str());
	if(!file || !file.is_open() || file.bad() || file.fail()){
		cout << "rxOBJ::Read : Invalid file specified" << endl;
		return false;
	}

	vector<glm::vec3> vnormals;
	vector<glm::vec2> vtexcoords;

	string buf;
	string::size_type comment_start = 0;
	while(!file.eof()){
		getline(file, buf);

		// '#'以降はコメントとして無視
		if( (comment_start = buf.find('#')) != string::size_type(-1) )
			buf = buf.substr(0, comment_start);

		// 行頭のスペース，タブを削除
		DeleteHeadSpace(buf);

		// 空行は無視
		if(buf.empty())
			continue;

		if(buf[0] == 'v'){
			if(buf[1] == 'n'){		// 頂点法線
				glm::vec3 n;
				if(StringToVec3s(buf, "vn", n)){
					vnormals.push_back(n);
				}
#if RX_DEBUG_OUT
				cout << "vn " << n << endl;
#endif
			}
			else if(buf[1] == 't'){	// テクスチャ座標
				glm::vec2 tc;
				if(StringToVec2s(buf, "vt", tc)){
					vtexcoords.push_back(tc);
				}
#if RX_DEBUG_OUT
				cout << "vt " << tc << endl;
#endif
			}
			else{					// 頂点座標
				glm::vec3 v;
				if(StringToVec3s(buf, "v", v)){
					vrts.push_back(v);
					vnms.push_back(glm::vec3(0.0));
				}
#if RX_DEBUG_OUT
				cout << "v " << v << endl;
#endif
			}
		}
		else if(buf[0] == 'f'){		// 面
			int num_face = 0;
			vector<int> vidxs, nidxs, tidxs;
			if(!(num_face = loadFace(buf, vidxs, nidxs, tidxs))) continue;

#if RX_DEBUG_OUT
			cout << "f ";
			for(int i = 0; i < (int)vidxs.size(); ++i){
				cout << vidxs[i]+1 << " ";
			}
			cout << endl;
#endif

			if(triangle && num_face >= 4){
				PolyToTri(plys, vidxs, tidxs, vtexcoords, m_strCurrentMat);
			}
			else{
				rxFace face;
				face.vert_idx.resize(num_face);;
				face.material_name = m_strCurrentMat;
				face.texcoords.resize(num_face);
				bool tc = !vtexcoords.empty();
				for(int i = 0; i < num_face; ++i){
					face[i] = vidxs[i];
					if(tc){
						face.texcoords[i] = vtexcoords[tidxs[i]];
					}
					else{
						face.texcoords[i] = glm::vec2(0.0);
					}
				}
				plys.push_back(face);
			}

			// 頂点法線
			if(!vnormals.empty()){
				for(int i = 0; i < num_face; ++i){
					vnms[vidxs[i]] += vnormals[nidxs[i]];
				}
			}
		}
		else if(buf[0] == 'u'){		// 材質名
			string mat_name;
			if(!StringToString(buf, "usemtl", mat_name)) continue;

			if(mat_name == "(null)" || m_mapMaterials.empty()) continue;

			map<string, rxMaterialOBJ>::iterator i = m_mapMaterials.find(mat_name);

			if(i != m_mapMaterials.end()){
				m_strCurrentMat = mat_name;
			}

			//cout << "usemtl " << mat_name << endl;
		}
		else if(buf[0] == 'm'){		// 材質情報ファイル
			string mat_file_name;
			if(!StringToString(buf, "mtllib", mat_file_name)) continue;

			string dir = ExtractDirPath(file_name);
			if(!dir.empty()){
				mat_file_name = dir+"\\"+mat_file_name;
			}
			//cout << "mtllib " << mat_file_name << endl;

			if(loadMTL(mat_file_name) == 0){
				//cout << "fail to read " << mat_file_name << endl;
				continue;
			}

		}
	}

	if(vnormals.empty()){
		vnms.clear();
	}

	if(!vnms.empty()){
		for(int i = 0; i < (int)vnms.size(); ++i){
			vnms[i] = glm::normalize(vnms[i]);
		}
	}

	mats = m_mapMaterials;

	file.close();

	return true;
}


/*!
* OBJファイル書き込み
* @param[in] file_name ファイル名(フルパス)
* @param[in] vrts 頂点座標
* @param[in] vnms 頂点法線
* @param[in] plys ポリゴン
* @param[in] mats 材質情報
*/
inline bool rxOBJ::Save(string file_name, 
						const vector<glm::vec3> &vrts, 
						const vector<glm::vec3> &vnms, 
						const vector<rxFace> &plys, 
						const rxMTL &mats)
{
	if((int)plys.size() == 0) return false;

	ofstream file;

	file.open(file_name.c_str());
	if(!file || !file.is_open() || file.bad() || file.fail()){
		cout << "rxOBJ::Save : Invalid file specified" << endl;
		return false;
	}

	//
	// 材質(*.mtl)
	//
	if(!mats.empty()){
		string mtl_file_name = ExtractPathWithoutExt(file_name)+".mtl";

		if(saveMTL(mtl_file_name, mats)){
			file << "mtllib " << mtl_file_name << endl;
		}
	}

	//
	// ジオメトリ(*.obj)
	//
	// 頂点(v)
	int nv = (int)vrts.size();
	for(int i = 0; i < nv; ++i){
		file << "v " << Vec3ToString(glm::vec3(vrts[i])) << endl;
	}

	// 法線(vn)
	int nn = (int)vnms.size();
	for(int i = 0; i < nn; ++i){
		file << "vn " << Vec3ToString(glm::vec3(vnms[i])) << endl;
	}

	// テクスチャ座標(vt)

	// 面(f)
	int np = (int)plys.size();
	string mat_name = "";
	for(int i = 0; i < np; ++i){
		if(plys[i].material_name != "" && plys[i].material_name != mat_name){
			mat_name = plys[i].material_name;
			file << "usemtl " << mat_name << endl;
		}

		file << "f ";
		int n = (int)plys[i].size();

		if(vnms.empty()){	// v
			for(int j = 0; j < n; ++j){
				int vidx = plys[i][j]+1;
				file << vidx;
				file << ((j == n-1) ? "" : " ");
			}
		}
		else{				// v//vn
			for(int j = 0; j < n; ++j){
				int vidx = plys[i][j]+1;
				file << vidx << "//" << vidx << " ";//((j == n-1) ? "" : " ");
			}
		}
		file << endl;
	}

	file.close();

	return true;
}


/*!
* 面を表す("f"から始まる)行から面情報を抽出
* @param[in] buf 行文字列
* @param[out] vidxs 頂点インデックス
* @param[out] nidxs 法線インデックス
* @param[out] tidxs テクスチャ座標インデックス
* @return 頂点数
*/
inline int rxOBJ::loadFace(string &buf, vector<int> &vidxs, vector<int> &nidxs, vector<int> &tidxs)
{
	int num_face = CountString(buf, 2, " ")+1;

	if(num_face >= 3){
		vidxs.resize(num_face);
		nidxs.resize(num_face);
		tidxs.resize(num_face);

		int vidx = 0;	// 頂点インデックス
		int tidx = 0;	// テクスチャ座標インデックス
		int nidx = 0;	// 法線インデックス
		int offset = 2;
		for(int i = 0; i < num_face; ++i){
			if(sscanf(&buf[0]+offset, "%d/%d/%d", &vidx, &tidx, &nidx) != 3){
				if(sscanf(&buf[0]+offset, "%d//%d", &vidx, &nidx) != 2){
					if(sscanf(&buf[0]+offset, "%d/%d", &vidx, &tidx) != 2){
						sscanf(&buf[0]+offset, "%d", &vidx);	// 頂点座標のみ
						tidx = 0;
						nidx = 0;
						offset += (int)log10((double)vidx)+1;
					}
					else{	// 頂点座標，テクスチャ座標 (*//*)
						nidx = 0;
						offset += (int)log10((double)vidx)+(int)log10((double)tidx)+3;
					}
				}
				else{	// 頂点座標，法線 (*/*)
					tidx = 0;
					offset += (int)log10((double)vidx)+(int)log10((double)nidx)+4;
				}
			}
			else{ // 頂点座標，テクスチャ座標，法線 (*/*/*)
				offset += (int)log10((double)vidx)+(int)log10((double)tidx)+1+(int)log10((double)nidx)+4;
			}

			offset++;	// スペース

			if(vidx >= 1){
				vidxs[i]   = vidx-1;
				nidxs[i] = nidx-1;
				tidxs[i]  = tidx-1;
			}
			else{
				return 0;
			}
		}
	}

	return num_face;
}


/*!
* 材質情報ファイル(*.mtl)の読み込み
* @param[in] mtl_fn ファイル名
* @return 材質の数
*/
inline int rxOBJ::loadMTL(const string &mtl_fn)
{
	ifstream file;

	file.open(mtl_fn.c_str());
	if(!file || !file.is_open() || file.bad() || file.fail()){
		cout << "rxOBJ::loadMTL : Invalid file specified" << endl;
		return false;
	}

	int num_mat = 0;
	rxMaterialOBJ *cur_mat;

	string buf;
	string::size_type comment_start = 0;
	while(!file.eof()){
		getline(file, buf);

		// '#'以降はコメントとして無視
		if( (comment_start = buf.find('#')) != string::size_type(-1) )
			buf = buf.substr(0, comment_start);

		// 行頭のスペース，タブを削除
		DeleteHeadSpace(buf);

		// 空行は無視
		if(buf.empty())
			continue;

		if(buf[0] == 'n'){
			string mat_name;
			if(!StringToString(buf, "newmtl", mat_name)) continue;

			rxMaterialOBJ mat;
			mat.name      = mat_name;
			mat.diffuse   = glm::vec4(0.0);
			mat.specular  = glm::vec4(0.0);
			mat.ambient   = glm::vec4(0.0);
			mat.color     = glm::vec4(0.0);
			mat.emission  = glm::vec4(0.0);
			mat.shininess = 0.0;
			mat.illum = 2;
			mat.tex_file = "";
			mat.tex_name = 0;

			m_mapMaterials[mat_name] = mat;
			cur_mat = &m_mapMaterials[mat_name];

			num_mat++;
		}
		else if(buf[0] == 'K'){
			glm::vec3 col;
			if(buf[1] == 'd'){		// 拡散反射成分
				if(StringToVec3s(buf, "Kd", col)){
					cur_mat->diffuse = glm::vec4(col, cur_mat->diffuse[3]);
				}
			}
			else if(buf[1] == 's'){	// 鏡面反射成分
				if(StringToVec3s(buf, "Ks", col)){
					cur_mat->specular = glm::vec4(col, cur_mat->specular[3]);
				}
			}
			else if(buf[1] == 'a'){	// 環境光反射成分
				if(StringToVec3s(buf, "Ka", col)){
					cur_mat->ambient = glm::vec4(col, cur_mat->ambient[3]);
				}
			}
		}
		else if(buf[0] == 'd'){	// アルファ値(透過率)
			double alpha;
			if(StringToDouble(buf, "d", alpha)){
				cur_mat->diffuse[3]  = alpha;
				cur_mat->specular[3] = alpha;
				cur_mat->ambient[3]  = alpha;
				cur_mat->color[3]    = alpha;
				cur_mat->emission[3] = alpha;
			}
		}
		else if(buf[0] == 'T' && buf[1] == 'r'){	// 発光色
			glm::vec3 emit;
			if(StringToVec3s(buf, "Tr", emit)){
				cur_mat->emission = glm::vec4(emit, cur_mat->emission[3]);
			}
		}
		else if(buf[0] == 'N' && buf[1] == 's'){	// Shininess
			double shine = 0.0;
			if(StringToDouble(buf, "Ns", shine)){
				cur_mat->shininess = shine;
			}
		}
		else if(buf[0] == 'i' && buf[1] == 'l' && buf[2] == 'l'){	// 証明モデル(1で鏡面反射無効, 2で有効)
			string illum;
			if(StringToString(buf, "illum", illum)){
				cur_mat->illum = atoi(illum.c_str());
			}
		}
		else if(buf[0] == 'm' && buf[1] == 'a' && buf[2] == 'p'){	// テクスチャ名
			string texfn;
			if(StringToString(buf, "map_Kd", texfn)){
				string dir = ExtractDirPath(mtl_fn);
				if(!dir.empty()){
					texfn = dir+"\\"+texfn;
				}

				cur_mat->tex_file = texfn;
			}
		}
	}

	if(!m_mapMaterials.empty()){
		map<string, rxMaterialOBJ>::iterator iter = m_mapMaterials.begin();
		for(; iter != m_mapMaterials.end(); ++iter){
			cout << "material : " << iter->first << endl;
			cout << "  diffuse   = " << glm::to_string(iter->second.diffuse) << endl;
			cout << "  specular  = " << glm::to_string(iter->second.specular) << endl;
			cout << "  ambient   = " << glm::to_string(iter->second.ambient) << endl;
			cout << "  color     = " << glm::to_string(iter->second.color) << endl;
			cout << "  emission  = " << glm::to_string(iter->second.emission) << endl;
			cout << "  shininess = " << iter->second.shininess << endl;
			cout << "  illum     = " << iter->second.illum << endl;
			cout << "  tex_file  = " << iter->second.tex_file << endl;
		}

		m_strCurrentMat = m_mapMaterials.begin()->first;
	}

	return num_mat;
}


/*!
* 材質情報ファイル(*.mtl)の書き込み
* @param[in] mtl_fn ファイル名
* @param[in] mats 材質情報
*/
inline int rxOBJ::saveMTL(const string &mtl_fn, const rxMTL &mats)
{
	ofstream file;

	file.open(mtl_fn.c_str(), ios::out);
	if(!file || !file.is_open() || file.bad() || file.fail()){
		cout << "rxOBJ::saveMTL : Invalid file specified" << endl;
		return 0;
	}

	if(mats.empty()) return 0;

	map<string, rxMaterialOBJ>::iterator iter = m_mapMaterials.begin();
	for(; iter != m_mapMaterials.end(); ++iter){
		file << "newmtl " << iter->first << endl;
		file << "Kd " << Vec3ToString(glm::vec3(iter->second.diffuse)) << endl;
		file << "Ks " << Vec3ToString(glm::vec3(iter->second.specular)) << endl;
		file << "Ka " << Vec3ToString(glm::vec3(iter->second.ambient)) << endl;
		file << "Tr " << Vec3ToString(glm::vec3(iter->second.emission)) << endl;
		file << "d " << iter->second.diffuse[3] << endl;
		file << "Ns " << iter->second.shininess << endl;
		file << "illum " << iter->second.illum << endl;
		file << "map_Kd " << iter->second.tex_file << endl;
		file << endl;
	}

	file.close();

	return 1;
}



#endif // _RX_OBJ_H_
