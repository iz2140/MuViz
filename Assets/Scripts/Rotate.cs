using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Rotate : MonoBehaviour {

    private const int ROTATE_SPEED_MULTIPLIER = 4;

	// Use this for initialization
	void Start () { 
	}
	
	// Update is called once per frame
	void Update () {
        if (transform.parent.name == "Plane1")
        {
            transform.Rotate(Vector3.forward * Time.deltaTime * ROTATE_SPEED_MULTIPLIER);
        }
        else {
            transform.Rotate(Vector3.back * Time.deltaTime * ROTATE_SPEED_MULTIPLIER);
        }
    }
}
