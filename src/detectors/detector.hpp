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


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <sys/stat.h>

#include <bitset>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>


#define CLAMP(x, a, b)  MIN(MAX(a, x), b)


/**
 * A generic bounding box.
 *
 */
class BoundingBox {
public:
    /**
     * Supported coordinate systems.
     *
     */
    enum CoordinateSystem { CARTESIAN = 1, SPHERICAL };


    /** Coordinate system type */
    CoordinateSystem system;

    /** Top-left / north-west point */
    cv::Point2d p1;

    /** Bottom-right / south-east point */
    cv::Point2d p2;


    /**
     * Empty constructor.
     */
    BoundingBox() : system(CARTESIAN) {
    }

    /**
     * Default constructor.
     *
     * \param system coordinate system
     */
    BoundingBox(CoordinateSystem system) : system(system) {
    }

    /**
     * Default constructor.
     *
     * \param system coordinate system
     * \param x1 x-coordinate of first point
     * \param y1 y-coordinate of first point
     * \param x2 x-coordinate of second point
     * \param y2 y-coordinate of second point
     */
    BoundingBox(CoordinateSystem system, double x1, double y1, double x2, double y2) : system(system), p1(x1, y1), p2(x2, y2) {
    }

    /**
     * Default constructor.
     *
     * \param system coordinate system
     * \param p1 first point
     * \param p2 second point
     */
    BoundingBox(CoordinateSystem system, const cv::Point2d &p1, const cv::Point2d &p2) : system(system), p1(p1), p2(p2) {
    }

    /**
     * Default constructor.
     *
     * \param system coordinate system
     * \param rect rectangle
     */
    BoundingBox(const cv::Rect &rect) : system(CARTESIAN), p1(rect.tl().x, rect.tl().y), p2(rect.br().x, rect.br().y) {
    }

    /**
     * Copy constructor.
     *
     * \param ref reference object
     */
    BoundingBox(const BoundingBox &ref) : system(ref.system), p1(ref.p1), p2(ref.p2) {
    }

    /**
     * Storage constructor.
     *
     * \param node storage node to read from
     */
    BoundingBox(const cv::FileNode &node) {
        int system;

        node["system"] >> system;
        node["p1"] >> this->p1;
        node["p2"] >> this->p2;
        this->system = (CoordinateSystem)system;
    }


    /**
     * Write area to storage.
     *
     * \param fs storage to write to
     */
    void write(cv::FileStorage &fs) const {
        fs << "{";
            fs << "system" << (int)this->system;
            fs << "p1" << this->p1;
            fs << "p2" << this->p2;
        fs << "}";
    }

    /**
     * Move coordinates.
     *
     * \param x x-axis translation
     * \param y y-axis translation
     */
    void move(double x, double y) {
        this->p1.x += x;
        this->p1.y += y;
        this->p2.x += x;
        this->p2.y += y;
    }

