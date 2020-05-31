//--------------------------------------------------------------------------------------
// File: BasicHLSL11_PS.hlsl
//
// The pixel shader file for the BasicHLSL11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
//cbuffer cbPerFrame : register( b1 )
//{
//    float3        g_vLightDir                : packoffset( c0 );
//    float        g_fAmbient                : packoffset( c0.w );
//};

//--------------------------------------------------------------------------------------
// Textures and Samplers
//--------------------------------------------------------------------------------------
//Texture2D    g_txDiffuse : register( t0 );
//SamplerState g_samLinear : register( s0 );

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
    float4 vColor    : COLOR1;
    float2 vTexcoord : TEXCOORD1;
    float4 vPosition : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain( PS_INPUT Input ) : SV_TARGET
{
    //float4 vDiffuse = g_txDiffuse.Sample( g_samLinear, Input.vTexcoord );
    
    //float fLighting = saturate( dot( g_vLightDir, Input.vNormal ) );
    //fLighting = max( fLighting, g_fAmbient );
    
    //return vDiffuse * fLighting;
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
