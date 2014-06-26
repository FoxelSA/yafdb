/*
 * yafdb - Yet Another Face Detection and Bluring
 *
 * Copyright (c) 2014 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 *
 * Author(s):
 *
 *      Antony Ducommun <nitro@tmsrv.org>
 *
 *
 * This file is part of the FOXEL project <http://foxel.ch>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 *      You are required to preserve legal notices and author attributions in
 *      that material or in the Appropriate Legal Notices displayed by works
 *      containing it.
 *
 *      You are required to attribute the work as explained in the "Usage and
 *      Attribution" section of <http://foxel.ch/license>.
 */


#ifndef __YAFDB_DETECTORS_HAAR_H_INCLUDE__
#define __YAFDB_DETECTORS_HAAR_H_INCLUDE__


#include "detector.hpp"


/**
 * OpenCV haar cascades object detector.
 *
 */
class HaarDetector : public ObjectDetector {
protected:
    /** Object class name */
    std::string className;

    /** Loaded classifier */
    cv::CascadeClassifier classifier;

    /** Scale factor */
    double scaleFactor;

    /** Minimum match overlap */
    int minOverlap;


public:
    /**
     * Empty constructor.
     */
    HaarDetector() : ObjectDetector(), className("object"), scaleFactor(1.1), minOverlap(5) {
    }

    /**
     * Default constructor.
     *
     * \param className object class name
     * \param modelFile haar model filename
     * \param scaleFactor haar reduction factor after each iteration
     * \param minOverlap minimum match overlap
     */
    HaarDetector(const std::string &className, const std::string &modelFile, double scaleFactor = 1.1, int minOverlap = 5) : ObjectDetector(), className(className), scaleFactor(scaleFactor), minOverlap(minOverlap) {
        this->classifier.load(modelFile);
    }

    /**
     * Empty destructor.
     */
    virtual ~HaarDetector() {
    }


    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColors() const {
        return false;
    }

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        std::vector<cv::Rect> window_objects;

        this->classifier.detectMultiScale(source, window_objects, this->scaleFactor, this->minOverlap, 0, cv::Size(10, 10), cv::Size());
        for (std::vector<cv::Rect>::const_iterator it = window_objects.begin(); it != window_objects.end(); ++it) {
            objects.push_back(DetectedObject(this->className, *it));
        }
        return true;
    }
};


#endif //__YAFDB_DETECTORS_HAAR_H_INCLUDE__