    /**
     * Check if other bounding box overlap, and if yes, merge other area
     * into this one.
     *
     * \param other other bounding box
     * \return true if overlap detected, false otherwise
     */
    bool mergeIfOverlap(const BoundingBox &other) {
        double ax1 = this->p1.x;
        double bx1 = other.p1.x;
        double ay1 = this->p1.y;
        double by1 = other.p1.y;
        double ax2 = this->p2.x;
        double bx2 = other.p2.x;
        double ay2 = this->p2.y;
        double by2 = other.p2.y;

        switch (this->system) {
        case CARTESIAN:
            if (ax1 > bx2 || ax2 < bx1 ||
                ay1 > by2 || ay2 < by1) {
                return false;
            }

            ax1 = MIN(ax1, bx1);
            ay1 = MIN(ay1, by1);
            ax2 = MAX(ax2, bx2);
            ay2 = MAX(ay2, by2);

            this->p1.x = ax1;
            this->p1.y = ay1;
            this->p2.x = ax2;
            this->p2.y = ay2;
            return true;

        case SPHERICAL:
            {
                double maxx = ax1;
                double maxy = ay1;

                maxx = MAX(maxx, bx1);
                maxx = MAX(maxx, ax2);
                maxx = MAX(maxx, bx2);
                maxy = MAX(maxy, by1);
                maxy = MAX(maxy, ay2);
                maxy = MAX(maxy, by2);

                if (this->p1.x > this->p2.x) {
                    ax2 += maxx;
                }
                if (other.p1.x > other.p2.x) {
                    bx2 += maxx;
                }
                if (this->p1.y > this->p2.y) {
                    ay2 += maxy;
                }
                if (other.p1.y > other.p2.y) {
                    by2 += maxy;
                }

                if (ax1 > bx2 || ax2 < bx1 ||
                    ay1 > by2 || ay2 < by1) {
                    return false;
                }

                ax1 = MIN(ax1, bx1);
                ay1 = MIN(ay1, by1);
                ax2 = MAX(ax2, bx2);
                ay2 = MAX(ay2, by2);

                this->p1.x = ax1;
                this->p1.y = ay1;
                if (this->p1.x > this->p2.x) {
                    this->p2.x = ax2 - maxx;
                } else {
                    this->p2.x = ax2;
                }
                if (this->p1.y > this->p2.y) {
                    this->p2.y = ay2 - maxy;
                } else {
                    this->p2.y = ay2;
                }
            }
            return true;
        }
        return false;
    }

