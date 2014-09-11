## Cascade training how-to

### Requirements

* OpenCV with 'opencv_traincascade' utility. Should be compiled with TBB enabled for parallel
computation support.

* A positive dataset: a set of images more or less having the same width/height ratio of the
objects to detect. The image set must be placed in the sub-folder images/positive.

* A negative dataset: a (vast) collection of images not containing the objects to detect. These
images are placed in images/negative sub-folder.

* A background dataset (optional): used to generate variations of positive images.

### Setup

* Place background, negative and positive images in respective folders.

* Configure sample input ratio in train.sh

### Training

* Run train.sh and check train.log for progress/error

* Once completed, the output is in classifier/classifier.xml
