using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;
using UnityEngine;

public class LinesEffect : MonoBehaviour {
  private const int SAMPLE_COUNT = 256;

  public AudioSource audioSource;
  public Material material;
  public float totalScaler = 1.0f;
  public float totalDamp = 1.0f;
  private Texture2D[] audioTextures = new Texture2D[2];
  private float[] intensities = new float[SAMPLE_COUNT];
  private float[] frequencies = new float[SAMPLE_COUNT];
  private float prevIntensity;
  private float[] beats = new float[4];

  private void Start() {
        //sample count is width, 1 is height, 
    audioTextures[0] = new Texture2D(SAMPLE_COUNT, 1, TextureFormat.RFloat, false);
    audioTextures[1] = new Texture2D(SAMPLE_COUNT, 1, TextureFormat.RFloat, false);

  }

  private void Update() {
    material.SetTexture("intensities0", audioTextures[0]);
    material.SetTexture("intensities1", audioTextures[1]);
    material.SetFloat("sampleCount", SAMPLE_COUNT);

        //every half frame uploading new data to shader
    var channelIndex = Time.frameCount % 2;
    
        //gets the audio source data from unity
    audioSource.GetOutputData(intensities, channelIndex);
    var intensitiesAddr = GCHandle.Alloc(intensities, GCHandleType.Pinned);
    var pointer = intensitiesAddr.AddrOfPinnedObject(); //upload c# float array
    
        //this is where samples are passed to shader
    audioTextures[channelIndex].LoadRawTextureData(pointer, SAMPLE_COUNT * sizeof(float));
    audioTextures[channelIndex].Apply();
    intensitiesAddr.Free();
        
        //total intensity is all the samples added together
        //this lets the line fade out when song fades out
        var total = intensities.Select(o => Mathf.Abs(o)).Sum();
    total = totalDamp * total + (1 - totalDamp) * prevIntensity;
    material.SetFloat("totalIntensity", totalScaler * Mathf.Log(1f + total) / SAMPLE_COUNT);
    prevIntensity = total;

        //not doing anything with beats yet
    audioSource.GetSpectrumData(frequencies, channelIndex, FFTWindow.Blackman);
    var b = frequencies.Take(frequencies.Length / 4).Sum();
    beats[0] = Mathf.Max(beats[0] * 0.975f, b);

    b = frequencies.Skip(frequencies.Length / 4).Take(frequencies.Length / 4).Sum();
    beats[1] = Mathf.Max(beats[1] * 0.975f, b);

    b = frequencies.Skip(2*frequencies.Length / 4).Take(frequencies.Length / 4).Sum();
    beats[2] = Mathf.Max(beats[2] * 0.975f, b);

    b = frequencies.Skip(3*frequencies.Length / 4).Take(frequencies.Length / 4).Sum();
    beats[3] = Mathf.Max(beats[3] * 0.975f, b);
    material.SetFloatArray("beats", beats);

  }

  private void OnRenderImage(RenderTexture src, RenderTexture dest) {
    Graphics.Blit(src, dest, material);
  }

}
