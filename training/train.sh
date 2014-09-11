#!/bin/sh


###############
# Configuration
###############

# Width of training samples
WIDTH=80

# Height of training samples
HEIGHT=40

# Type of feature to use
#
# HAAR: haar wavelets
# LBP: local binary patterns
# HOG: histogram of oriented gradients
FEATURES=LBP

# Type of boosted classifiers
#
# GAB: gentle adaboost
# LB: logitboost
# RAB: real adaboost
# DAB: discrete adaboost
BOOSTED_CLASSIFIERS=GAB

# Minimum hit rate per stage
MINHITRATE=0.995

# Maximum false alarm rate per stage
MAXFALSEALARM=0.5

# Number of stages
NUMSTAGES=20

# Number of samples to generate (variations)
#
# By default, it's 10x the input samples
SAMPLES=`expr $(find images/positive/ -type f | wc -l) \* 10`

# Maximum cache buffer size (in MB)
BUFFERSIZE=2048


###############
# Positive sample preprocessing
###############

echo "=========================" >> train.log
echo "Creating samples..." >> train.log
date "+%Y-%m-%d %H:%M:%S" >> train.log
echo "=========================" >> train.log

cd images/
find background/ -type f -not -name .gitkeep > background.lst
find positive/ -type f -not -name .gitkeep > positive.lst
find negative/ -type f -not -name .gitkeep > negative.lst
cd ..

if [ ! -s images/positive.lst ];
then
	echo "no input images!" >> train.log
	echo >> train.log
	echo >> train.log
	exit 1
fi

# if [ ! -s images/background.lst ];
# then
# 	echo "no background images!" >> train.log
# 	echo >> train.log
# 	echo >> train.log
# 	exit 1
# fi

if [ ! -x ../yafdb-gendataset ];
then
	echo "yafdb-gendataset not found!" >> train.log
	echo >> train.log
	echo >> train.log
	exit 2
fi

../yafdb-gendataset \
	--count $SAMPLES \
	--width $WIDTH \
	--height $HEIGHT \
	--rotate-stddev-x 2 \
	--rotate-stddev-y 2 \
	--rotate-stddev-z 2 \
	--luminosity-stddev 1 \
	images/positive.lst \
	images/samples.vec \
	>> train.log 2>&1

#	--backgrounds images/background.lst \

if [ $? -ne 0 ];
then
	echo "yafdb-gendataset failed!"
	echo >> train.log
	echo >> train.log
	exit 3
fi


###############
# Cascade training
###############

echo "=========================" >> train.log
echo "Training on samples..." >> train.log
date "+%Y-%m-%d %H:%M:%S" >> train.log
echo "=========================" >> train.log

if [ ! -s images/negative.lst ];
then
	echo "no negative images!" >> train.log
	echo >> train.log
	echo >> train.log
	exit 1
fi

NUMPOS=`expr $SAMPLES \* 800 / 1000`
NUMNEG=`wc -l images/negative.lst | grep -E -o "^[0-9]+"`

rm -f classifier/*.xml
opencv_traincascade \
	-vec images/samples.vec \
	-bg images/negative.lst \
	-w $WIDTH \
	-h $HEIGHT \
	-stageType BOOST \
	-featureType $FEATURES \
	-mode ALL \
	-bt $BOOSTED_CLASSIFIERS \
	-minHitRate $MINHITRATE \
	-maxFalseAlarmRate $MAXFALSEALARM \
	-numPos $NUMPOS \
	-numNeg $NUMNEG \
	-numStages $NUMSTAGES \
	-precalcValBufSize $BUFFERSIZE \
	-precalcIdxBufSize 2048 \
	-data classifier/ \
	>> train.log 2>&1

#	-weightTrimRate 0.95 \
#	-maxDepth 5 \
#	-maxWeakCount 50 \

if [ $? -ne 0 ];
then
	echo "opencv_traincascade failed!"
	echo >> train.log
	echo >> train.log
	exit 6
fi

echo >> train.log
echo >> train.log
exit 0
