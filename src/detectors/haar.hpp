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

    /** Projection window horizontal half-aperture in degree */
    double hax;

    /** Projection window vertical half-aperture in degree **/
    double hay;


    virtual void run(cv::CascadeClassifier &classifier, const cv::Mat &window, std::vector<cv::Rect> &objects, const cv::Scalar &color, cv::Mat &preview) {
        classifier.detectMultiScale(window, objects, 1.05, 5, 0, cv::Size(10, 10), cv::Size(this->width / 2, this->height / 2));
        for (std::vector<cv::Rect>::iterator it = objects.begin(); it != objects.end(); ++it) {
            rectangle(preview, *it, color, 4);
        }
    }


public:
    /**
     * Empty constructor.
     */
    HaarDetector() : ObjectDetector(), width(1024), height(1024), ax(60.0), ay(60.0) {
        this->hax = this->ax / 2;
        this->hay = this->ay / 2;
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
        cv::Mat windows(
            256 * (((180 - this->ay) / this->hay) + 1),
            256 * (((360 - this->ax) / this->hax) + 1),
            source.type()
        );
        cv::Mat window(this->height, this->width, source.type());
        cv::CascadeClassifier haar1, haar2, haar3, haar4, haar5;
        int ix = 0, iy = 0;

        haar1.load("haarcascades/haarcascade_frontalface_default.xml");
        haar2.load("haarcascades/haarcascade_frontalface_alt.xml");
        haar3.load("haarcascades/haarcascade_frontalface_alt2.xml");
        haar4.load("haarcascades/haarcascade_frontalface_alt_tree.xml");
        haar5.load("haarcascades/haarcascade_profileface.xml");
        for (double y = 90 - this->hay; y >= -90 + this->hay; y -= this->hay) {
            ix = 0;
            for (double x = this->hax; x <= 360.0 - this->hax; x += this->hax) {
                cv::Mat grayWindow;
                std::vector<cv::Rect> window_objects;
                cv::Mat region(
                    windows,
                    cv::Range(iy * 256, (iy + 1) * 256),
                    cv::Range(ix * 256, (ix + 1) * 256)
                );

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
                    this->hax * ( M_PI / 180.0 ),
                    this->hay * ( M_PI / 180.0 ),
                    gnomonic_interp_bicubicf
                );

                cv::cvtColor(window, grayWindow, cv::COLOR_RGB2GRAY);
                cv::equalizeHist(grayWindow, grayWindow);

                this->run(haar1, window, window_objects, cv::Scalar(255, 0, 0), window);
                this->run(haar2, window, window_objects, cv::Scalar(0, 255, 0), window);
                this->run(haar3, window, window_objects, cv::Scalar(0, 0, 255), window);
                this->run(haar4, window, window_objects, cv::Scalar(255, 255, 0), window);
                this->run(haar5, window, window_objects, cv::Scalar(255, 0, 255), window);

                cv::resize(window, region, region.size());
                cv::imshow("window", windows);
                cv::waitKey(10);

                ix++;
            }
            iy++;
        }

        while (cv::waitKey(0) != '\n');
        return true;
    }
};


#endif //__YAFDB_DETECTORS_HAAR_H_INCLUDE__