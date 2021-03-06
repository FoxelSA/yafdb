## yafdb<br />Yet Another Face Detection and Blurring

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

### Table of Contents

- [Compilation](#compilation)
- [Usage](#usage)
- [Main programs](#main-programs)
  - [Object detection](#object-detection)
  - [Object export](#object-export)
  - [Object preview](#object-preview)
  - [Object validation](#object-validation)
- [Tools](#batch-tools)
  - [yafdb-create-directories](#yafdb-create-directories)
  - [yafdb-batch-blur](#yafdb-batch-blur)
  - [yafdb-batch-detect](#yafdb-batch-detect)
  - [yafdb-batch-extract](#yafdb-batch-extract)
  - [yafdb-batch-validate](#yafdb-batch-validate)
- [Controls](#controls)
- [Object blurring](#object-blurring)
- [Performance validation](#performance-validation)
- [Copyright](#copyright)
- [License](#license)

### Compilation

    git submodule init
    git submodule update --init --recursive
    make clean
    make
    sudo make install


### Usage

#### Main programs

##### Object detection

    yafdb-detect --algorithm algo input-image.tiff output-objects.yaml
    
    Detects objects within input image. Detected objects are written to a yaml file.
    
    General options:
    
    --full-invalid : Mark all detected objects as invalid
    --merge-valid-objects : Merge overlapping valid objects rectangles
    --merge-min-overlap 1 : Minimum occurrence of overlap to keep detected objects
    --algorithm algo : algorithm to use for object detection ('haar')
    
    Gnomonic projection options:
    
    --gnomonic               : activate task
    --gnomonic-width 2048    : projection window width
    --gnomonic-aperture-x 60 : horizontal projection aperture
    --gnomonic-aperture-y 60 : vertical projection aperture
    
    Filtering options:
    
    --filters-disable : Disable filtering
    --flter-ratio-min 0.7 : Minimum detected object ratio filtering threshold
    --flter-ratio-max 1.3 : Maximum detected object ratio filtering threshold
    --flter-size-max-width 3000  : Maximum detected object width filtering threshold
    --flter-size-max-height 3000  : Maximum detected object height filtering threshold
    
    Haar-cascades options:
    
    --haar-model class:file.xml[:minChild:maxChild]   : parent haar model file with class name (allowed multiple times)
    --haar-model class:file.xml:parentclass[:min:max] : child haar model file with class name (allowed multiple times)
    --haar-scale 1.1                                  : haar reduction scale factor
    --haar-min-overlap 3                              : haar minimum detection overlap



##### Object export

    yafdb-export input-image.tiff input-objects.yaml output-path/

    Export detected objects from source image into output folder (yaml+images).

    General options:

    --merge-disable: don't merge overlapping rectangles
    --merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects
    --format png : set exported object image format ('png', 'jpeg', 'tiff')


##### Object preview

    yafdb-preview input-image.tiff input-objects.yaml

    Preview detected objects in source image.

    General options:

    --merge-disable: don't merge overlapping rectangles
    --merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects
    --export output-image.tiff : export detected objects in image


##### Object validation

    yafdb-validate input-image.tiff [input-objects.yaml] output-objects.yaml
    
    Validate detected objects in source image.
    
    General options:
    
    --image-width 7134 : Default image width during validation
    --fullscreen : Start validation window in fullscreen
    --show-invalid-objects : Display invalid objects
    --merge-disable: don't merge overlapping rectangles
    --merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects
    --auto-validate : enable auto-validation instead of manual validation
    
    Gnomonic projection options:
    
    --gnomonic : activate gnomonic reprojection for visualization

#### Tools

##### yafdb-create-directories
Create default directory structure

    yafdb-create-directories [Base path]

##### yafdb-batch-blur
Perform blurring on all images

    yafdb-batch-blur [Base path]

##### yafdb-batch-detect
Perform object detection on all images

    yafdb-batch-detect <HAAR cascades folder> [Base Path]

##### yafdb-batch-extract
Perform objects extraction on all images

    yafdb-batch-extract [Base Path]

##### yafdb-batch-validate
Perform batch validation on all images

    yafdb-batch-validate [OPTIONS]

    -h --help           Prints this
    -i --ignore         Ignore validated images
    -c --convert        Write all yml files timestamps to state file
    -d --dir            Base directory

#### Controls

* Panorama view

        Left click on a rectangle => Edit
        Right click on a rectangle => Invalidate
        Left click then right click => Create a new zone (defined by the two previous selected points)

* Edit view

        Left click => Change rectangle position (Upper left point)
        Right click => Change rectangle size (Bottom right point)
        "t" => Edit class type
        "y" | "Enter" => Validate
        "n" | "Escape" => Invalidate

* Type config view

        "Escape" => Don't change the class type and close the window
        "y" | "Enter" => Validate


#### Object blurring

    yafdb-blur --algorithm algo input-image.tiff input-objects.yaml output-image.tiff

    Blurs detected objects and write modified image as output.

    General options:

    --algorithm algo : algorithm to use for blurring ('gaussian', 'progressive')
    --merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects

    Gaussian options:

    --gaussian-kernel 65 : gaussian kernel size
    --gaussian-steps 1 : gaussian blurring steps

    Resizing options:

    --resize-width 800: Resizing width
    --resize-height 600: Resizing height

#### Performance validation

    yafdb-test input-objects.yaml mask-image.png

    Compute the detection error rate by comparing optimal area given in
    reference bitmap mask (black=none, white=object) to the detected area
    given in input text file.

    General options:

    --merge-disable: don't merge overlapping rectangles
    --merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects
    --preview input-image.tiff: preview detection errors


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

You must read <[http://foxel.ch/license](http://foxel.ch/license)> for more
information about our Licensing terms and our Usage and Attribution guidelines.
