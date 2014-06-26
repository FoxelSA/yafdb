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


#ifndef __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__
#define __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__


#include <bitset>
#include <list>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>


/**
 * A generic bounding box.
 *
 */
class BoundingBox {
public:
    /** Top-left / north-west point */
    double p1[2];

    /** Bottom-right / south-east point */
    double p2[2];


    /**
     * Empty constructor.
     */
    BoundingBox() : p1 { 0, 0 }, p2 { 0, 0 } {
    }

    /**
     * Default constructor.
     *
     * \param x1 x-coordinate of first point
     * \param y1 y-coordinate of first point
     * \param x2 x-coordinate of second point
     * \param y2 y-coordinate of second point
     */
    BoundingBox(double x1, double y1, double x2, double y2) : p1 { x1, y1 }, p2 { x2, y2 } {
    }

    /**
     * Default constructor.
     *
     * \param x1 x-coordinate of first point
     * \param y1 y-coordinate of first point
     * \param x2 x-coordinate of second point
     * \param y2 y-coordinate of second point
     */
    BoundingBox(const cv::Rect &rect) : p1 { (double)rect.x, (double)rect.y }, p2 { (double)(rect.x + rect.width), (double)(rect.y + rect.height) } {
    }

    /**
     * Default constructor.
     *
     * \param x1 x-coordinate of first point
     * \param y1 y-coordinate of first point
     * \param x2 x-coordinate of second point
     * \param y2 y-coordinate of second point
     */
    BoundingBox(const cv::Rect_<double> &rect) : p1 { rect.x, rect.y }, p2 { rect.x + rect.width, rect.y + rect.height } {
    }

    /**
     * Storage constructor.
     *
     * \param node storage node to read from
     */
    BoundingBox(const cv::FileNode &node) {
        cv::Rect_<double> rect;

        node >> rect;
        this->p1[0] = rect.x;
        this->p1[1] = rect.y;
        this->p2[0] = rect.x + rect.width;
        this->p2[1] = rect.y + rect.height;
    }

    /**
     * Copy constructor.
     *
     * \param ref reference object
     */
    BoundingBox(const BoundingBox &ref) : p1 { ref.p1[0], ref.p1[1] }, p2 { ref.p2[0], ref.p2[1] } {
    }


    /**
     * Check if other bounding box overlap, and if yes, merge other area
     * into this one.
     *
     * \param other other bounding box
     * \return true if overlap detected, false otherwise
     */
    bool mergeIfOverlap(const BoundingBox &other) {
        double ax1 = this->p1[0];
        double bx1 = other.p1[0];
        double ay1 = this->p1[1];
        double by1 = other.p1[1];
        double ax2 = this->p2[0];
        double bx2 = other.p2[0];
        double ay2 = this->p2[1];
        double by2 = other.p2[1];
        double maxx = ax1;
        double maxy = ay1;

        if (maxx < bx1) {
            maxx = bx1;
        }
        if (maxx < ax2) {
            maxx = ax2;
        }
        if (maxx < bx2) {
            maxx = bx2;
        }
        if (maxy < by1) {
            maxy = by1;
        }
        if (maxy < ay2) {
            maxy = ay2;
        }
        if (maxy < by2) {
            maxy = by2;
        }

        if (this->p1[0] > this->p2[0]) {
            ax2 += maxx;
        }
        if (other.p1[0] > other.p2[0]) {
            bx2 += maxx;
        }
        if (this->p1[1] > this->p2[1]) {
            ay2 += maxy;
        }
        if (other.p1[1] > other.p2[1]) {
            by2 += maxy;
        }

        if (ax1 > bx2 || ax2 < bx1 ||
            ay1 > by2 || ay2 < by1) {
            return false;
        }
        if (ax1 > bx1) {
            ax1 = bx1;
        }
        if (ay1 > by1) {
            ay1 = by1;
        }
        if (ax2 < bx2) {
            ax2 = bx2;
        }
        if (ay2 < by2) {
            ay2 = by2;
        }
        this->p1[0] = ax1;
        this->p1[1] = ay1;
        if (this->p1[0] > this->p2[0]) {
            this->p2[0] = ax2 - maxx;
        } else {
            this->p2[0] = ax2;
        }
        if (this->p1[1] > this->p2[1]) {
            this->p2[1] = ay2 - maxy;
        } else {
            this->p2[1] = ay2;
        }
        return true;
    }


    /**
     * Convert bounding box to opencv rectangle.
     *
     * \return opencv rectangle
     */
    cv::Rect_<double> rect() const {
        return cv::Rect_<double>(
            this->p1[0],
            this->p1[1],
            this->p2[0] - this->p1[0],
            this->p2[1] - this->p1[1]
        );
    }

    /**
     * Convert bounding box to opencv rectangle assuming bounding box is given
     * in spherical coordinates.
     *
     * \return opencv rectangle(s)
     */
    std::vector<cv::Rect> eqrRects(int width, int height) const {
        std::vector<cv::Rect> v;
        int x1 = (int)(this->p1[0] / (2 * M_PI) * (width - 1));
        int y1 = (int)((this->p1[1] + M_PI / 2) / M_PI * (height - 1));
        int x2 = (int)(this->p2[0] / (2 * M_PI) * (width - 1));
        int y2 = (int)((this->p2[1] + M_PI / 2) / M_PI * (height - 1));

        if (x1 > x2) {
            if (y1 > y2) {
                v.push_back(cv::Rect(x1, y1, width - x1, height - y2));
                v.push_back(cv::Rect(x1,  0, width - x1,      y2 + 1));
                v.push_back(cv::Rect(0,  y1,     x2 + 1, height - y2));
                v.push_back(cv::Rect(0,   0,     x2 + 1,      y2 + 1));
            } else {
                v.push_back(cv::Rect(x1,  y1, width - x1, y2 - y1 + 1));
                v.push_back(cv::Rect(0,   y1,     x2 + 1, y2 - y1 + 1));
            }
        } else if (y1 > y2) {
            v.push_back(cv::Rect(x1, y1, x2 - x1 + 1, height - y1));
            v.push_back(cv::Rect(x1,  0, x2 - x1 + 1,      y2 + 1));
        } else {
            v.push_back(cv::Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1));
        }
        return v;
    }
};


