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
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>


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
    BoundingBox(const cv::FileNode &node);


    /**
     * Write area to storage.
     *
     * \param fs storage to write to
     */
    void write(cv::FileStorage &fs) const;

    /**
     * Check if coordinates are in cartesian system.
     *
     * \param return true if coordinates are cartesian
     */
    bool isCartesian() const {
        return this->system == CARTESIAN;
    }

    /**
     * Check if coordinates are in spherical system.
     *
     * \param return true if coordinates are spherical
     */
    bool isSpherical() const {
        return this->system == SPHERICAL;
    }

    /**
     * Get box width.
     *
     * \return box width
     */
    double width() const;

    /**
     * Get box height.
     *
     * \return box height
     */
    double height() const;

    /**
     * Move coordinates.
     *
     * \param x x-axis translation
     * \param y y-axis translation
     */
    void move(double x, double y);

    /**
     * Check if other bounding box overlap, and if yes, merge other area
     * into this one.
     *
     * \param other other bounding box
     * \return true if overlap detected, false otherwise
     */
    bool mergeIfOverlap(const BoundingBox &other);

    /**
     * Convert bounding box to opencv rectangle(s). If the coordinate system is
     * spherical, multiple rectangles might be returned to cover the area.
     *
     * \param width target image width
     * \param height target image height
     * \return opencv rectangle(s)
     */
    std::vector<cv::Rect> rects(int width, int height) const;
};


class GnomonicTransform {
protected:
    /** Width in pixels of gnomonic projection */
    int gnomonic_width;

    /** Height in pixels of gnomonic projection */
    int gnomonic_height;

    /** Gnomonic horizontal aperture angle (in radian) */
    double gnomonic_ax;

    /** Gnomonic vertical aperture angle (in radian) */
    double gnomonic_ay;

    /** Gnomonic center azimuthal angle (in radian) */
    double gnomonic_phi;

    /** Gnomonic center polar angle (in radian) */
    double gnomonic_theta;

    /** Half-aperture angle */
    double gnomonic_thax;

    /** Half-aperture angle */
    double gnomonic_thay;

    /** Rotation from eqr to gnomonic */
    cv::Mat gnomonicRotation;

    /** Rotation from gnomonic to eqr */
    cv::Mat eqrRotation;


public:
    /**
     * Empty constructor.
     *
     */
    GnomonicTransform() : gnomonic_width(0), gnomonic_height(0), gnomonic_ax(0), gnomonic_ay(0), gnomonic_phi(0), gnomonic_theta(0), gnomonic_thax(0), gnomonic_thay(0) {
    }

    /**
     * Default constructor.
     *
     * \param gnomonic_width width in pixels of gnomonic projection
     * \param gnomonic_height height in pixels of gnomonic projection
     * \param gnomonic_ax gnomonic horizontal aperture angle (in radian)
     * \param gnomonic_ay gnomonic vertical aperture angle (in radian)
     * \param gnomonic_phi gnomonic center azimuthal angle (in radian)
     * \param gnomonic_theta gnomonic center polar angle (in radian)
     */
    GnomonicTransform(int gnomonic_width, int gnomonic_height, double gnomonic_ax, double gnomonic_ay, double gnomonic_phi, double gnomonic_theta) : gnomonic_width(0), gnomonic_height(0), gnomonic_ax(0), gnomonic_ay(0), gnomonic_phi(0), gnomonic_theta(0), gnomonic_thax(0), gnomonic_thay(0) {
        this->setup(gnomonic_width, gnomonic_height, gnomonic_ax, gnomonic_ay, gnomonic_phi, gnomonic_theta);
    }

    /**
     * Copy constructor.
     *
     * \param ref other transform
     */
    GnomonicTransform(const GnomonicTransform &ref) : gnomonic_width(ref.gnomonic_width), gnomonic_height(ref.gnomonic_height), gnomonic_ax(ref.gnomonic_ax), gnomonic_ay(ref.gnomonic_ay), gnomonic_phi(ref.gnomonic_phi), gnomonic_theta(ref.gnomonic_theta), gnomonic_thax(ref.gnomonic_thax), gnomonic_thay(ref.gnomonic_thay), gnomonicRotation(ref.gnomonicRotation), eqrRotation(ref.eqrRotation) {
    }


    /**
     * Setup gnomonic transformation.
     *
     * \param gnomonic_width width in pixels of gnomonic projection
     * \param gnomonic_height height in pixels of gnomonic projection
     * \param gnomonic_ax gnomonic horizontal aperture angle (in radian)
     * \param gnomonic_ay gnomonic vertical aperture angle (in radian)
     * \param gnomonic_phi gnomonic center azimuthal angle (in radian)
     * \param gnomonic_theta gnomonic center polar angle (in radian)
     */
    void setup(int gnomonic_width, int gnomonic_height, double gnomonic_ax, double gnomonic_ay, double gnomonic_phi, double gnomonic_theta);


