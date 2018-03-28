Shader "Hidden/Lines"
{
  Properties
  {
    _MainTex("Texture", 2D) = "white" {}
    f("f", Float) = 25.0
    lineBoost("lineBoost", Float) = 1.0
  }
  SubShader
  {
    // No culling or depth
    Cull Off ZWrite Off ZTest Always

    Pass
    {
      CGPROGRAM
      #pragma vertex vert
      #pragma fragment frag
      
      #include "UnityCG.cginc"

      struct appdata
      {
        float4 vertex : POSITION;
        float2 uv : TEXCOORD0;
      };

      struct v2f
      {
        float2 uv : TEXCOORD0;
        float4 vertex : SV_POSITION;
      };

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);
        o.uv = v.uv;
        return o;
      }
      
      sampler2D _MainTex;
      sampler2D intensities0;
      sampler2D intensities1;
      float sampleCount;
      float f;
      float lineBoost;

      float F(float t) {
        return exp(-t*f);
      }


      float3 pal(float3 a, float3 b, float3 c, float3 d, float t)
      {
        return a + b * cos(6.28318*(c*t + d));
      }
      float3 filmicToneMapping(float3 color)
      {
        color = max(0, color - 0.004);
        color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
        return color;
      }

      fixed4 frag (v2f i) : SV_Target
      {
        float inverseSampleCount = 1.0 / sampleCount;
        float intensity0 = .7*tex2D(intensities0, float2(lerp(0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x), 0.5)).r;
        float intensity1 = .7*tex2D(intensities1, float2(lerp(0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x), 0.5)).r;
        float intensity2 = .7*tex2D(intensities0, float2(lerp(0.5 + 0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x), 0.5)).r;
        float intensity3 = .7*tex2D(intensities1, float2(lerp(0.5 + 0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x), 0.5)).r;

        float y = i.uv.y * 2.0 - 1.0;

        
        float3 f = 0;
        f += pal(float3(0.5,0.5,0.5),float3(0.5,0.5,0.5),float3(1.0,1.0,1.0),float3(0.0,0.33,0.67), _Time.y + 0.5) * F(abs(y - intensity0));
        f += pal(float3(0.5,0.5,0.5),float3(0.5,0.5,0.5),float3(1.0,1.0,1.0),float3(0.0,0.33,0.67), _Time.y) * F(abs(y - intensity1));
        //f += pal(float3(0.5,0.5,0.5),float3(0.5,0.5,0.5),float3(1.0,1.0,1.0),float3(0.0,0.33,0.67), 0.5) * F(abs(y - intensity2));
        //f += pal(float3(0.5,0.5,0.5),float3(0.5,0.5,0.5),float3(1.0,1.0,1.0),float3(0.0,0.33,0.67), 0.75) * F(abs(y - intensity3));
        f *= lineBoost;
        //f = filmicToneMapping(f * 10.0);
        //f /= 4.0;
        //f *= 0.5;
        // just invert the colors
        //col.rgb = 1 - col.rgb;
        //col.rgb *= 0.0;

        //float t = 0.5;
        //col.rgb += abs(i.uv.y - t);
        return float4(f, 1);
      }
      ENDCG
    }
  }
}
