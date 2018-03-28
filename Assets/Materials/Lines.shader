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
      float sampleCount = 256;
      float f;
      float lineBoost;

      float F(float t) {
        return exp(-t*f*.5);
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
      float Sample(sampler2D s, float x) {
        float f = 2.0 * tex2D(s, float2(x + 0.0 / sampleCount, 0.5)).r 
          + tex2D(s, float2(x - 1.0 / sampleCount, 0.5)).r
          + tex2D(s, float2(x - 0.0 / sampleCount, 0.5)).r;
        return f / 4;
      }

      fixed4 frag (v2f i) : SV_Target
      {
        float inverseSampleCount = 1.0 / sampleCount;
      float a = saturate(0.5 - abs(0.5 - i.uv.x)) / 0.5;
       a = .7 * pow(smoothstep(0.1, 0.9, a), .5);
        float intensity0 = a*Sample(intensities0, lerp(0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x));
        float intensity1 = a*Sample(intensities1, lerp(0.5 * inverseSampleCount, 1.0 - 0.5 * inverseSampleCount, i.uv.x));

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

        //f = lerp(f, dot(f, 1.0 / 3), 0.25);

        //float t = 0.5;
        //col.rgb += abs(i.uv.y - t);
        float v = saturate(0.5 - distance(i.uv.x, 0.5))/.5;
        v = smoothstep(0.05, .9, pow(v, 1.0));
        //return v;
        //return float4(f, 1);
        return float4(v*f, 1);
      }
      ENDCG
    }
  }
}
