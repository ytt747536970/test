//=============================================================================
// cubemap.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates sampling a cubemap texture.
//=============================================================================


#include "lighthelper.fx"
 
#define PI 3.14159265
cbuffer cbPerFrame
{
	Light gLight;
	float3 gEyePosW;
};

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWVP; 
	float4x4 gTexMtx;
	float4 gReflectMtrl;
	bool gCubeMapEnabled;
};
cbuffer cbImmutable
{
    float g_Kd = 0.1;
    float g_KsPoint = 0.01;
    float g_KsDir = 10;
    float g_specPower = 10;
    float4 pointLightColor = float4(1.0,1.0,1.0,1.0);
    float3 g_PointLightPos = float3(  3.7,8.8,3.15);     
    float3 g_PointLightPos2 = float3(-3.7,5.8,3.15);
	float3 g_beta = float3(10.04,10.04,10.04);
	float g_PointLightIntensity = 2.0;
    float g_fXOffset = 0; 
    float g_fXScale = 0.6366198; //1/(PI/2)
    float g_fYOffset = 0;        
    float g_fYScale = 0.5;
    
    float g_20XOffset = 0; 
    float g_20XScale = 0.6366198; //1/(PI/2) 
    float g_20YOffset = 0;
    float g_20YScale = 0.5;

    float g_diffXOffset = 0; 
    float g_diffXScale = 0.5;
    float g_diffYOffset = 0;        
    float g_diffYScale = 0.3183099;  //1/PI   
}
// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
Texture2D gSpecMap;
TextureCube gCubeMap;
Texture2D Ftable;
Texture2D Gtable;
Texture2D G_20table;
SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};
SamplerState samLinearClamp
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_IN
{
	float3 posL    : POSITION;
	float3 normalL : NORMAL;
	float2 texC    : TEXCOORD;
};

struct VS_OUT
{
	float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
    float2 texC    : TEXCOORD;
};

//---------------------------------------------------------------------------------------
//auxiliary functions for calculating the Fog
//---------------------------------------------------------------------------------------

float3 calculateAirLightPointLight(float Dvp,float Dsv,float3 S,float3 V)
{
    float gamma = acos(dot(S, V));
    gamma = clamp(gamma,0.01,PI-0.01);
    float sinGamma = sin(gamma);
    float cosGamma = cos(gamma);
    float u = g_beta.x * Dsv * sinGamma;
    float v1 = 0.25*PI+0.5*atan((Dvp-Dsv*cosGamma)/(Dsv*sinGamma)); 
    float v2 = 0.5*gamma;
            
    float lightIntensity = g_PointLightIntensity * 100;        
            
    float f1= Ftable.SampleLevel(samLinearClamp, float2((v1-g_fXOffset)*g_fXScale, (u-g_fYOffset)*g_fYScale), 0);
    float f2= Ftable.SampleLevel(samLinearClamp, float2((v2-g_fXOffset)*g_fXScale, (u-g_fYOffset)*g_fYScale), 0);
    float airlight = (g_beta.x*lightIntensity*exp(-g_beta.x*Dsv*cosGamma))/(2*PI*Dsv*sinGamma)*(f1-f2);
    
    return airlight.xxx;
}

float3 calculateDiffusePointLight(float Kd,float Dvp,float Dsv,float3 pointLightDir,float3 N,float3 V)
{

    float Dsp = length(pointLightDir);
    float3 L = pointLightDir/Dsp;
    float thetas = acos(dot(N, L));
    float lightIntensity = g_PointLightIntensity * 100;
    
    //spotlight
  /*  float angleToSpotLight = dot(-L, g_SpotLightDir);
    if(g_useSpotLight)
    {    if(angleToSpotLight > g_cosSpotlightAngle)
             lightIntensity *= abs((angleToSpotLight - g_cosSpotlightAngle)/(1-g_cosSpotlightAngle));
         else
             lightIntensity = 0;         
    }   */
    
    //diffuse contribution
    float t1 = exp(-g_beta.x*Dsp)*max(cos(thetas),0)/Dsp;
    float4 t2 = g_beta.x*Gtable.SampleLevel(samLinearClamp, float2((g_beta.x*Dsp-g_diffXOffset)*g_diffXScale, (thetas-g_diffYOffset)*g_diffYScale),0)/(2*PI);
    float rCol = (t1+t2.x)*exp(-g_beta.x*Dvp)*Kd*lightIntensity/Dsp;
    float diffusePointLight = float3(rCol,rCol,rCol);  
    return diffusePointLight.xxx;
}



