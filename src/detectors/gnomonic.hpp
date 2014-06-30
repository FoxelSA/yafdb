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


#ifndef __YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__
#define __YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__


#include "detector.hpp"


/**
 * Object detector with gnomonic projection.
 *
 */
class GnomonicProjectionDetector : public ObjectDetector {
protected:
    /** Underlying object detector */
    std::shared_ptr<ObjectDetector> detector;

    /** Projection window width */
    int width;

    /** Projection window height */
    int height;

    /** Projection window horizontal aperture in radian */
    double ax;

    /** Projection window vertical aperture in radian **/
    double ay;

    /** Projection window horizontal half-aperture in radian */
    double hax;

    /** Projection window vertical half-aperture in radian **/
    double hay;


public:
    /**
     * Empty constructor.
     */
    GnomonicProjectionDetector() : ObjectDetector(), width(512), height(512), ax(M_PI / 3), ay(M_PI / 3), hax(M_PI / 6), hay(M_PI / 6) {
    }

    /**
     * Default constructor.
     *
     * \param detector underlying object detector
     * \param width projection window width in pixels
     * \param ax projection window horizontal aperture in radian
     * \param ay projection window vertical aperture in radian
     */
    GnomonicProjectionDetector(const std::shared_ptr<ObjectDetector> &detector, int width, double ax = M_PI / 3, double ay = M_PI / 3) : ObjectDetector(), detector(detector), width(width), height(width * ay / ax), ax(ax), ay(ay), hax(ax / 2), hay(ay / 2) {
    }

    /**
     * Empty destructor.
     */
    virtual ~GnomonicProjectionDetector() {
    }


    /**
     * Enable detected object export.
     *
     * \param path target path for image files
     * \param suffix image file suffix (such as '.png')
     */
    virtual void setObjectExport(const std::string &path, const std::string &suffix);

    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const;

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects);
};


#endif //__YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__