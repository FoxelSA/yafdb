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


#include "gnomonic.hpp"


void GnomonicProjectionDetector::setObjectExport(const std::string &path, const std::string &suffix) {
    if (this->detector) {
        this->detector->setObjectExport(path, suffix);
    }
}

bool GnomonicProjectionDetector::supportsColor() const {
    return !this->detector || this->detector->supportsColor();
}

bool GnomonicProjectionDetector::detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
    cv::Mat window(this->height, this->width, source.type());

    // scan the whole source image in eqr projection
    for (double y = M_PI / 2; y >= -M_PI / 2; y -= this->hay) {
        for (double x = 0; x < 2 * M_PI; x += this->hax) {
            // gnomonic projection of current area
            GnomonicTransform transform(window.cols, window.rows, this->ax, this->ay, x, y);

            transform.toGnomonic(source, window);

            // detect objects within reprojected area
            std::list<DetectedObject> window_objects;

            if (this->detector && !this->detector->detect(window, window_objects)) {
                return false;
            }

            // remap detected objects coordinates to eqr
            std::function<bool(DetectedObject &)> eqrMapper = [&] (DetectedObject &object) {
                BoundingBox eqrArea(BoundingBox::SPHERICAL);

                if (transform.toEqr(object.area, eqrArea)) {
                    object.area = eqrArea;

                    // map children
                    return std::all_of(object.children.begin(), object.children.end(), [&] (DetectedObject &child) {
                        return eqrMapper(child);
                    });
                }
                return false;
            };

            std::for_each(window_objects.begin(), window_objects.end(), [&] (DetectedObject &object) {
                if (eqrMapper(object)) {
                    objects.push_back(object);
                }
            });
        }
    }
    return true;
}
