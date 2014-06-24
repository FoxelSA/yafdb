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

#ifndef __YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__
#define __YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__


#include <gnomonic-all.h>


#include "detector.hpp"


/**
 * Object detector with gnomonic projection.
 *
 */
class GnomonicProjectionDetector : public MultiDetector {
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


public:
    /**
     * Empty constructor.
     */
    GnomonicProjectionDetector() : MultiDetector(), width(512), height(512), ax(60.0), ay(60.0) {
        this->hax = this->ax / 2;
        this->hay = this->ay / 2;
    }

    /**
     * Default constructor.
     *
     * \param width projection window width in pixels
     * \param ax projection window horizontal aperture in degree
     * \param ay projection window vertical aperture in degree
     */
    GnomonicProjectionDetector(int width, double ax = 60.0, double ay = 60.0) : MultiDetector(), width(width), height(width * ay / ax), ax(ax), ay(ay) {
        this->hax = this->ax / 2;
        this->hay = this->ay / 2;
    }

    /**
     * Empty destructor.
     */
    virtual ~GnomonicProjectionDetector() {
    }


    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        cv::Mat windows(
            128 * (((180 - this->ay) / this->hay) + 1),
            128 * (((360 - this->ax) / this->hax) + 1),
            source.type()
        );
        cv::Mat window(this->height, this->width, source.type());
        int ix = 0, iy = 0;

        for (double y = 90 - this->hay; y >= -90 + this->hay; y -= this->hay) {
            ix = 0;
            for (double x = this->hax; x <= 360.0 - this->hax; x += this->hax) {
                cv::Mat grayWindow;
                cv::Mat region(
                    windows,
                    cv::Range(iy * 128, (iy + 1) * 128),
                    cv::Range(ix * 128, (ix + 1) * 128)
                );
                std::list<DetectedObject> window_objects;

                // auto eqrCoordinateMapper = [&] (const cv::Rect &rect, struct SphericalArea &area) {
                //     double northWestX, northWestY;
                //     double southEastX, southEastY;

                //     gnomonic_gtt_coord(
                //         &northWestX,
                //         &northWestY,
                //         source.cols,
                //         source.rows,
                //         x,
                //         y,
                //         window.cols,
                //         window.rows,
                //         source.cols,
                //         source.rows,
                //         0,
                //         0,
                //         rect.x,
                //         rect.y
                //     );
                //     gnomonic_gtt_coord(
                //         &southEastX,
                //         &southEastY,
                //         source.cols,
                //         source.rows,
                //         x,
                //         y,
                //         window.cols,
                //         window.rows,
                //         source.cols,
                //         source.rows,
                //         0,
                //         0,
                //         rect.x + rect.width,
                //         rect.y + rect.height
                //     );

                //     northWestX = northWestX / source.cols * 360.0;
                //     northWestY = northWestY / source.rows * 180.0 - 90.0;
                //     southEastX = southEastX / source.cols * 360.0;
                //     southEastY = southEastY / source.rows * 180.0 - 90.0;

                //     printf("rect: <%d ; %d> -> <%d ; %d>\n", rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
                //     printf("window: <%f ; %f> -> <%f ; %f>\n", x - this->hax, y - this->hay, x + this->hax, y + this->hay);
                //     printf("area: <%f ; %f> -> <%f ; %f>\n\n", northWestX, northWestY, southEastX, southEastY);

                //     area.northWest.azimuthAngle = northWestX;
                //     area.northWest.polarAngle = northWestY;
                //     area.southEast.azimuthAngle = southEastX;
                //     area.southEast.polarAngle = southEastY;
                // };

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
                    gnomonic_interp_bilinearf
                );

                if (!MultiDetector::detect(window, window_objects)) {
                    return false;
                }
                for (std::list<DetectedObject>::const_iterator it = window_objects.begin(); it != window_objects.end(); ++it) {
                    // TODO: remap area
                    // coordMapper(*it, area);
                    objects.push_back(DetectedObject((*it).className, (*it).area));

                    rectangle(window, (*it).area, cv::Scalar(0, 255, 255), 4);
                }

                cv::resize(window, region, region.size());
                cv::imshow("gnomonic", windows);
                cv::waitKey(10);

                ix++;
            }
            iy++;
        }

        while (cv::waitKey(0) != '\n');
        return true;
    }
};


#endif //__YAFDB_DETECTORS_GNOMONIC_H_INCLUDE__