    /**
     * Project a point from eqr to gnomonic.
     *
     * \param eqr_phi azimuthal angle (in radian)
     * \param eqr_theta polar angle (in radian)
     * \param gnomonic_x output x coordinate in gnomonic projection
     * \param gnomonic_y output y coordinate in gnomonic projection
     * \return true if projection is conform, false otherwise
     */
    bool toGnomonic(double eqr_phi, double eqr_theta, int &gnomonic_x, int &gnomonic_y) const;

    /**
     * Compute whole gnomonic projection.
     *
     * \param src eqr source
     * \param dst gnomonic target (must be of correct size)
     */
    void toGnomonic(const cv::Mat &src, cv::Mat &dst) const;


    /**
     * Project a point from gnomonic to eqr.
     *
     * \param gnomonic_x x coordinate in gnomonic projection
     * \param gnomonic_y y coordinate in gnomonic projection
     * \param eqr_phi output azimuthal angle (in radian)
     * \param eqr_theta output polar angle (in radian)
     * \return true if projection is conform, false otherwise
     */
    bool toEqr(int gnomonic_x, int gnomonic_y, double &eqr_phi, double &eqr_theta) const;

    /**
     * Project a point from gnomonic to eqr.
     *
     * \param src gnomonic area
     * \param dst eqr area
     * \return true if projection is conform, false otherwise
     */
    bool toEqr(const BoundingBox &src, BoundingBox &dst) const;
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

    /** Is false positive ? */
    std::string falsePositive;

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
     * \param falsePositive Is false positive ?
     */
    DetectedObject(const std::string &className, const BoundingBox &area, const std::string &falsePositive) : className(className), area(area), falsePositive(falsePositive) {
    }

    /**
     * Default constructor.
     *
     * \param className object class name
     * \param area bounding area
     * \param falsePositive Is false positive ?
     * \param children children objects
     */
    DetectedObject(const std::string &className, const BoundingBox &area, const std::string &falsePositive, const std::list<DetectedObject> &children) : className(className), area(area), falsePositive(falsePositive), children(children) {
    }

    /**
     * Copy constructor.
     */
    DetectedObject(const DetectedObject &ref) : className(ref.className), area(ref.area), falsePositive(ref.falsePositive), children(ref.children) {
    }

    /**
     * Storage constructor.
     *
     * \param node storage node to read from
     */
    DetectedObject(const cv::FileNode &node);


    /**
     * Write detected object to storage.
     *
     * \param fs storage to write to
     */
    void write(cv::FileStorage &fs) const;

    /**
     * Add a child detection.
     *
     * \param child child object
     */
    void addChild(const DetectedObject &child);

    /**
     * Add children detection.
     *
     * \param children child objects
     */
    void addChildren(const std::list<DetectedObject> &children);

    /**
     * Move object coordinates.
     *
     * \param x x-axis translation
     * \param y y-axis translation
     */
    void move(double x, double y);

    /**
     * Get detected object region.
     *
     * \param source source image
     * \param offset output image offset in source image
     * \param rect output rectangle of object in returned image
     * \param borderSize extra border size
     * \return detected object region
     */
    cv::Mat getRegion(const cv::Mat &source, cv::Point &offset, cv::Rect &rect, int borderSize = 0) const;

    /**
     * Get detected object region in gnomonic projection.
     *
     * \param source source image (in eqr projection)
     * \param transform output gnomonic transform
     * \param rect output rectangle of object in returned image
     * \param gnomonicWidth width of gnomonic image
     * \param extraAperture extra aperture angle (in radian)
     * \return detected object region (in gnomonic projection)
     */
    cv::Mat getGnomonicRegion(const cv::Mat &source, GnomonicTransform &transform, cv::Rect &rect, int gnomonicWidth = 1024, double extraAperture = 0.0) const;
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
    virtual bool supportsColor() const;

    /*
     * Execute object detector against given image.
     *
     * \param source source image to scan for objects
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    virtual bool detect(const cv::Mat &source, std::list<DetectedObject> &objects);


    /**
     * Load detected objects from yaml file.
     *
     * \param file yaml filename
     * \param objects output list of detected objects
     * \return true on success, false otherwise
     */
    static bool load(const std::string &file, std::list<DetectedObject> &objects);

    /**
     * Merge overlapping detected objects.
     *
     * \param objects detected objects (input/output)
     * \param minOverlap minimum overlap to keep objects
     */
    static void merge(std::list<DetectedObject> &objects, int minOverlap = 1);

    /**
     * Export detected objects.
     *
     * \param exportPath target path for image files and yaml file
     * \param imageSuffix image file suffix (such as '.png')
     * \param source source image to scan for objects
     * \param objects list of detected objects
     */
    static void exportImages(const std::string &exportPath, const std::string &imageSuffix, const cv::Mat &source, const std::list<DetectedObject> &objects);
};


#endif //__YAFDB_DETECTORS_DETECTOR_H_INCLUDE__