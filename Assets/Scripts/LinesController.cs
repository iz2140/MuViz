using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LinesController : MonoBehaviour {

  public AudioSource audioSource;
  public LineRenderer lineRenderer;

  // Use this for initialization
  void Start() {

  }

  // Update is called once per frame
  void Update() {

    var startPosition = lineRenderer.GetPosition(0);
    var endPosition = lineRenderer.GetPosition(lineRenderer.positionCount - 1);

    lineRenderer.positionCount = 256;
    var spectrumData = audioSource.GetOutputData(lineRenderer.positionCount, 0);

    var positions = new Vector3[lineRenderer.positionCount];
    for (int i = 0; i < positions.Length; ++i) {
      var t = i / (positions.Length - 1f);
      var x = Mathf.Lerp(startPosition.x, endPosition.x, t);
      var z = Mathf.Lerp(startPosition.z, endPosition.z, t);
      positions[i] = new Vector3(x, 0.5f + spectrumData[i], z);
    }
    lineRenderer.SetPositions(positions);

  }
}
