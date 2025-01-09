// SkinnedModel.hlsl

cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxProjection : packoffset(c4);
    float3 gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject : packoffset(c0);
};

Texture2D gTexture : register(t0); // Texture binding
SamplerState gSampler : register(s0); // Sampler binding

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_FBX_MODEL_INPUT
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VS_FBX_MODEL_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_FBX_MODEL_OUTPUT VSFbxModel(VS_FBX_MODEL_INPUT input)
{
    VS_FBX_MODEL_OUTPUT output;

    output.position = mul(mul(mul(input.position, gmtxGameObject), gmtxView), gmtxProjection);
    output.texcoord = input.texcoord;

    return (output);
}

float4 PSFbxModel(VS_FBX_MODEL_OUTPUT input) : SV_TARGET
{
	// float4 cColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
    float2 uv = input.texcoord;
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    return gTexture.Sample(gSampler, uv); // Texture Sampling
}

VS_FBX_MODEL_OUTPUT VSFbxSkinnedModel(VS_FBX_MODEL_INPUT input)
{
    VS_FBX_MODEL_OUTPUT output;

    output.position = mul(mul(mul(input.position, gmtxGameObject), gmtxView), gmtxProjection);
    output.texcoord = input.texcoord;
	
    return (output);
}

float4 PSFbxSkinnedModel(VS_FBX_MODEL_OUTPUT input) : SV_TARGET
{
    float2 uv = input.texcoord;
    float4 sample = gTexture.Sample(gSampler, uv);

    // 알파 값이 0.0이면 Magenta 반환
    if (sample.a == 0.0f)
    {
        return float4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
    }

    // UV 좌표가 유효하지 않으면 Green 반환
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    }

    // 텍스처 샘플링 결과 반환
    return sample;
    // return float4(uv.x, uv.y, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SKINNED_ANIMATION_BONES 256
#define MAX_VERTEX_INFLUENCES 4

cbuffer cbBoneOffsets : register(b7)
{
    float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
};

cbuffer cbBoneTransforms : register(b8)
{
    float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
};

//struct VS_ANIMATED_MODEL_INPUT
//{
//    float3 position : POSITION;
//    float2 uv : TEXCOORD;
//    int4 indices : BONEINDEX;
//    float4 weights : BONEWEIGHT;
//};

//struct VS_ANIMATED_MODEL_OUTPUT
//{
//    float4 position : SV_POSITION;
//    float2 uv : TEXCOORD;
//};

//VS_ANIMATED_MODEL_OUTPUT VSAnimation(VS_ANIMATED_MODEL_INPUT input)
//{
//    VS_ANIMATED_MODEL_OUTPUT output;
    
//    float3 positionW = float3(0.0f, 0.0f, 0.0f);
//    matrix mtxVertexToBoneWorld;
//    for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
//    {
//        mtxVertexToBoneWorld = mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
//        positionW += input.weights[i] * mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
//    }

//    output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);
//    output.uv = input.uv;
    
//    return (output);
//}

//float4 PSAnimation(VS_ANIMATED_MODEL_OUTPUT input) : SV_TARGET
//{
//    float4 Color = gTexture.Sample(gSampler, input.uv);

//    return (Color);
//}