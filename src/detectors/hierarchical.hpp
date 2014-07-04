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


#ifndef __YAFDB_DETECTORS_HIERARCHICAL_H_INCLUDE__
#define __YAFDB_DETECTORS_HIERARCHICAL_H_INCLUDE__


#include "detector.hpp"


/**
 * Object detector with hierarchy.
 *
 */
class HierarchicalObjectDetector : public ObjectDetector {
protected:
    /**
     * Child object detector configuration.
     *
     */
    typedef struct {
        /** Child object detector */
        std::shared_ptr<ObjectDetector> detector;

        /** Minimum required matches within parent (0 = any) */
        int minOccurences;

        /** Maximum required matches within parent (-1 = any) */
        int maxOccurences;
    } ObjectDetectorConfig;

    /** Parent object detector */
    std::shared_ptr<ObjectDetector> parent;

    /** Minimum total required matches within parent (0 = any) */
    int minOccurences;

    /** Maximum total required matches within parent (-1 = any) */
    int maxOccurences;

    /** Child object detectors */
    std::list<ObjectDetectorConfig> children;


public:
    /**
     * Empty constructor.
     */
    HierarchicalObjectDetector(const std::shared_ptr<ObjectDetector> &parent, int minOccurences = 0, int maxOccurences = -1) : ObjectDetector(), parent(parent), minOccurences(minOccurences), maxOccurences(maxOccurences) {
    }

    /**
     * Empty destructor.
     */
    virtual ~HierarchicalObjectDetector() {
    }


    /**
     * Add an underlying object detector applied to gnomonic reprojections.
     *
     * The detector memory will be automatically freed.
     *
     * \param detector pointer to detector to add
     */
    HierarchicalObjectDetector* addChildDetector(const std::shared_ptr<ObjectDetector> &detector, int minOccurences, int maxOccurences);

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


#endif //__YAFDB_DETECTORS_HIERARCHICAL_H_INCLUDE__