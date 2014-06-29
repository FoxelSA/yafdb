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

#include <gnomonic-all.h>


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
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const {
        return !this->detector || this->detector->supportsColor();
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

        // scan the whole source image in eqr projection
        for (double y = M_PI / 2; y >= -M_PI / 2; y -= this->hay) {
            for (double x = 0; x <= 2 * M_PI; x += this->hax) {
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

                if (this->detector && !this->detector->detect(window, window_objects)) {
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
                    eqr_theta = asin(z);
                };
                std::function<void(DetectedObject &)> eqrMapper = [&] (DetectedObject &object) {
                    // map bounding box to eqr coordinates
                    const BoundingBox &gnomonicArea = object.area;
                    BoundingBox eqrArea(BoundingBox::SPHERICAL);

                    eqrCoordinateMapper((int)gnomonicArea.p1.x, (int)gnomonicArea.p1.y, eqrArea.p1.x, eqrArea.p1.y);
                    eqrCoordinateMapper((int)gnomonicArea.p2.x, (int)gnomonicArea.p2.y, eqrArea.p2.x, eqrArea.p2.y);

                    // swap bounding box coordinates if necessary
                    if (eqrArea.p1.x > eqrArea.p2.x && (2 * M_PI - eqrArea.p1.x + eqrArea.p2.x) > this->ax) {
                        double tmp = eqrArea.p1.x;

                        eqrArea.p1.x = eqrArea.p2.x;
                        eqrArea.p2.x = tmp;
                    }
                    if (eqrArea.p1.y > eqrArea.p2.y && (M_PI - eqrArea.p1.y + eqrArea.p2.y) > this->ay) {
                        double tmp = eqrArea.p1.y;

                        eqrArea.p1.y = eqrArea.p2.y;
                        eqrArea.p2.y = tmp;
                    }
                    object.area = eqrArea;

                    // map children
                    std::for_each(object.children.begin(), object.children.end(), [&] (DetectedObject &child) {
                        eqrMapper(child);
                    });
                };

                std::for_each(window_objects.begin(), window_objects.end(), [&] (DetectedObject &object) {
                    eqrMapper(object);
                    objects.push_back(object);
                });
            }
        }
        return true;
    }
};


#endif //__YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__