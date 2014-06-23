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

#ifndef __YAFDB_DETECTORS_HAAR_H_INCLUDE__
#define __YAFDB_DETECTORS_HAAR_H_INCLUDE__


#include <gnomonic-all.h>


#include "detector.hpp"


/**
 * Vanilla opencv haar object detector.
 *
 */
class HaarDetector : public ObjectDetector {
protected:
    /** Projection window width */
    int width;

    /** Projection window height */
    int height;

    /** Projection window horizontal aperture in degree */
    double ax;

    /** Projection window vertical aperture in degree **/
    double ay;


public:
    /**
     * Empty constructor.
     */
    HaarDetector() : ObjectDetector(), width(1024), height(1024), ax(90.0), ay(90.0) {
    }

    /**
     * Empty destructor.
     */
    virtual ~HaarDetector() {
    }


    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::vector<DetectedObject> &objects) {
        cv::Mat window(this->width, this->height, source.type());
        double hax = this->ax / 2;
        double hay = this->ay / 2;

        for (double x = hax; x <= 360.0 - hax; x += hax) {
            for (double y = -90 + hay; y <= 90 - hay; y += hay) {
                gnomonic_etg(
                    source.data,
                    source.cols,
                    source.rows,
                    source.channels(),
                    window.data,
                    window.cols,
                    window.rows,
                    window.channels(),
                    x * ( M_PI / 180.0 ),
                    y * ( M_PI / 180.0 ),
                    hax * ( M_PI / 180.0 ),
                    hay * ( M_PI / 180.0 ),
                    gnomonic_interp_bicubicf
                );

                cv::namedWindow("window", CV_WINDOW_NORMAL);
                cv::imshow("window", window);
                while (cv::waitKey(0) <= 0);


            }
        }
        return true;
    }
};


#endif //__YAFDB_DETECTORS_HAAR_H_INCLUDE__