    /**
     * Convert bounding box to opencv rectangle(s). If the coordinate system is
     * spherical, multiple rectangles might be returned to cover the area.
     *
     * \param width target image width
     * \param height target image height
     * \return opencv rectangle(s)
     */
    std::vector<cv::Rect> rects(int width, int height) const {
        std::vector<cv::Rect> v;

        switch (this->system) {
        case CARTESIAN:
            {
                int x1 = CLAMP((int)this->p1.x, 0, width);
                int y1 = CLAMP((int)this->p1.y, 0, height);
                int x2 = CLAMP((int)this->p2.x, 0, width);
                int y2 = CLAMP((int)this->p2.y, 0, height);

                v.push_back(cv::Rect(x1, y1, x2 - x1, y2 - y1));
            }
            break;

        case SPHERICAL:
            {
                int x1 = (int)(this->p1.x / (2 * M_PI) * width);
                int y1 = (int)((this->p1.y + M_PI / 2) / M_PI * height);
                int x2 = (int)(this->p2.x / (2 * M_PI) * width);
                int y2 = (int)((this->p2.y + M_PI / 2) / M_PI * height);

                if (x1 > x2) {
                    if (y1 > y2) {
                        v.push_back(cv::Rect(x1, y1, width - x1, height - y2));
                        v.push_back(cv::Rect(0,  y1,         x2, height - y2));
                        v.push_back(cv::Rect(x1,  0, width - x1,          y2));
                        v.push_back(cv::Rect(0,   0,         x2,          y2));
                    } else {
                        v.push_back(cv::Rect(x1, y1, width - x1,     y2 - y1));
                        v.push_back(cv::Rect(0,  y1,         x2,     y2 - y1));
                    }
                } else if (y1 > y2) {
                    v.push_back(cv::Rect(x1, y1, x2 - x1, height - y1));
                    v.push_back(cv::Rect(x1,  0, x2 - x1,          y2));
                } else {
                    v.push_back(cv::Rect(x1, y1, x2 - x1, y2 - y1));
                }
            }
            break;
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

    /** Children objects */
    std::list<DetectedObject> children;


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
     * Default constructor.
     *
     * \param className object class name
     * \param area bounding area
     * \param children children objects
     */
    DetectedObject(const std::string &className, const BoundingBox &area, const std::list<DetectedObject> &children) : className(className), area(area), children(children) {
    }

    /**
     * Copy constructor.
     */
    DetectedObject(const DetectedObject &ref) : className(ref.className), area(ref.area), children(ref.children) {
    }

    /**
     * Storage constructor.
     *
     * \param node storage node to read from
     */
    DetectedObject(const cv::FileNode &node) : className(node["className"]), area(node["area"]) {
        auto childrenNode = node["children"];

        for (auto it = childrenNode.begin(); it != childrenNode.end(); ++it) {
            this->children.push_back(*it);
        }
    }


    /**
     * Write detected object to storage.
     *
     * \param fs storage to write to
     */
    void write(cv::FileStorage &fs) const {
        fs << "{";
            fs << "className" << this->className;
            fs << "area";
            this->area.write(fs);
            if (!this->children.empty()) {
                fs << "children" << "[";
                    for (auto it = this->children.begin(); it != this->children.end(); ++it) {
                        (*it).write(fs);
                    }
                fs << "]";
            }
        fs << "}";
    }

    /**
     * Add a child detection.
     *
     * \param child child object
     */
    void addChild(const DetectedObject &child) {
        this->children.push_back(child);
    }

    /**
     * Add children detection.
     *
     * \param children child objects
     */
    void addChildren(const std::list<DetectedObject> &children) {
        this->children.insert(this->children.end(), children.begin(), children.end());
    }

    /**
     * Move object coordinates.
     *
     * \param x x-axis translation
     * \param y y-axis translation
     */
    void move(double x, double y) {
        this->area.move(x, y);
        for (auto it = this->children.begin(); it != this->children.end(); ++it) {
            (*it).move(x, y);
        }
    }
};


/**
 * Basic object detector (no algorithm implemented).
 *
 */
class ObjectDetector {
protected:
    /** Enable object export */
    bool exportEnable;

    /** Object export path */
    std::string exportPath;

    /** Object export filename suffix */
    std::string exportSuffix;


    /**
     * Get detected object region.
     *
     * \param source source image to scan for objects
     * \param object detected object
     * \return detected object region
     */
    cv::Mat getObjectRegion(const cv::Mat &source, const DetectedObject &object) {
        auto rects = object.area.rects(source.cols, source.rows);

        if (rects.size() == 4) {
            auto rect1 = rects[0];
            auto rect2 = rects[1];
            auto rect3 = rects[2];
            auto rect4 = rects[3];
            cv::Mat region(rect1.height + rect3.height, rect1.width + rect2.width, source.type());
            cv::Mat r1(region, cv::Rect(          0,            0, rect1.height, rect1.width));
            cv::Mat r2(region, cv::Rect(rect1.width,            0, rect2.height, rect2.width));
            cv::Mat r3(region, cv::Rect(          0, rect1.height, rect3.height, rect3.width));
            cv::Mat r4(region, cv::Rect(rect1.width, rect1.height, rect4.height, rect4.width));

            cv::Mat(source, rect1).copyTo(r1);
            cv::Mat(source, rect2).copyTo(r2);
            cv::Mat(source, rect3).copyTo(r3);
            cv::Mat(source, rect4).copyTo(r4);
            return region;
        } else if (rects.size() == 2) {
            auto rect1 = rects[0];
            auto rect2 = rects[1];

            if (rect1.x != rect2.x) {
                cv::Mat region(rect1.height, rect1.width + rect2.width, source.type());
                cv::Mat r1(region, cv::Rect(          0,            0, rect1.height, rect1.width));
                cv::Mat r2(region, cv::Rect(rect1.width,            0, rect2.height, rect2.width));

                cv::Mat(source, rect1).copyTo(r1);
                cv::Mat(source, rect2).copyTo(r2);
                return region;
            } else {
                cv::Mat region(rect1.height + rect2.height, rect1.width, source.type());
                cv::Mat r1(region, cv::Rect(          0,            0, rect1.height, rect1.width));
                cv::Mat r2(region, cv::Rect(          0, rect1.height, rect2.height, rect2.width));

                cv::Mat(source, rect1).copyTo(r1);
                cv::Mat(source, rect2).copyTo(r2);
                return region;
            }
        }
        return cv::Mat(source, rects[0]);
    }


public:
    /**
     * Empty constructor.
     */
    ObjectDetector() : exportEnable(false) {
    }

    /**
     * Empty destructor.
     */
    virtual ~ObjectDetector() {
    }


    /**
     * Enable detected object export.
     *
     * \param path target path for image files
     * \param suffix image file suffix (such as '.png')
     */
    virtual void setObjectExport(const std::string &path, const std::string &suffix) {
        this->exportEnable = true;
        this->exportPath = path;
        this->exportSuffix = suffix;
    }

    /**
     * Export detected objects.
     *
     * \param source source image to scan for objects
     * \param objects list of detected objects
     */
    virtual void exportObjects(const cv::Mat &source, const std::list<DetectedObject> &objects) {
        if (this->exportEnable) {
            // export objects
            std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
                char filename[1024];
                auto doubleToBits = [] (double value) {
                    const unsigned char *ptr = (const unsigned char *)&value;
                    unsigned char v1 = ptr[0] ^ ptr[4];
                    unsigned char v2 = ptr[1] ^ ptr[5];
                    unsigned char v3 = ptr[2] ^ ptr[6];
                    unsigned char v4 = ptr[3] ^ ptr[7];

                    return (v1 << 24 | v2 << 16 | v3 << 8 | v4);
                };
                struct timespec ts;
                unsigned int hash1 = 11;
                unsigned int hash2 = 11;

                memset(&ts, 0, sizeof(struct timespec));
                clock_gettime(CLOCK_MONOTONIC, &ts);

                hash1 = hash1 * 31 + doubleToBits(object.area.p1.x);
                hash1 = hash1 * 31 + doubleToBits(object.area.p1.y);
                hash2 = hash2 * 31 + doubleToBits(object.area.p2.x);
                hash2 = hash2 * 31 + doubleToBits(object.area.p2.y);

                snprintf(filename, sizeof(filename), "%s_%08X_%08X_%lu.%lu", object.className.c_str(), hash1, hash2, ts.tv_sec, ts.tv_nsec / 1000l);
                cv::imwrite(this->exportPath + "/" + filename + this->exportSuffix, this->getObjectRegion(source, object));
            });
        }
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

        auto objectsNode = fs["objects"];

        for (auto it = objectsNode.begin(); it != objectsNode.end(); ++it) {
            objects.push_back(*it);
        }
        return true;
    }

