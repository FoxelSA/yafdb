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
 *      Kevin Velickovic <k.velickovic@foxel.ch>
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


#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/stat.h>

#include "detector.hpp"
#include "gnomonic.hpp"

#include <gnomonic-all.h>


#define CLAMP(x, a, b)  MIN(MAX(a, x), b)


BoundingBox::BoundingBox(const cv::FileNode &node) {
    int system;

    node["system"] >> system;
    node["p1"] >> this->p1;
    node["p2"] >> this->p2;
    this->system = (CoordinateSystem)system;
}

void BoundingBox::write(cv::FileStorage &fs) const {
    fs << "{";
        fs << "system" << (int)this->system;
        fs << "p1" << this->p1;
        fs << "p2" << this->p2;
    fs << "}";
}

double BoundingBox::width() const {
    switch (this->system) {
    case CARTESIAN:
        return (this->p2.x - this->p1.x);

    case SPHERICAL:
        if (this->p1.x > this->p2.x) {
            return (2 * M_PI - this->p1.x + this->p2.x);
        }
        return fabs(this->p2.x - this->p1.x);
    }
    return 0;
}

double BoundingBox::height() const {
    switch (this->system) {
    case CARTESIAN:
        return (this->p2.y - this->p1.y);

    case SPHERICAL:
        if (this->p1.y > this->p2.y) {
            return (M_PI / 2 - this->p1.x + fabs(this->p2.x));
        }
        return fabs(this->p2.y - this->p1.y);
    }
    return 0;
}

void BoundingBox::move(double x, double y) {
    this->p1.x += x;
    this->p1.y += y;
    this->p2.x += x;
    this->p2.y += y;
}

