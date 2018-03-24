using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Linq;

public class bouncy : MonoBehaviour {

	// Use this for initialization
	void Start () {
	}
  float s = 0f;
  // Update is called once per frame
  float y = 0f;
	void Update () {
    var samples = new float[256];
    AudioListener.GetSpectrumData(samples, 0, FFTWindow.BlackmanHarris);
    var x = samples.Max();
    s = 0.1f * x  + 0.9f * s;
    this.transform.localScale = Vector3.one * (1f + s* 100f);
    y += s * 100f;
    transform.localEulerAngles = new Vector3(y / 2f, y / 3f, y);

  }
}
