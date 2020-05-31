//--------------------------------------------------------------------------------------
// File: BasicHLSL11_VS.hlsl
//
// The vertex shader file for the BasicHLSL11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
//cbuffer cbPerObject : register( b0 )
//{
//    matrix g_mWorldViewProjection : packoffset( c0 );
//};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 vPosition : POSITION;
    float4 vColor   : COLOR0;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float3 vColor    : COLOR1;
    float2 vTexcoord : TEXCOORD1;
    float4 vPosition : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( VS_INPUT Input )
{
    VS_OUTPUT Output;
    
    //Output.vPosition = mul( Input.vPosition, g_mWorldViewProjection );
    Output.vPosition = Input.vPosition;
    Output.vColor = Input.vColor;
    Output.vTexcoord = Input.vTexcoord;
    
    return Output;
}
