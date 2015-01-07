#ifndef SKYDOME_H
#define SKYDOME_H

#include "d3dUtil.h"

class Sky
{
public:
	Sky();
	~Sky();

	void init(ID3D10Device* device, ID3D10ShaderResourceView* cubemap, float radius);
	HRESULT loadLUTS(char* fileName, LPCSTR shaderTextureName, int xRes, int yRes, ID3D10Device* pd3dDevice);
	void draw();

private:
	Sky(const Sky& rhs);
	Sky& operator=(const Sky& rhs);

private:
	ID3D10Device* md3dDevice;
	ID3D10Buffer* mVB;
	ID3D10Buffer* mIB;
	
	ID3D10ShaderResourceView* mCubeMap;

	UINT mNumIndices;
 
	ID3D10EffectTechnique* mTech;
	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWVar;
	ID3D10EffectShaderResourceVariable* mfxCubeMapVar;
	ID3D10EffectVariable* mfxEyePosVar;
};

#endif // SKY_H