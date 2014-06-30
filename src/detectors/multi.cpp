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


#include "multi.hpp"


MultiObjectDetector* MultiObjectDetector::addDetector(const std::shared_ptr<ObjectDetector> &detector) {
    this->detectors.push_back(detector);
    return this;
}


void MultiObjectDetector::setObjectExport(const std::string &path, const std::string &suffix) {
    std::for_each(this->detectors.begin(), this->detectors.end(), [&] (const std::shared_ptr<ObjectDetector> &detector) {
        detector->setObjectExport(path, suffix);
    });
}

bool MultiObjectDetector::supportsColor() const {
    return std::any_of(this->detectors.begin(), this->detectors.end(), [] (const std::shared_ptr<ObjectDetector> &detector) {
        return detector->supportsColor();
    });
}

bool MultiObjectDetector::detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
    cv::Mat graySource(source);

    return std::all_of(this->detectors.begin(), this->detectors.end(), [&] (const std::shared_ptr<ObjectDetector> &detector) {
        if (!detector->supportsColor()) {
            if (graySource.channels() != 1) {
                cv::cvtColor(source, graySource, cv::COLOR_RGB2GRAY);
                // cv::equalizeHist(graySource, graySource);
            }
            return detector->detect(graySource, objects);
        }
        return detector->detect(source, objects);
    });
}
