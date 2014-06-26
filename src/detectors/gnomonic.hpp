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


#include <gnomonic-all.h>

#include "detector.hpp"


// #define PREVIEW_SIZE 256


/**
 * Object detector with gnomonic projection.
 *
 */
class GnomonicProjectionDetector : public ObjectDetector {
protected:
    /** Underlying object detector */
    ObjectDetector* detector;

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
    GnomonicProjectionDetector() : ObjectDetector(), detector(NULL), width(512), height(512), ax(M_PI / 3), ay(M_PI / 3), hax(M_PI / 6), hay(M_PI / 6) {
    }

    /**
     * Default constructor.
     *
     * \param detector underlying object detector
     * \param width projection window width in pixels
     * \param ax projection window horizontal aperture in radian
     * \param ay projection window vertical aperture in radian
     */
    GnomonicProjectionDetector(ObjectDetector* detector, int width, double ax = M_PI / 3, double ay = M_PI / 3) : ObjectDetector(), detector(detector), width(width), height(width * ay / ax), ax(ax), ay(ay), hax(ax / 2), hay(ay / 2) {
    }

    /**
     * Empty destructor.
     */
    virtual ~GnomonicProjectionDetector() {
        delete detector;
    }


    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const {
        return (this->detector != NULL ? this->detector->supportsColor() : true);
    }

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        cv::Mat window(this->height, this->width, source.type());
        double tax = tan(this->hax);
        double tay = tan(this->hay);
        // cv::Mat windows(
        //     PREVIEW_SIZE * (((M_PI - this->ay) / this->hay) + 1),
        //     PREVIEW_SIZE * (((2 * M_PI - this->ax) / this->hax) + 1),
        //     source.type()
        // );
        // int ix = 0, iy = 0;

        // scan the whole source image in eqr projection
        for (double y = M_PI / 2 - this->hay; y >= -M_PI / 2 + this->hay; y -= this->hay) {
            // ix = 0;
            for (double x = this->hax; x <= 2 * M_PI - this->hax; x += this->hax) {
                // gnomonic projection of current area
                gnomonic_etg(
                    source.data,
                    source.cols,
                    source.rows,
                    source.channels(),
                    window.data,
                    window.cols,
                    window.rows,
                    window.channels(),
                    x,
                    y,
                    this->hax,
                    this->hay,
                    gnomonic_interp_bilinearf
                );

                // detect objects within reprojected area
                std::list<DetectedObject> window_objects;

                if (this->detector != NULL && !this->detector->detect(window, window_objects)) {
                    return false;
                }

                // remap detected objects coordinates to eqr
                double rotateDataZ[3][3] = {
                    {  cos(x), -sin(x),     0.0 },
                    {  sin(x),  cos(x),     0.0 },
                    {     0.0,     0.0,     1.0 }
                };
                double rotateDataY[3][3] = {
                    {  cos(y),     0.0,  sin(y) },
                    {     0.0,     1.0,     0.0 },
                    { -sin(y),     0.0,  cos(y) }
                };
                cv::Mat rotate(cv::Mat(3, 3, CV_64F, rotateDataZ) * cv::Mat(3, 3, CV_64F, rotateDataY));
                auto eqrCoordinateMapper = [&] (int gnomonic_x, int gnomonic_y, double &eqr_phi, double &eqr_theta) {
                    double ux = (2.0 * gnomonic_x / (this->width - 1) - 1.0) * tax;
                    double uy = (2.0 * gnomonic_y / (this->height - 1) - 1.0) * tay;
                    double p = cos(atan(sqrt(ux * ux + uy * uy)));
                    double positionData[3] = { p, ux * p, uy * p };
                    cv::Mat position(rotate * cv::Mat(3, 1, CV_64F, positionData));
                    double x = position.at<double>(0, 0);
                    double y = position.at<double>(1, 0);
                    double z = position.at<double>(2, 0);

                    eqr_phi = acos(x / sqrt(x * x + y * y));
                    if (y < 0) {
                        eqr_phi = 2.0 * M_PI - eqr_phi;
                    }
                    eqr_theta = -asin(z);
                };

                for (std::list<DetectedObject>::const_iterator it = window_objects.begin(); it != window_objects.end(); ++it) {
                    const BoundingBox &gnomonicArea = (*it).area;
                    BoundingBox eqrArea;

                    eqrCoordinateMapper(gnomonicArea.p1[0], gnomonicArea.p1[1], eqrArea.p1[0], eqrArea.p1[1]);
                    eqrCoordinateMapper(gnomonicArea.p2[0], gnomonicArea.p2[1], eqrArea.p2[0], eqrArea.p2[1]);

                    objects.push_back(DetectedObject((*it).className, eqrArea));

                    // rectangle(window, gnomonicArea.rect(), cv::Scalar(0, 255, 255), 4);
                }

                // cv::Mat region(
                //     windows,
                //     cv::Range(iy * PREVIEW_SIZE, (iy + 1) * PREVIEW_SIZE),
                //     cv::Range(ix * PREVIEW_SIZE, (ix + 1) * PREVIEW_SIZE)
                // );

                // cv::resize(window, region, region.size());
                // cv::namedWindow("gnomonic", CV_WINDOW_NORMAL);
                // cv::imshow("gnomonic", windows);
                // cv::waitKey(10);
                // ix++;
            }
            // iy++;
        }
        return true;
    }
};


#endif //__YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__
