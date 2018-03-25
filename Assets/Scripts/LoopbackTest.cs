using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
public class LoopbackTest : MonoBehaviour {

  private const string PLAYER_PREFS_DEVICE_INDEX = "LOOPBACK_DEVICE_INDEX";
  private uint deviceIndex;
  private int errorToneIndex = 0;
  private string selectedEndpointLabelText;

  void Start() {
    deviceIndex = (uint)PlayerPrefs.GetInt(PLAYER_PREFS_DEVICE_INDEX, 0);

    var endpointName = GetEndpointName(deviceIndex);
    if (null != endpointName) {
      selectedEndpointLabelText = String.Format("Using \"{0}\" as audio source.", endpointName);
      if (!StartLoopback()) {
        Debug.LogErrorFormat("Error using {0}!", endpointName);
      }
    } else {
      Debug.LogError("Couldn't find a suitable audio source!");
    }
  }


  void Update() {
    var audioSource = GetComponent<AudioSource>();

    if (Input.GetKeyDown(KeyCode.Space)) {
      ++deviceIndex;

      var endpointName = GetEndpointName(deviceIndex);
      if (null == endpointName) {
        deviceIndex = 0;
        endpointName = GetEndpointName(deviceIndex);
      }

      if (null != endpointName) {
        selectedEndpointLabelText = String.Format("Using \"{0}\" as audio source.", endpointName);
        if (!StartLoopback()) {
          Debug.LogErrorFormat("Error using {0}!", endpointName);
        } else {
          PlayerPrefs.SetInt(PLAYER_PREFS_DEVICE_INDEX, (int)deviceIndex);
        }
      } else {
        Debug.LogError("Couldn't find a suitable audio source!");
      }
    }

  }

  private void OnGUI() {
    if (null != selectedEndpointLabelText) {
      GUI.Label(Screen.safeArea, selectedEndpointLabelText);
    }
  }

  private void PCMReaderCallback(float[] data) {
    GCHandle pinnedArray = GCHandle.Alloc(data, GCHandleType.Pinned);
    IntPtr pointer = pinnedArray.AddrOfPinnedObject();
    if (!NativeApi.Read((uint)data.Length, pointer)) {
      for (var j = 0; j < data.Length; ++j) {
        data[j] = Mathf.Sin((errorToneIndex++ / 2) / 44100f * 2f * Mathf.PI * 440f) * 0.15f;
      }
    }
    pinnedArray.Free();
  }

  private bool StartLoopback() {
    if (!NativeApi.Start(deviceIndex)) return false;

    var audioSource = GetComponent<AudioSource>();
    audioSource.clip = AudioClip.Create("loopback", int.MaxValue, 2, 44100, true, PCMReaderCallback);
    audioSource.Play();

    return true;
  }

  private static string GetEndpointName(uint index) {
    var namePtr = NativeApi.GetEndpointName(index);
    if (IntPtr.Zero == namePtr) return null;

    var nameString = Marshal.PtrToStringBSTR(namePtr);
    Marshal.FreeBSTR(namePtr);

    return nameString;
  }

  struct NativeApi {
    [DllImport("loopback_recorder")]
    public static extern bool Start(uint device_index);

    [DllImport("loopback_recorder")]
    public static extern bool Read(uint sample_count, IntPtr out_samples);

    [DllImport("loopback_recorder")]
    public static extern IntPtr GetEndpointName(uint index);
  }
}
