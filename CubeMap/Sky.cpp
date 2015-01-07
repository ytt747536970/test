#include "Sky.h"
#include "TextureMgr.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"
#include <fstream>
using namespace std;
struct SkyVertex
{
	D3DXVECTOR3 pos;
};
 
Sky::Sky()
: md3dDevice(0), mVB(0), mIB(0), mCubeMap(0)
{
	mNumIndices = 0;
}

Sky::~Sky()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
}

void Sky::init(ID3D10Device* device, ID3D10ShaderResourceView* cubemap, float radius)
{
	md3dDevice = device;
	mCubeMap   = cubemap;

	mTech         = fx::SkyFX->GetTechniqueByName("SkyTech");
	mfxWVPVar     = fx::SkyFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWVar       = fx::SkyFX->GetVariableByName("gWorld")->AsMatrix();
	mfxCubeMapVar = fx::SkyFX->GetVariableByName("gCubeMap")->AsShaderResource();
	mfxEyePosVar  = fx::SkyFX->GetVariableByName("gEyePosW")->AsVector();
	mfxInverseProjection =  fx::SkyFX->GetVariableByName("g_mInverseProjection")->AsMatrix();
	mfxg_mInvView =  fx::SkyFX->GetVariableByName("g_mInvView")->AsMatrix();
	

	std::vector<D3DXVECTOR3> vertices;
//	DWORD indices[6];
//	std::vector<DWORD> indices;
//	BuildGeoSphere(2, radius, vertices, indices);

	std::vector<SkyVertex> skyVerts(vertices.size());
	for(size_t i = 0; i < vertices.size(); ++i)
	{
		// Scale on y-axis to turn into an ellipsoid to make a flatter Sky surface
		skyVerts[i].pos = 0.5f*vertices[i];
		
	}

	/*D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SkyVertex) * (UINT)skyVerts.size();
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &skyVerts[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));*/
	D3DXVECTOR3 verticesQuad[4];
    verticesQuad[0] = D3DXVECTOR3( 1.0f,  1.0f,0.0);
    verticesQuad[1] = D3DXVECTOR3( 1.0f, -1.0f,0.0);
    verticesQuad[2] = D3DXVECTOR3(-1.0f,  -1.0f,0.0);
    verticesQuad[3] = D3DXVECTOR3(-1.0f,  1.0f,0.0);
    D3D10_SUBRESOURCE_DATA InitDataQuad;
    InitDataQuad.pSysMem  = verticesQuad;
    D3D10_BUFFER_DESC      bdQuad;
    bdQuad.Usage          = D3D10_USAGE_IMMUTABLE;
    bdQuad.ByteWidth      = sizeof( D3DXVECTOR3 ) * 4;
    bdQuad.BindFlags      = D3D10_BIND_VERTEX_BUFFER;
    bdQuad.CPUAccessFlags = 0;
    bdQuad.MiscFlags      = 0;    
    md3dDevice->CreateBuffer( &bdQuad, &InitDataQuad, &mVB );
	
/*	mNumIndices = (UINT)indices.size();

	D3D10_BUFFER_DESC ibd;
    ibd.Usage = D3D10_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * mNumIndices;
    ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));*/
	DWORD indices[6] = {
		// front face
		0, 1, 2,
		0, 2, 3
	};
	mNumIndices = 6;
	D3D10_BUFFER_DESC ibd;
    ibd.Usage = D3D10_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * 6;
    ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));


	//Ftable
	 //load the look up tables for the fog, see http://www1.cs.columbia.edu/~bosun/sig05.htm
    if(loadLUTS("F_512_data.csv","Ftable",512,512, md3dDevice) == S_FALSE)
		   loadLUTS("F_512_data.csv","Ftable",512,512, md3dDevice);    
	
   /* if(loadLUTS("G0_pi_2_64_data.csv","Gtable",64,64, md3dDevice) == S_FALSE)
        loadLUTS("../Media/G0_pi_2_64_data.csv","Gtable",64,64, md3dDevice); 
    if(loadLUTS("../../Media/G20_pi_2_64_data.csv","G_20table",64,64, md3dDevice) == S_FALSE)
        loadLUTS("../Media/G20_pi_2_64_data.csv","G_20table",64,64, md3dDevice); */
}

void Sky::draw()
{
	D3DXVECTOR3 eyePos = GetCamera().position();

	// center Sky about eye in world space
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W, 0, 1, 0);
//	D3DXMatrixTranslation(&W, eyePos.x,eyePos.y,eyePos.z);
	
	D3DXMATRIX V = GetCamera().view();
	D3DXMATRIX P = GetCamera().proj();

	D3DXMATRIX WVP = W*V*P;

	HR(mfxWVar->SetMatrix((float*)W));
	HR(mfxWVPVar->SetMatrix((float*)WVP));
	HR(mfxCubeMapVar->SetResource(mCubeMap));
	mfxEyePosVar->SetRawValue(&GetCamera().position(), 0, sizeof(D3DXVECTOR3));
	D3DXMATRIX InvProjectionMatrix;
	D3DXMatrixInverse( &InvProjectionMatrix, NULL,&GetCamera().proj());
	mfxInverseProjection->SetMatrix(InvProjectionMatrix);
	D3DXMATRIX InvViewMatrix;
	D3DXMatrixInverse( &InvViewMatrix, NULL, &GetCamera().view());
	mfxg_mInvView->SetMatrix(InvViewMatrix);

	UINT stride = sizeof(SkyVertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
	md3dDevice->IASetInputLayout(InputLayout::Pos);
	md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(p);

		pass->Apply(0);
		md3dDevice->DrawIndexed(mNumIndices, 0, 0);
	//	md3dDevice->Draw(4,0);
	}
}
HRESULT Sky::loadLUTS(char* fileName, LPCSTR shaderTextureName, int xRes, int yRes, ID3D10Device* pd3dDevice)
{
    HRESULT hr = S_OK;
	
    ifstream infile (fileName ,ios::in);
    if (infile.is_open())
    {   
        float* data = new float[xRes*yRes];
        int index = 0;
        char tempc;
        for(int j=0;j<yRes;j++)
        {   for(int i=0;i<xRes-1;i++)  
               infile>>data[index++]>>tempc;
            infile>>data[index++];
            
        }
        
        D3D10_SUBRESOURCE_DATA InitData;
        InitData.SysMemPitch = sizeof(float) * xRes;
        InitData.pSysMem = data;

        ID3D10Texture2D* texture = NULL;
        D3D10_TEXTURE2D_DESC texDesc;
        ZeroMemory( &texDesc, sizeof(D3D10_TEXTURE2D_DESC) );
        texDesc.Width = xRes;
        texDesc.Height = yRes;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D10_USAGE_DEFAULT;
        texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;

        pd3dDevice->CreateTexture2D(&texDesc,&InitData,&texture);

        D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
        SRVDesc.Format = texDesc.Format;
        SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;
        SRVDesc.Texture2D.MostDetailedMip = 0;

        ID3D10ShaderResourceView* textureRview;
        pd3dDevice->CreateShaderResourceView( texture, &SRVDesc, &textureRview);
		ID3D10EffectShaderResourceVariable* textureRVar = fx::SkyFX->GetVariableByName( shaderTextureName )->AsShaderResource();
        textureRVar->SetResource( textureRview );

		texture->Release();
		textureRview->Release();
        delete[] data;
    }
    else
       hr = S_FALSE;
    return hr;
}