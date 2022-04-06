/*!
  @file rx_material.h
	
  @brief OpenGLライティング
 
  @author Makoto Fujisawa
  @date 2005
*/


#ifndef _RX_MATERIAL_H_
#define _RX_MATERIAL_H_

//-----------------------------------------------------------------------------
// インクルードファイル
//-----------------------------------------------------------------------------
#include "glm/glm.hpp"
#include <GLFW/glfw3.h>


using namespace std;

//-----------------------------------------------------------------------------
//! 材質データ
//-----------------------------------------------------------------------------
class rxMaterial
{
public:
	glm::vec4 m_vDiff, m_vSpec, m_vAmbi, m_vEmit;
	float m_fDiffS, m_fSpecS, m_fAmbiS, m_fEmitS;
	float m_fShin;

	glm::vec4 m_vColor;

	// 屈折，鏡面反射パラメータ
	float m_fEta;		//!< 屈折率
	float m_fBias;		//!< Fresnelバイアス
	float m_fPower;	//!< Fresnel指数
	float m_fScale;	//!< Fresnel倍率

	string m_strName;
	int m_iID;

	int m_iIllum;
	string m_strTexFile;
	unsigned int m_uTexName;

public:
	rxMaterial()
	{
		SetColor(glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0), glm::vec3(1.0), glm::vec3(0.0), 30.0);
		SetScale(1.0, 0.75, 0.2, 0.0);
		m_fEta = 0.9;
		m_fBias = 1.0;
		m_fPower = 0.98;
		m_fScale = 5.0;
	}

	void SetColor(const glm::vec4 &diff, const glm::vec4 &spec, const glm::vec4 &ambi, const glm::vec4 &emit, const float &shin)
	{
		m_vDiff = diff;
		m_vSpec = spec;
		m_vAmbi = ambi;
		m_vEmit = emit;
		m_fShin = shin;
	}

	void SetColor(const GLfloat diff[4], 
				  const GLfloat spec[4], 
				  const GLfloat ambi[4], 
				  const GLfloat emit[4], 
				  const GLfloat shine)
	{
		m_vDiff = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		m_vSpec = glm::vec4(spec[0], spec[1], spec[2], spec[3]);
		m_vAmbi = glm::vec4(ambi[0], ambi[1], ambi[2], ambi[3]);
		m_vEmit = glm::vec4(emit[0], emit[1], emit[2], emit[3]);
		m_fShin = shine;
	}

	void SetColor(const glm::vec3 &diff, const glm::vec3 &spec, const glm::vec3 &ambi, const glm::vec3 &emit, const float &shin)
	{
		m_vDiff = glm::vec4(diff, 1.0f);
		m_vSpec = glm::vec4(spec, 1.0f);
		m_vAmbi = glm::vec4(ambi, 1.0f);
		m_vEmit = glm::vec4(emit, 1.0f);
		m_fShin = shin;
	}

	void SetDiff(const glm::vec4 &col){ m_vDiff = col; }
	void SetSpec(const glm::vec4 &col){ m_vSpec = col; }
	void SetAmbi(const glm::vec4 &col){ m_vAmbi = col; }
	void SetEmit(const glm::vec4 &col){ m_vEmit = col; }
	void SetDiff3(const glm::vec3 &col){ m_vDiff = glm::vec4(col, 1.0f); }
	void SetSpec3(const glm::vec3 &col){ m_vSpec = glm::vec4(col, 1.0f); }
	void SetAmbi3(const glm::vec3 &col){ m_vAmbi = glm::vec4(col, 1.0f); }
	void SetEmit3(const glm::vec3 &col){ m_vEmit = glm::vec4(col, 1.0f); }

	void SetScale(const float &sdiff, const float &sspec, const float &sambi, const float &semit)
	{
		m_fDiffS = sdiff;
		m_fSpecS = sspec;
		m_fAmbiS = sambi;
		m_fEmitS = semit;
 	}

	void SetRefrac(const float &eta, const float &bias, const float &power, const float &scale)
	{
		m_fEta = eta;
		m_fBias = bias;
		m_fPower = power;
		m_fScale = scale;
 	}

	void SetGL(void)
	{
		GLfloat mat_diff[] = { m_fDiffS*m_vDiff[0], m_fDiffS*m_vDiff[1], m_fDiffS*m_vDiff[2], m_vDiff[3] };
		GLfloat mat_spec[] = { m_fSpecS*m_vSpec[0], m_fSpecS*m_vSpec[1], m_fSpecS*m_vSpec[2], m_vSpec[3] };
		GLfloat mat_ambi[] = { m_fAmbiS*m_vAmbi[0], m_fAmbiS*m_vAmbi[1], m_fAmbiS*m_vAmbi[2], m_vAmbi[3] };
		GLfloat mat_shin[] = { m_fShin };

		glMaterialfv(GL_FRONT, GL_DIFFUSE,  mat_diff);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
		glMaterialfv(GL_FRONT, GL_AMBIENT,  mat_ambi); 
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shin);

		glColor3fv(mat_diff);
	}

	void SetGL(const glm::vec4 &diff, const glm::vec4 &spec, const glm::vec4 &ambi)
	{
		GLfloat mat_diff[] = { m_fDiffS*diff[0], m_fDiffS*diff[1], m_fDiffS*diff[2], diff[3] };
		GLfloat mat_spec[] = { m_fSpecS*spec[0], m_fSpecS*spec[1], m_fSpecS*spec[2], spec[3] };
		GLfloat mat_ambi[] = { m_fAmbiS*ambi[0], m_fAmbiS*ambi[1], m_fAmbiS*ambi[2], ambi[3] };
		GLfloat mat_shin[] = { m_fShin };

		glMaterialfv(GL_FRONT, GL_DIFFUSE,  mat_diff);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
		glMaterialfv(GL_FRONT, GL_AMBIENT,  mat_ambi); 
		glMaterialfv(GL_FRONT, GL_SHININESS,mat_shin);

		glColor4fv(mat_diff);
	}

	// アクセスメソッド
	glm::vec4 GetDiff(void) const { return m_vDiff*m_fDiffS; }
	glm::vec4 GetSpec(void) const { return m_vSpec*m_fSpecS; }
	glm::vec4 GetAmbi(void) const { return m_vAmbi*m_fAmbiS; }
	glm::vec4 GetEmit(void) const { return m_vEmit*m_fEmitS; }
	glm::vec3 GetDiff3(void) const { return glm::vec3(m_vDiff[0], m_vDiff[1], m_vDiff[2])*m_fDiffS; }
	glm::vec3 GetSpec3(void) const { return glm::vec3(m_vSpec[0], m_vSpec[1], m_vSpec[2])*m_fSpecS; }
	glm::vec3 GetAmbi3(void) const { return glm::vec3(m_vAmbi[0], m_vAmbi[1], m_vAmbi[2])*m_fAmbiS; }
	glm::vec3 GetEmit3(void) const { return glm::vec3(m_vEmit[0], m_vEmit[1], m_vEmit[2])*m_fEmitS; }
	float GetShin(void) const { return m_fShin; }

	glm::vec3 GetReflec(const glm::vec3 &irr, const glm::vec3 &nrm)
	{
		float ref_coef = m_fBias+m_fScale*pow((1.0+glm::dot(irr, nrm)), m_fPower);
		ref_coef = glm::clamp(ref_coef, 0.0f, 1.0f);
		return ref_coef*GetSpec3();
	}

	glm::vec3 GetRefrac(const glm::vec3 &irr, const glm::vec3 &nrm)
	{
		float ref_coef = m_fBias+m_fScale*pow((1.0+glm::dot(irr, nrm)), m_fPower);
		ref_coef = glm::clamp(ref_coef, 0.0f, 1.0f);
		return (1.0f-ref_coef)*GetSpec3();
	}

	void Get(glm::vec3 &diff, glm::vec3 &spec, glm::vec3 &ambi, glm::vec3 &emit, float &shin)
	{
		diff = glm::vec3(m_vDiff[0], m_vDiff[1], m_vDiff[2]);
		spec = glm::vec3(m_vSpec[0], m_vSpec[1], m_vSpec[2]);
		ambi = glm::vec3(m_vAmbi[0], m_vAmbi[1], m_vAmbi[2]);
		emit = glm::vec3(m_vEmit[0], m_vEmit[1], m_vEmit[2]);
		shin = m_fShin;
	}
};

static rxMaterial g_matDefault;



#endif // #ifndef _RX_MATERIAL_H_
