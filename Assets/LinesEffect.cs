using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class LinesEffect : MonoBehaviour {
  private const int SAMPLE_COUNT = 256;

  public AudioSource audioSource;
  public Material material;
  private Texture2D[] audioTextures = new Texture2D[2];
  private float[] intensities = new float[SAMPLE_COUNT];

  private void Start() {
    audioTextures[0] = new Texture2D(SAMPLE_COUNT, 1, TextureFormat.RFloat, false);
    audioTextures[1] = new Texture2D(SAMPLE_COUNT, 1, TextureFormat.RFloat, false);

  }

  int x = 0;
  private void Update() {
    material.SetTexture("intensities0", audioTextures[0]);
    material.SetTexture("intensities1", audioTextures[1]);
    material.SetFloat("sampleCount", SAMPLE_COUNT);
    ++x;
    for (int i = 0; i < 2; ++i) {
      if ((x + i) % 2 != 0) continue;
      audioSource.GetOutputData(intensities, i);
      var intensitiesAddr = GCHandle.Alloc(intensities, GCHandleType.Pinned);
      var pointer = intensitiesAddr.AddrOfPinnedObject();
      audioTextures[i].LoadRawTextureData(pointer, SAMPLE_COUNT * sizeof(float));
      audioTextures[i].Apply();

      intensitiesAddr.Free();
    }
  }

  private void OnRenderImage(RenderTexture src, RenderTexture dest) {
    Graphics.Blit(src, dest, material);
  }

}