    /**
     * Merge overlapping detected objects.
     *
     * \param objects detected objects (input/output)
     * \param minOverlap minimum overlap to keep objects
     */
    static void merge(std::list<DetectedObject> &objects, int minOverlap = 1) {
        std::vector<DetectedObject> v(objects.begin(), objects.end());
        std::set<unsigned int> used;

        objects.clear();
        for (unsigned int i = 0; i < v.size(); i++) {
            if (used.find(i) != used.end()) {
                continue;
            }
            used.insert(i);

            BoundingBox area(v[i].area);
            std::set<std::string> classNames;
            std::list<DetectedObject> children(v[i].children);
            int count = 1;

            classNames.insert(v[i].className);
            for (unsigned int j = i + 1; j < v.size(); j++) {
                if (used.find(j) != used.end()) {
                    continue;
                }
                if (area.mergeIfOverlap(v[j].area)) {
                    classNames.insert(v[j].className);
                    children.insert(children.end(), v[j].children.begin(), v[j].children.end());
                    used.insert(j);
                    count++;
                    j = i;
                }
            }

            std::function<void(const DetectedObject &)> childCounter = [&] (const DetectedObject &child) {
                count++;
                std::for_each(child.children.begin(), child.children.end(), childCounter);
            };
            std::for_each(children.begin(), children.end(), childCounter);

            if (count >= minOverlap) {
                std::string className;

                for (auto it = classNames.begin(); it != classNames.end(); ++it) {
                    if (!className.empty()) {
                        className.append(":");
                    }
                    className.append(*it);
                }

                ObjectDetector::merge(children);

                objects.push_back(DetectedObject(className, area, children));
            }
        }
    }
};


#endif //__YAFDB_DETECTORS_DETECTOR_H_INCLUDE__