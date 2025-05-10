// StageShader.hlsl

cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxProjection : packoffset(c4);
    float3 gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject : packoffset(c0);
    float4 gcPixelColor : packoffset(c4);
};

cbuffer cbStageInfo : register(b3)
{
    matrix gStageWorldMatrix; // 맵 월드 변환 행렬
}

struct VS_FBX_STAGE_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VS_FBX_STAGE_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D gStageTexture : register(t1); // 맵 텍스처
SamplerState gStageSampler : register(s0); // 텍스처 샘플러

VS_FBX_STAGE_OUTPUT VSStage(VS_FBX_STAGE_INPUT input)
{
    VS_FBX_STAGE_OUTPUT output;

    float4 worldPosition = mul(float4(input.position, 1.0f), gStageWorldMatrix);
    float4 viewProjectionPosition = mul(worldPosition, mul(gmtxView, gmtxProjection));
    output.position = viewProjectionPosition;

    output.texcoord = input.texcoord;

    return output;
}

float4 PSStage(float2 texcoord : TEXCOORD) : SV_TARGET
{
     // UV 좌표 검증
    if (texcoord.x < 0 || texcoord.x > 1 || texcoord.y < 0 || texcoord.y > 1)
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f); // UV 오류 시 빨간색
    }

    return gStageTexture.Sample(gStageSampler, texcoord);
}