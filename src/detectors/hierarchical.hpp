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
    HierarchicalObjectDetector* addChildDetector(const std::shared_ptr<ObjectDetector> &detector, int minOccurences, int maxOccurences) {
        this->children.push_back({detector, minOccurences, maxOccurences});
        return this;
    }


    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const {
        return (this->parent && this->parent->supportsColor()) ||
            std::any_of(this->children.begin(), this->children.end(), [] (const ObjectDetectorConfig &config) {
                return (!config.detector || config.detector->supportsColor());
            });
    }

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        std::list<DetectedObject> parentObjects;
        cv::Mat graySource(source);

        if (this->parent) {
            if (this->parent->supportsColor()) {
                if (!this->parent->detect(source, parentObjects)) {
                    return false;
                }
            } else {
                if (graySource.channels() != 1) {
                    cv::cvtColor(source, graySource, cv::COLOR_RGB2GRAY);
                    // cv::equalizeHist(graySource, graySource);
                }
                if (!this->parent->detect(graySource, parentObjects)) {
                    return false;
                }
            }
        }

        // check children detectors
        std::for_each(parentObjects.begin(), parentObjects.end(), [&] (DetectedObject &object) {
            // note: only supports lookup within single region (doesn't work with eqr regions)
            cv::Rect area = object.area.rects(source.cols, source.rows)[0];
            cv::Mat region(source, area);
            cv::Mat grayRegion(region);

            // keep object?
            int matched = std::count_if(this->children.begin(), this->children.end(), [&] (ObjectDetectorConfig &config) {
                std::list<DetectedObject> childObjects;

                if (!config.detector) {
                    return true;
                }
                if (config.detector->supportsColor()) {
                    if (!config.detector->detect(region, childObjects)) {
                        return false;
                    }
                } else {
                    if (grayRegion.channels() != 1) {
                        cv::cvtColor(region, grayRegion, cv::COLOR_RGB2GRAY);
                        // cv::equalizeHist(grayRegion, grayRegion);
                    }
                    if (!config.detector->detect(grayRegion, childObjects)) {
                        return false;
                    }
                }

                // enforce constraints
                if ((int)childObjects.size() < config.minOccurences) {
                    // printf("rejected by childObjects: %d < %d\n", (int)childObjects.size(), config.minOccurences);
                    return false;
                }
                if (config.maxOccurences >= 0 && (int)childObjects.size() > config.maxOccurences) {
                    // printf("rejected by childObjects: %d > %d\n", (int)childObjects.size(), config.maxOccurences);
                    return false;
                }

                // remap coordinates
                std::for_each(childObjects.begin(), childObjects.end(), [&] (DetectedObject &childObject) {
                    childObject.move(area.x, area.y);
                });

                object.addChildren(childObjects);
                return true;
            });

            if (matched >= this->minOccurences && (this->maxOccurences <= 0 || matched <= this->maxOccurences)) {
                objects.push_back(object);
            }
        });
        return true;
    }
};


#endif //__YAFDB_DETECTORS_HIERARCHICAL_H_INCLUDE__