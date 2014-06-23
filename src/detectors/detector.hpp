/*
 * yafdb - Yet Another Face Detection and Bluring
 *
 * Copyright (c) 2014 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 *
 * Author(s):
 *
 * Antony Ducommun <nitro@tmsrv.org>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 * You are required to preserve legal notices and author attributions in
 * that material or in the Appropriate Legal Notices displayed by works
 * containing it.
 *
 * You are required to attribute the work as explained in the "Usage and
 * Attribution" section of <http://foxel.ch/license>.
 */

#ifndef __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__
#define __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__


#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>


/**
 * Detected object instance.
 *
 */
class DetectedObject {
public:
    /** Class of object */
    int clazz;

    /** Bounding box in source image */
    int x, y, width, height;


    /**
     * Empty constructor.
     */
    DetectedObject() : clazz(0), x(0), y(0), width(0), height(0) {
    }

    /**
     * Copy constructor.
     */
    DetectedObject(const DetectedObject &ref) : clazz(ref.clazz), x(ref.x), y(ref.y), width(ref.width), height(ref.height) {
    }
};


/**
 * Basic object detector (no algorithm implemented).
 *
 */
class ObjectDetector {
public:
    /**
     * Empty constructor.
     */
    ObjectDetector() {
    }

    /**
     * Empty destructor.
     */
    virtual ~ObjectDetector() {
    }


    /*
     * Called to display source image.
     *
     * \param source source image to display
     */
    virtual void preview(const cv::Mat &source) {
        cv::Mat preview;

        cv::resize(source, preview, cv::Size(), 0.25, 0.25);
        cv::namedWindow("preview", CV_WINDOW_NORMAL);
        cv::imshow("preview", preview);
        while (cv::waitKey(0) != '\n');

        cv::destroyWindow("preview");
        cv::waitKey(1);
    }

    /*
     * Called to display source image with detected objects.
     *
     * \param source source image to display
     * \param objects list of detected objects
     */
    virtual void preview(const cv::Mat &source, const std::vector<DetectedObject> &objects) {
        cv::Mat preview;

        cv::resize(source, preview, cv::Size(), 0.25, 0.25);

        // TODO: mark detected objects

        cv::namedWindow("preview", CV_WINDOW_NORMAL);
        cv::imshow("preview", preview);
        while (cv::waitKey(0) != '\n');

        cv::destroyWindow("preview");
        cv::waitKey(1);
    }


    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::vector<DetectedObject> &objects) {
        return true;
    }
};


#endif //__YAFDB_DETECTORS_DETECTOR_H_INCLUDE__