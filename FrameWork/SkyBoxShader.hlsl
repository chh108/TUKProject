// SkyBoxShader.hlsl

cbuffer CameraBuffer : register(b0)
{
    matrix ViewProjection;
}

TextureCube SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

VS_OUTPUT VSSkyBox(float4 position : POSITION)
{
    //VS_OUTPUT output;
    //output.Position = mul(position, ViewProjection);
    //output.TexCoord = position.xyz;
    //return output;
    
    VS_OUTPUT output;

    // 뷰 행렬의 위치 제거 (오프셋 없이)
    float4x4 modifiedViewProjection = ViewProjection;
    modifiedViewProjection._41 = 0;
    modifiedViewProjection._42 = 0;
    modifiedViewProjection._43 = 0;

    output.Position = mul(position, modifiedViewProjection);
    output.TexCoord = position.xyz;

    return output;
}

float4 PSSkyBox(VS_OUTPUT input) : SV_TARGET
{
    return SkyboxTexture.Sample(SkyboxSampler, input.TexCoord);
}