bool BoundingBox::mergeIfOverlap(const BoundingBox &other) {
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

std::vector<cv::Rect> BoundingBox::rects(int width, int height) const {
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


void GnomonicTransform::setup(int gnomonic_width, int gnomonic_height, double gnomonic_ax, double gnomonic_ay, double gnomonic_phi, double gnomonic_theta) {
    double rotateData1Z[3][3] = {
        { cos(-gnomonic_phi), -sin(-gnomonic_phi), 0.0 },
        { sin(-gnomonic_phi),  cos(-gnomonic_phi), 0.0 },
        {                0.0,                 0.0, 1.0 }
    };
    double rotateData1Y[3][3] = {
        {  cos(gnomonic_theta), 0.0, sin(gnomonic_theta) },
        {                   0.0, 1.0,                  0.0 },
        { -sin(gnomonic_theta), 0.0, cos(gnomonic_theta) }
    };
    double rotateData2Z[3][3] = {
        { cos(gnomonic_phi), -sin(gnomonic_phi), 0.0 },
        { sin(gnomonic_phi),  cos(gnomonic_phi), 0.0 },
        {               0.0,                0.0, 1.0 }
    };
    double rotateData2Y[3][3] = {
        {  cos(-gnomonic_theta), 0.0, sin(-gnomonic_theta) },
        {                  0.0, 1.0,                 0.0 },
        { -sin(-gnomonic_theta), 0.0, cos(-gnomonic_theta) }
    };

    this->gnomonic_width = gnomonic_width;
    this->gnomonic_height = gnomonic_height;
    this->gnomonic_ax = gnomonic_ax;
    this->gnomonic_ay = gnomonic_ay;
    this->gnomonic_phi = gnomonic_phi;
    this->gnomonic_theta = gnomonic_theta;
    this->gnomonic_thax = tan(gnomonic_ax / 2);
    this->gnomonic_thay = tan(gnomonic_ay / 2);
    this->gnomonicRotation = cv::Mat(3, 3, CV_64F, rotateData1Y) * cv::Mat(3, 3, CV_64F, rotateData1Z);
    this->eqrRotation = cv::Mat(3, 3, CV_64F, rotateData2Z) * cv::Mat(3, 3, CV_64F, rotateData2Y);
}

bool GnomonicTransform::toGnomonic(double eqr_phi, double eqr_theta, int &gnomonic_x, int &gnomonic_y) const {
    double positionData[3] = {
        cos(eqr_phi) * cos(eqr_theta),
        sin(eqr_phi) * cos(eqr_theta),
        sin(eqr_theta)
    };
    cv::Mat position(this->gnomonicRotation * cv::Mat(3, 1, CV_64F, positionData));
    double x = position.at<double>(0, 0);
    double y = position.at<double>(1, 0);
    double z = position.at<double>(2, 0);

    if (x <= 0) {
        return false;
    }
    gnomonic_x = (int)(((y / x / this->gnomonic_thax + 1.0) / 2.0) * (this->gnomonic_width - 1));
    gnomonic_y = (int)(((z / x / this->gnomonic_thay + 1.0) / 2.0) * (this->gnomonic_height - 1));
    return true;
}

void GnomonicTransform::toGnomonic(const cv::Mat &src, cv::Mat &dst) const {
    gnomonic_etg(
        src.data,
        src.cols,
        src.rows,
        src.channels(),
        dst.data,
        dst.cols,
        dst.rows,
        dst.channels(),
        this->gnomonic_phi,
        -this->gnomonic_theta,
        this->gnomonic_ax / 2.0,
        this->gnomonic_ay / 2.0,
        inter_bilinearf
    );
}

bool GnomonicTransform::toEqr(int gnomonic_x, int gnomonic_y, double &eqr_phi, double &eqr_theta) const {
    double ux = (2.0 * gnomonic_x / (this->gnomonic_width - 1.0) - 1.0) * this->gnomonic_thax;
    double uy = (2.0 * gnomonic_y / (this->gnomonic_height - 1.0) - 1.0) * this->gnomonic_thay;
    double p = cos(atan(sqrt(ux * ux + uy * uy)));
    double positionData[3] = { p, ux * p, uy * p };
    cv::Mat position(this->eqrRotation * cv::Mat(3, 1, CV_64F, positionData));
    double x = position.at<double>(0, 0);
    double y = position.at<double>(1, 0);
    double z = position.at<double>(2, 0);

    eqr_phi = acos(x / sqrt(x * x + y * y));
    if (y < 0) {
        eqr_phi = 2.0 * M_PI - eqr_phi;
    }
    eqr_theta = asin(z);
    return true;
}

bool GnomonicTransform::toEqr(const BoundingBox &src, BoundingBox &dst) const {
    dst.system = BoundingBox::SPHERICAL;
    if (this->toEqr((int)src.p1.x, (int)src.p1.y, dst.p1.x, dst.p1.y) &&
        this->toEqr((int)src.p2.x, (int)src.p2.y, dst.p2.x, dst.p2.y)) {

        // swap bounding box coordinates if necessary
        if (dst.p1.x > dst.p2.x && (2 * M_PI - dst.p1.x + dst.p2.x) > this->gnomonic_ax) {
            double tmp = dst.p1.x;

            dst.p1.x = dst.p2.x;
            dst.p2.x = tmp;
        }
        if (dst.p1.y > dst.p2.y && (M_PI - dst.p1.y + dst.p2.y) > this->gnomonic_ay) {
            double tmp = dst.p1.y;

            dst.p1.y = dst.p2.y;
            dst.p2.y = tmp;
        }
        return true;
    }
    return false;
}


DetectedObject::DetectedObject(const cv::FileNode &node) : className(node["className"]), area(node["area"]), falsePositive(node["falsePositive"]), autoStatus(node["autoStatus"]), manualStatus(node["manualStatus"]) {
    auto childrenNode = node["children"];

    for (auto it = childrenNode.begin(); it != childrenNode.end(); ++it) {
        this->children.push_back(*it);
    }
}

void DetectedObject::write(cv::FileStorage &fs) const {
    fs << "{";
        fs << "className" << this->className;
        fs << "area";
        this->area.write(fs);
        fs << "falsePositive" << (this->falsePositive.length() > 0 ? this->falsePositive : "No");
        fs << "autoStatus"    << (this->autoStatus.length() > 0 ? this->autoStatus : "None");
        fs << "manualStatus"  << (this->manualStatus.length() > 0 ? this->manualStatus : "None");
        if (!this->children.empty()) {
            fs << "children" << "[";
                for (auto it = this->children.begin(); it != this->children.end(); ++it) {
                    (*it).write(fs);
                }
            fs << "]";
        }
    fs << "}";
}

void DetectedObject::addChild(const DetectedObject &child) {
    this->children.push_back(child);
}

void DetectedObject::addChildren(const std::list<DetectedObject> &children) {
    this->children.insert(this->children.end(), children.begin(), children.end());
}

void DetectedObject::move(double x, double y) {
    this->area.move(x, y);
    for (auto it = this->children.begin(); it != this->children.end(); ++it) {
        (*it).move(x, y);
    }
}

cv::Mat DetectedObject::getRegion(const cv::Mat &source, cv::Point &offset, cv::Rect &rect, int borderSize) const {
    auto rects = this->area.rects(source.cols, source.rows);
    auto borderTop = [&] (const cv::Rect &rect) {
        return rect.y > borderSize ? borderSize : rect.y;
    };
    auto borderLeft = [&] (const cv::Rect &rect) {
        return rect.x > borderSize ? borderSize : rect.x;
    };
    auto borderBottom = [&] (const cv::Rect &rect) {
        return (source.rows - rect.y + rect.height) > borderSize ? borderSize : (source.rows - rect.y + rect.height);
    };
    auto borderRight = [&] (const cv::Rect &rect) {
        return (source.cols - rect.x + rect.width) > borderSize ? borderSize : (source.cols - rect.x + rect.width);
    };

    if (rects.size() == 4) {
        int bt = borderTop(rects[0]);
        int bl = borderLeft(rects[0]);
        int bb = borderBottom(rects[3]);
        int br = borderRight(rects[3]);
        auto rect1 = cv::Rect(rects[0].x - bl, rects[0].y - bt, rects[0].width + bl, rects[0].height + bt);
        auto rect2 = cv::Rect(rects[1].x,      rects[1].y - bt, rects[1].width + br, rects[1].height + bt);
        auto rect3 = cv::Rect(rects[2].x - bl, rects[2].y,      rects[0].width + bl, rects[0].height + bb);
        auto rect4 = cv::Rect(rects[3].x     , rects[3].y,      rects[0].width + br, rects[0].height + bb);
        cv::Mat region(rect1.height + rect3.height, rect1.width + rect2.width, source.type());
        cv::Mat r1(region, cv::Rect(          0,            0, rect1.width, rect1.height));
        cv::Mat r2(region, cv::Rect(rect1.width,            0, rect2.width, rect2.height));
        cv::Mat r3(region, cv::Rect(          0, rect1.height, rect3.width, rect3.height));
        cv::Mat r4(region, cv::Rect(rect1.width, rect1.height, rect4.width, rect4.height));

        cv::Mat(source, rect1).copyTo(r1);
        cv::Mat(source, rect2).copyTo(r2);
        cv::Mat(source, rect3).copyTo(r3);
        cv::Mat(source, rect4).copyTo(r4);
        offset.x = rect1.x;
        offset.y = rect1.y;
        rect.x = bl;
        rect.y = bt;
        rect.width = region.cols - bl - br;
        rect.height = region.rows - bt - bb;
        return region;
    } else if (rects.size() == 2) {
        if (rects[0].x != rects[1].x) {
            int bt = borderTop(rects[0]);
            int bl = borderLeft(rects[0]);
            int bb = borderBottom(rects[1]);
            int br = borderRight(rects[1]);
            auto rect1 = cv::Rect(rects[0].x - bl, rects[0].y - bt, rects[0].width + bl, rects[0].height + bt + bb);
            auto rect2 = cv::Rect(rects[1].x,      rects[1].y - bt, rects[1].width + br, rects[1].height + bt + bb);
            cv::Mat region(rect1.height, rect1.width + rect2.width, source.type());
            cv::Mat r1(region, cv::Rect(          0,            0, rect1.width, rect1.height));
            cv::Mat r2(region, cv::Rect(rect1.width,            0, rect2.width, rect2.height));

            cv::Mat(source, rect1).copyTo(r1);
            cv::Mat(source, rect2).copyTo(r2);
            offset.x = rect1.x;
            offset.y = rect1.y;
            rect.x = bl;
            rect.y = bt;
            rect.width = region.cols - bl - br;
            rect.height = region.rows - bt - bb;
            return region;
        } else {
            int bt = borderTop(rects[0]);
            int bl = borderLeft(rects[0]);
            int bb = borderBottom(rects[1]);
            int br = borderRight(rects[1]);
            auto rect1 = cv::Rect(rects[0].x - bl, rects[0].y - bt, rects[0].width + bl + br, rects[0].height + bt);
            auto rect2 = cv::Rect(rects[1].x - bl, rects[1].y,      rects[1].width + bl + br, rects[1].height + bb);
            cv::Mat region(rect1.height + rect2.height, rect1.width, source.type());
            cv::Mat r1(region, cv::Rect(          0,            0, rect1.width, rect1.height));
            cv::Mat r2(region, cv::Rect(          0, rect1.height, rect2.width, rect2.height));

            cv::Mat(source, rect1).copyTo(r1);
            cv::Mat(source, rect2).copyTo(r2);
            offset.x = rect1.x;
            offset.y = rect1.y;
            rect.x = bl;
            rect.y = bt;
            rect.width = region.cols - bl - br;
            rect.height = region.rows - bt - bb;
            return region;
        }
    }

    int bt = borderTop(rects[0]);
    int bl = borderLeft(rects[0]);
    int bb = borderBottom(rects[0]);
    int br = borderRight(rects[0]);

    offset.x = rects[0].x;
    offset.y = rects[0].y;
    rect.x = bl;
    rect.y = bt;
    rect.width = rects[0].width;
    rect.height = rects[0].height;
    return cv::Mat(source, cv::Rect(rects[0].x - bl, rects[0].y - bt, rects[0].width + bl + br, rects[0].height + bt + bb));
}

cv::Mat DetectedObject::getGnomonicRegion(const cv::Mat &source, GnomonicTransform &transform, cv::Rect &rect, int gnomonicWidth, double extraAperture) const {
    // check coordinates system
    if (this->area.system != BoundingBox::SPHERICAL || this->area.width() == 0) {
        return cv::Mat(0, 0, source.type());
    }

    // find object center / aperture
    double x = this->area.p1.x + this->area.width() / 2;
    double y = this->area.p1.y + this->area.height() / 2;
    double ax, ay;

    if (this->area.width() > this->area.height()) {
        ax = this->area.width() + extraAperture;
        ay = ax * this->area.height() / this->area.width();
        if (ay < ax / 2) {
            ay = ax / 2;
        }
    } else {
        ay = this->area.height() + extraAperture;
        ax = ay * this->area.width() / this->area.height();
        if (ax < ay / 2) {
            ax = ay / 2;
        }
    }

    // reproject in gnomonic
    cv::Mat window((int)((double)gnomonicWidth * ay / ax), gnomonicWidth, source.type());

    transform.setup(window.cols, window.rows, ax, ay, x, y);
    transform.toGnomonic(source, window);
    if (transform.toGnomonic(this->area.p1.x, this->area.p1.y, rect.x, rect.y) &&
        transform.toGnomonic(this->area.p2.x, this->area.p2.y, rect.width, rect.height)) {
        rect.width -= rect.x;
        rect.height -= rect.y;
    } else {
        rect.x = rect.y = rect.width = rect.height = 0;
    }
    return window;
}

bool ObjectDetector::supportsColor() const {
    return true;
}

bool ObjectDetector::detect(const cv::Mat &source, std::list<DetectedObject> &objects) {
    return false;
}

bool ObjectDetector::load(const std::string &file, std::list<DetectedObject> &objects) {
    cv::FileStorage fs(file, cv::FileStorage::READ);

    if (!fs.isOpened()) {
        return false;
    }

    auto objectsNode = fs["objects"];

    for (auto it = objectsNode.begin(); it != objectsNode.end(); ++it) {
        objects.push_back(*it);
    }

    auto objectsNode_inv = fs["invalidObjects"];
    for (auto it = objectsNode_inv.begin(); it != objectsNode_inv.end(); ++it) {
        objects.push_back(*it);
    }

    return true;
}

void ObjectDetector::merge(std::list<DetectedObject> &objects, int minOverlap) {
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
        std::set<std::string> falsePositives;
        std::set<std::string> autoStatuses;
        std::set<std::string> manualStatuses;
        std::list<DetectedObject> children(v[i].children);
        int count = 1;

        classNames.insert(v[i].className);
        falsePositives.insert(v[i].falsePositive);
        autoStatuses.insert(v[i].autoStatus);
        manualStatuses.insert(v[i].manualStatus);

        for (unsigned int j = i + 1; j < v.size(); j++) {
            if (used.find(j) != used.end()) {
                continue;
            }
            if (area.mergeIfOverlap(v[j].area)) {
                classNames.insert(v[j].className);
                falsePositives.insert(v[j].falsePositive);
                autoStatuses.insert(v[j].autoStatus);
                manualStatuses.insert(v[j].manualStatus);
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
            std::string falsePositiveName;
            std::string autoStatusName;
            std::string manualStatusName;

            for (auto it = classNames.begin(); it != classNames.end(); ++it) {
                if (!className.empty()) {
                    className.append(":");
                }
                className.append(*it);
            }

            for (auto it = falsePositives.begin(); it != falsePositives.end(); ++it) {
                if (!falsePositiveName.empty()) {
                    falsePositiveName.append(":");
                }
                falsePositiveName.append(*it);
            }

            for (auto it = autoStatuses.begin(); it != autoStatuses.end(); ++it) {
                if (!autoStatusName.empty()) {
                    autoStatusName.append(":");
                }
                autoStatusName.append(*it);
            }

            for (auto it = manualStatuses.begin(); it != manualStatuses.end(); ++it) {
                if (!manualStatusName.empty()) {
                    manualStatusName.append(":");
                }
                manualStatusName.append(*it);
            }

            ObjectDetector::merge(children);

            objects.push_back(DetectedObject(className, area, falsePositiveName, autoStatusName, manualStatusName, children));
        }
    }
}

void ObjectDetector::exportImages(const std::string &exportPath, const std::string &imageSuffix, const cv::Mat &source, const std::list<DetectedObject> &objects) {
    struct timespec ts;
    char timestamp[64];

    memset(&ts, 0, sizeof(struct timespec));
    clock_gettime(CLOCK_MONOTONIC, &ts);

    snprintf(timestamp, sizeof(timestamp), "%ld_%ld", ts.tv_sec, ts.tv_nsec / 1000l);

    // Create directories
    std::stringstream stream(exportPath);
    std::string path_mkd;

    for (std::string item; std::getline(stream, item, '/'); ) {
        if (path_mkd.length() > 0) {
            path_mkd.append("/");
        }
        path_mkd.append(item);
        mkdir(path_mkd.c_str(), 0755);
    }

    // export object writer
    cv::FileStorage fs(exportPath + "/" + timestamp + ".yaml", cv::FileStorage::WRITE);

    std::function<void(const DetectedObject &, const std::string &)> writeObject = [&] (const DetectedObject &object, const std::string &objectSuffix) {
        // TODO: clean invalid characters
        std::string className(object.className);

        std::for_each(className.begin(), className.end(), [] (char &c) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                return;
            }
            c = '-';
        });

        // export image
        std::string path(exportPath + "/" + object.className + "/" + timestamp + "_" + objectSuffix + "_" + className + imageSuffix);
        std::string path_false(exportPath +  "/" + object.className + "/" + "false_positives" + "/" + timestamp + "_" + objectSuffix + "_" + className + imageSuffix);

        // Create className directory
        mkdir((exportPath + "/" + object.className).c_str(), 0755);

        // Create false_positives directory
        if(object.falsePositive == "Yes")
        mkdir((exportPath + "/" + object.className + "/" + "false_positives").c_str(), 0755);

        // Configure the exported images quality level
        std::vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(100);
        compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(0);

        if (object.area.isCartesian()) {
            cv::Rect rect;
            cv::Point offset;
            cv::Mat region(object.getRegion(source, offset, rect));

            rect.x = MAX(rect.x, 0);
            rect.y = MAX(rect.y, 0);
            rect.width = MIN(rect.width, region.cols - rect.x);
            rect.height = MIN(rect.height, region.rows - rect.y);

            if(object.falsePositive == "Yes")
            {
                cv::imwrite(path_false, cv::Mat(region, rect), compression_params);
            } else {
                cv::imwrite(path, cv::Mat(region, rect), compression_params);
            }
        }

        if (object.area.isSpherical()) {
            GnomonicTransform transform;
            cv::Rect rect;
            cv::Mat region(object.getGnomonicRegion(source, transform, rect, 1024, 5.0 * M_PI / 180.0));

            rect.x = MAX(rect.x, 0);
            rect.y = MAX(rect.y, 0);
            rect.width = MIN(rect.width, region.cols - rect.x);
            rect.height = MIN(rect.height, region.rows - rect.y);
            if (rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0) {
                if(object.falsePositive == "Yes")
                {
                    cv::imwrite(path_false, cv::Mat(region, rect), compression_params);
                } else {
                    cv::imwrite(path, cv::Mat(region, rect), compression_params);
                }
            }
        }

        // write descriptor
        fs << "{";
            fs << "path" << (object.falsePositive == "Yes" ? path_false : path);
            fs << "className" << object.className;
            fs << "area";
            object.area.write(fs);
            fs << "falsePositive" << (object.falsePositive.length() > 0 ? object.falsePositive : "No");
            fs << "autoStatus"    << (object.autoStatus.length() > 0 ? object.autoStatus : "None");
            fs << "manualStatus"  << (object.manualStatus.length() > 0 ? object.manualStatus : "None");

            if (!object.children.empty()) {
                int childIndex = 0;

                fs << "children" << "[";
                std::for_each(object.children.begin(), object.children.end(), [&] (const DetectedObject &child) {
                    char suffix[32];

                    snprintf(suffix, sizeof(suffix), "%04d", childIndex++);
                    writeObject(child, objectSuffix + "_" + suffix);
                });
                fs << "]";
            }
        fs << "}";
    };

    // export objects
    int nextIndex = 0;

    fs << "objects";
    fs << "[";
    std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
        char suffix[32];

        snprintf(suffix, sizeof(suffix), "%04d", nextIndex++);
        writeObject(object, suffix);
    });
    fs << "]";
}
