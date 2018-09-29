#include "BasicObject.fx"

// ��ʡ�ڴ���Դ������float4����������
static const float4 gVec[2] = { float4(0.0f, 1.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 0.0f) };
static const float2 gTex[4] = (float2[4])gVec;

[maxvertexcount(4)]
void GS(point PointSprite input[1], uint primID : SV_PrimitiveID, 
    inout TriangleStream<BillboardVertex> output)
{
    // ���㹫��������ľֲ�����ϵ�����й�����൱��
    // ��ͶӰ���˾ֲ�����ϵ��xyƽ�棬z=0

    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - input[0].PosW;
    look.y = 0.0f;  // look����ֻȡͶӰ��xzƽ�������
    look = normalize(look);
    float3 right = cross(up, look);

    // ������������ε��ĸ�����
    //            up
    //      v1 ___|___ v3
    //        |   |   |
    // right__|___|   |
    //        |  /    |
    //        |_/_____|
    //      v0 /       v2
    //       look  
    float4 v[4];
    float3 center = input[0].PosW;
    float halfWidth = 0.5f * input[0].SizeW.x;
    float halfHeight = 0.5f * input[0].SizeW.y;
    v[0] = float4(center + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(center + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(center - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(center - halfWidth * right + halfHeight * up, 1.0f);

    // �Զ���λ�ý��о���任������TriangleStrip��ʽ���
    BillboardVertex gOut;
    row_major matrix viewProj = mul(gView, gProj);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gOut.PosW = v[i].xyz;
        gOut.PosH = mul(v[i], viewProj);
        gOut.NormalW = look;
        gOut.Tex = gTex[i];
        gOut.PrimID = primID;
        output.Append(gOut);
    }

}