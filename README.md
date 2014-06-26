
## YAFDB<br />Yet Another Face Detection and Bluring

This project is a set of tools to detect objects in panoramic images and apply blur operation to them. It supports
currently input images in EQR (equirectangular) projection and outputs coordinates of detected objects as well as
a blurred version of the input panoramic image.

It also contains a tool to evaluate the performance of a given object detection algorithm / model.

### Implemented algorithms

* Vanilla haar cascades: by using a gnomonic projection window from the eqr image, the default face detection
model can be applied to the undistorted image.

* Trained haar cascades (todo): the goal is to try to bypass the linear reprojection step by training a model
against variations of faces when subjected to eqr projection. This could work for the main area (near the
equatorial line) but probably not for the poles. It's possible to compute eqr projection centered at the
poles to achieve detection in 3 passes (north, center, south).

* Trained latent svm (todo): by following same approach as above, but using a different classifier based on a
trained non-linear svm model. This should improve detection accuracy directly within eqr compared to haar
cascades.


### Compilation

    git submodule init
    git submodule update
    make clean
    make


### Usage

#### Object detection

    yafdb-detect --algorithm algo input-image.tiff output-objects.yml

    Detects objects within input image. Detected objects are written to a yml file.

    General options:

    --algorithm algo: algorithm to use for object detection ('haar')

    Gnomonic projection options:

    --gnomonic: activate task
    --gnomonic-width 256: projection window width
    --gnomonic-aperture-x 30: horizontal projection aperture
    --gnomonic-aperture-y 30: vertical projection aperture

    Haar-cascades options:

    --haar-model class:file.xml: haar model file with class name (allowed multiple times)
    --haar-scale 1.1: haar reduction scale factor
    --haar-min-overlap 5: haar minimum detection overlap


#### Object preview

    yafdb-preview input-image.tiff input-objects.yml

    Preview detected objects in source image.


#### Object blurring

    yafdb-blur --algorithm algo input-image.tiff input-objects.yml output-image.tiff

    Blurs detected objects and write modified image as output.

    General options:

    --algorithm algo: algorithm to use for blurring ('default')


#### Performance validation

    yafdb-test input-image.tiff mask-image.tiff input-objects.yml

    Compute the detection error rate by comparing optimal area given in
    reference bitmap mask (black=none, white=object) to the detected area

### Copyright

Copyright (c) 2014 FOXEL SA - [http://foxel.ch](http://foxel.ch)<br />
This program is part of the FOXEL project <[http://foxel.ch](http://foxel.ch)>.

Please read the [COPYRIGHT.md](COPYRIGHT.md) file for more information.

### License

This program is licensed under the terms of the
[GNU Affero General Public License v3](http://www.gnu.org/licenses/agpl.html)
(GNU AGPL), with two additional terms. The content is licensed under the terms
of the
[Creative Commons Attribution-ShareAlike 4.0 International](http://creativecommons.org/licenses/by-sa/4.0/)
(CC BY-SA) license.

Please read <[http://foxel.ch/license](http://foxel.ch/license)> for more
information.