/**
 * Detected object instance.
 *
 */
class DetectedObject {
public:
    /** Class of object */
    std::string className;

    /** Rough bounding area in source image */
    BoundingBox area;


    /**
     * Empty constructor.
     */
    DetectedObject() {
    }

    /**
     * Default constructor.
     *
     * \param className object class name
     * \param area bounding area
     */
    DetectedObject(const std::string &className, const BoundingBox &area) : className(className), area(area) {
    }

    /**
     * Storage constructor.
     *
     * \param node storage node to read from
     */
    DetectedObject(const cv::FileNode &node) : className(node["className"]), area(node["area"]) {
    }

    /**
     * Copy constructor.
     */
    DetectedObject(const DetectedObject &ref) : className(ref.className), area(ref.area) {
    }


    /**
     * Write detected object to storage.
     *
     * \param fs storage to write to
     */
    void write(cv::FileStorage &fs) const {
        fs << "{";
        fs << "className" << this->className;
        fs << "area" << this->area.rect();
        fs << "}";
    }
};


/**
 * Basic object detector (no algorithm implemented).
 *
 */
class ObjectDetector {
public:
    /**
     * Empty constructor.
     */
    ObjectDetector() {
    }

    /**
     * Empty destructor.
     */
    virtual ~ObjectDetector() {
    }


    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const {
        return true;
    }

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        return false;
    }


    /**
     * Load detected objects from yml file.
     *
     * \param file yml filename
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    static bool load(const std::string &file, std::list<DetectedObject> &objects) {
        cv::FileStorage fs(file, cv::FileStorage::READ);

        if (!fs.isOpened()) {
            return false;
        }

        cv::FileNode objectsNode = fs["objects"];

        for (cv::FileNodeIterator it = objectsNode.begin(); it != objectsNode.end(); ++it) {
            objects.push_back(DetectedObject(*it));
        }
        return true;
    }

    /**
     * Merge overlapping detected objects.
     *
     * \param objects detected objects
     * \param result output list of merge objects
     */
    static void merge(const std::list<DetectedObject> &objects, std::list<DetectedObject> &result) {
        std::vector<DetectedObject> v(objects.begin(), objects.end());
        std::set<unsigned int> used;

        for (unsigned int i = 0; i < v.size(); i++) {
            if (used.find(i) != used.end()) {
                continue;
            }
            used.insert(i);

            BoundingBox area(v[i].area);
            std::set<std::string> classNames;

            classNames.insert(v[i].className);
            for (unsigned int j = i + 1; j < v.size(); j++) {
                if (used.find(j) != used.end()) {
                    continue;
                }
                if (area.mergeIfOverlap(v[j].area)) {
                    classNames.insert(v[j].className);
                    used.insert(j);
                    j = i;
                }
            }

            std::string className;

            for (auto it = classNames.begin(); it != classNames.end(); ++it) {
                if (!className.empty()) {
                    className.append(":");
                }
                className.append(*it);
            }

            result.push_back(DetectedObject(className, area));
        }
    }
};


/**
 * Object detector with multiple underlying detectors.
 *
 */
class MultiDetector : public ObjectDetector {
protected:
    /** Underlying object detectors */
    std::list<ObjectDetector*> detectors;


public:
    /**
     * Empty constructor.
     */
    MultiDetector() : ObjectDetector() {
    }

    /**
     * Empty destructor.
     */
    virtual ~MultiDetector() {
        while (!this->detectors.empty()) {
            delete this->detectors.back();
            this->detectors.pop_back();
        }
    }


    /**
     * Add an underlying object detector applied to gnomonic reprojections.
     *
     * The detector memory will be automatically freed.
     *
     * \param detector pointer to detector to add
     */
    MultiDetector* addDetector(ObjectDetector* detector) {
        this->detectors.push_back(detector);
        return this;
    }


    /**
     * Check if this object detector supports color images.
     *
     * \return true if detector works with color images, false otherwise.
     */
    virtual bool supportsColor() const {
        for (std::list<ObjectDetector*>::const_iterator it = this->detectors.begin(); it != this->detectors.end(); ++it) {
            if ((*it)->supportsColor()) {
                return true;
            }
        }
        return false;
    }

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
        cv::Mat graySource(source);

        for (std::list<ObjectDetector*>::const_iterator it = this->detectors.begin(); it != this->detectors.end(); ++it) {
            if ((*it)->supportsColor()) {
                if (!(*it)->detect(source, objects)) {
                    return false;
                }
            } else {
                if (graySource.channels() != 1) {
                    cv::cvtColor(source, graySource, cv::COLOR_RGB2GRAY);
                    // cv::equalizeHist(graySource, graySource);
                }
                if (!(*it)->detect(graySource, objects)) {
                    return false;
                }
            }
        }
        return true;
    }
};


#endif //__YAFDB_DETECTORS_DETECTOR_H_INCLUDE__
