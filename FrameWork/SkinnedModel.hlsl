// SkinnedModel.hlsl

// 본 행렬 최대 개수 및 가중치 영향 개수
#define SKINNED_ANIMATION_BONES 256
#define MAX_VERTEX_INFLUENCES 4

// 본 오프셋 행렬 (모델 로딩 시 세팅)
cbuffer cbBoneOffsets : register(b7)
{
    float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
}

// 애니메이션 본 변환 행렬 (애니메이션 진행 중 업데이트)
cbuffer cbBoneTransforms : register(b8)
{
    float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
}

// 카메라 및 게임 오브젝트 변환 정보
cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxProjection : packoffset(c4);
    float3 gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject : packoffset(c0);
}

// 텍스처 및 샘플러
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// 스키닝 애니메이션 버텍스 입력 구조체
struct VS_ANIMATED_MODEL_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    int4 indices : BONEINDEX; // 버텍스에 영향을 주는 본 인덱스
    float4 weights : BONEWEIGHT; // 각 본의 가중치
};

// 버텍스 셰이더 출력 구조체
struct VS_ANIMATED_MODEL_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// 스키닝 애니메이션 버텍스 셰이더
VS_ANIMATED_MODEL_OUTPUT VSAnimation(VS_ANIMATED_MODEL_INPUT input)
{
    VS_ANIMATED_MODEL_OUTPUT output;

    float3 positionW = float3(0.0f, 0.0f, 0.0f);

    // 최대 4개의 본 가중치를 기반으로 위치 계산
    for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
    {
        int boneIndex = input.indices[i];
        float weight = input.weights[i];

        if (weight > 0.0f)
        {
            // 버텍스 → 본 공간 변환 → 본 → 월드 변환
            matrix boneTransform = mul(gpmtxBoneOffsets[boneIndex], gpmtxBoneTransforms[boneIndex]);
            positionW += weight * mul(float4(input.position, 1.0f), boneTransform).xyz;
        }
    }

    // 최종 변환: 월드 → 뷰 → 투영
    float4 worldPosition = mul(float4(positionW, 1.0f), gmtxGameObject);
    float4 viewPosition = mul(worldPosition, gmtxView);
    output.position = mul(viewPosition, gmtxProjection);

    // UV 좌표 전달
    output.uv = input.uv;

    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// 픽셀 셰이더 (스키닝 모델)
float4 PSAnimation(VS_ANIMATED_MODEL_OUTPUT input) : SV_TARGET
{
    // 텍스처 샘플링
    float4 color = gTexture.Sample(gSampler, input.uv);

    // UV 좌표 오류 시 마젠타 색상 출력
    if (input.uv.x < 0.0f || input.uv.x > 1.0f || input.uv.y < 0.0f || input.uv.y > 1.0f)
    {
        return float4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
    }

    return color;
}