float3 Specular(float lightIntensity, float Ks, float Dsp, float Dvp, float specPow, float3 L, float3 VReflect)
{
    lightIntensity = lightIntensity * 100;
    float LDotVReflect = dot(L,VReflect);
    float thetas = acos(LDotVReflect);

    float t1 = exp(-g_beta*Dsp)*pow(max(LDotVReflect,0),specPow)/Dsp;
    float4 t2 = g_beta.x*G_20table.SampleLevel(samLinearClamp, float2((g_beta.x*Dsp-g_20XOffset)*g_20XScale, (thetas-g_20YOffset)*g_20YScale),0)/(2*PI);
    float specular = (t1+t2.x)*exp(-g_beta.x*Dvp)*Ks*lightIntensity/Dsp;
    return specular.xxx;
}
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	// Transform to world space space.
	vOut.posW    = mul(float4(vIn.posL, 1.0f), gWorld);
	vOut.normalW = mul(float4(vIn.normalL, 0.0f), gWorld);
		
	// Transform to homogeneous clip space.
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);
	
	// Output vertex attributes for interpolation across triangle.
	vOut.texC = mul(float4(vIn.texC, 0.0f, 1.0f), gTexMtx);
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
/*	float4 diffuse = gDiffuseMap.Sample( gTriLinearSam, pIn.texC );
	
	// Kill transparent pixels.
	clip(diffuse.a - 0.15f);
	
	float4 spec    = gSpecMap.Sample( gTriLinearSam, pIn.texC );
	
	// Map [0,1] --> [0,256]
	spec.a *= 256.0f;
	
	// Interpolating normal can make it not be of unit length so normalize it.
    float3 normalW = normalize(pIn.normalW);
    
	// Compute the lit color for this pixel.
    SurfaceInfo v = {pIn.posW, normalW, diffuse, spec};
	float3 litColor = ParallelLight(v, gLight, gEyePosW);
	
	[branch]
	if( gCubeMapEnabled )
	{
		float3 incident = pIn.posW - gEyePosW;
		float3 refW = reflect(incident, normalW);
		float4 reflectedColor = gCubeMap.Sample(gTriLinearSam, refW);
		litColor += (gReflectMtrl*reflectedColor).rgb;
	}
    
    return float4(litColor, diffuse.a);*/


	 float4 outputColor;
        
    float4 sceneColor = gDiffuseMap.Sample( samLinear, pIn.texC );
    float3 viewVec = pIn.posW - gEyePosW;
    float Dvp = length(viewVec);
    float3 V =  viewVec/Dvp; 
	float3 N = pIn.normalW;
	 //reflection of the scene-----------------------------------------------------------
    float3 reflVect = reflect(V, N);
        
    float3 lightEyeVec = g_PointLightPos - gEyePosW;
	float Dsv = length(lightEyeVec);
	float3 S = normalize(lightEyeVec);

    //point light ---------------------------------------------------------------------
    //diffuse surface radiance and airlight due to point light
    float3 pointLightDir = g_PointLightPos - pIn.posW;
    //diffuse
    float3 diffusePointLight = calculateDiffusePointLight(0.3,Dvp,Dsv,pointLightDir,N,V);
    //airlight
    float3 airlight = calculateAirLightPointLight(Dvp,Dsv,S,V);
    //specular
    float3 specularPointLight = Specular(g_PointLightIntensity, g_KsPoint, length(pointLightDir), Dvp, g_specPower, normalize(pointLightDir), reflVect);
	outputColor = float4( airlight + sceneColor.xyz/**diffusePointLight.xyz*/  ,1); 
	return outputColor;
}
 
technique10 CubeMapTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
