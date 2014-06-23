## Yet Another Face Detection and Bluring

This project is a set of tools to detect objects in panoramic images and apply blur operation to them. It supports
currently input images in EQR (equirectangular) projection and outputs coordinates of detected objects as well as
a blurred version of the input panoramic image.

It also contains a tool to evaluate the performance of a given object detection algorithm / model.

## Implemented algorithms

* Vanilla haar cascades: by using a gnomonic projection window from the eqr image, the default face detection
model can be applied to the undistorted image.

* Trained haar cascades (todo): the goal is to try to bypass the linear reprojection step by training a model
against variations of faces when subjected to eqr projection. This could work for the main area (near the
equatorial line) but probably not for the poles. It's possible to compute eqr projection centered at the
poles to achieve detection in 3 passes (north, center, south).

* Trained latent svm (todo): by following same approach as above, but using a different classifier based on a
trained non-linear svm model. This should improve detection accuracy directly within eqr compared to haar
cascades.


## Compilation

	make clean
	make


## Usage: object detection

Detects objects within input panorama (eqr). Detected objects are written to a text file (see format below).

	yafdb-detect --algorithm algo input-panorama.tiff output-objects.txt

	--algorithm algo: algorithm to use for object detection ("haar")
	--reprojection-window-size=640x480: window size for reprojection task
	--reprojection-dx=45: horizontal angle increment for reprojection task
	--reprojection-dy=45: vertical angle increment for reprojection task


## Usage: object blurring

Blurs detected objects and write modified panorama as output.

	yafdb-blur --algorithm algo input-panorama.tiff input-objects.txt output-panorama.tiff

	--algorithm algo: algorithm to use for object detection ("default")


## Usage: performance validation

Compute the detection error rate by comparing optimal area given in reference bitmap mask (black=none, white=object)
to the detected area given in input text file.

	yafdb-test input-panorama.tiff reference-mask.tiff detected-objects.txt


=

The yafdb toolchain is distributed under [GNU GPL license](http://foxel.ch/en/license) and is part of the FOXEL project [http://foxel.ch/](http://foxel.ch/)
