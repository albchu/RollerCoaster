README
CPSC 587 Animation
Assignment 1
Albert Chu
10059388

This project has been developed and tested with GLM 0.9.5.4 on Visual Studio 2013
Note: Graphics lab computers have an older version of GLM that has a bug which does not allow this program to run. 
Please update GLM lib to proper version before running Makefile

Compiling:
Run "make" in source directory.

Running:
"./RollerCoaster"

Controls:
'l' : Toggles camera lock
'r' : Enables manual mouse rotation of camera. Click and hold left mouse button to drag and rotate camera.
't' : Enables manual mouse camera translation. Click and hold left mouse button to drag and move camera.
Right click and drag will translate camera in z axis

About:
The Frenet Frame is applied to both the track and the car to show the correct orientation of the track and car respectfully given variable speed.
On program initialization, once the track is loaded from file, it is smoothed into a nice curve with chaikin quadratic subdivision. Following this, 
the Frenet frame is applied to each vertex of the track in order to get a wide railing for the cart. The railing is computed using the binormal calculated
given its position in relation to the maximum height of the roller coaster track.