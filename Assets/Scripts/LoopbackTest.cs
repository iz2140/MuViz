using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
public class LoopbackTest : MonoBehaviour {

  int i = 0;
  void PCMReaderCallback(float[] data)
  {
  //  for (var j = 0; j < data.Length; ++j)
  //  { 
  //    data[j] = Mathf.Sin(i++ / 44100f * 2f * Mathf.PI * 440f);

  //  }

    GCHandle pinnedArray = GCHandle.Alloc(data, GCHandleType.Pinned);
    IntPtr pointer = pinnedArray.AddrOfPinnedObject();
    NativeApi.Read((uint)data.Length, pointer);

    pinnedArray.Free();


  }

  // Use this for initialization
  void Start () {
    NativeApi.Start(2u);
    var audioSource = GetComponent<AudioSource>();
    audioSource.clip = AudioClip.Create("loopback", int.MaxValue, 1, 44100, true, PCMReaderCallback);
    audioSource.Play();
  }
  
  // Update is called once per frame
  void Update () {
    var audioSource = GetComponent<AudioSource>();

  }

  struct NativeApi
  {
    [DllImport("loopback_recorder")]
    public static extern void Start(uint device_index);

    [DllImport("loopback_recorder")]
    public static extern void Read(uint sample_count, IntPtr out_samples);
  }
}
