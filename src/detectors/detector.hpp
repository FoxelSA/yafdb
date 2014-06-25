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

#ifndef __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__
#define __YAFDB_DETECTORS_DETECTOR_H_INCLUDE__


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
     * \return opencv rectangle
     */
    cv::Rect eqrRect(int width, int height) const {
        int x1 = (int)(this->p1[0] / (2 * M_PI) * (width - 1));
        int y1 = (int)((-this->p1[1] + M_PI / 2) / M_PI * (height - 1));
        int x2 = (int)(this->p2[0] / (2 * M_PI) * (width - 1));
        int y2 = (int)((-this->p2[1] + M_PI / 2) / M_PI * (height - 1));

        return cv::Rect(x1, y1, x2 - x1, y2 - y1);
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