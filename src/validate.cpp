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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "detectors/detector.hpp"
#include "detectors/gnomonic.hpp"


/*
 * Program arguments.
 *
 */

#define OPTION_FULLSCREEN             0
#define OPTION_SHOW_INVALID_OBJECTS   1
#define OPTION_MERGE_DISABLE          2
#define OPTION_MERGE_MIN_OVERLAP      3
#define OPTION_AUTO_VALIDATE          4
#define OPTION_GNOMONIC               5


int type_slider = 0;
static int manual_mode = 0;
static int fullscreen = 0;
static int show_invalid_objects = 0;
static int merge_enabled = 1;
static int merge_min_overlap = 1;
static int auto_validate = 0;
static int gnomonic_enabled = 0;
static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *target_file = NULL;


static struct option options[] = {
    {"fullscreen",           no_argument,       &fullscreen,           1 },
    {"show-invalid-objects", no_argument,       &show_invalid_objects, 1 },
    {"merge-disable",        no_argument,       &merge_enabled,        0 },
    {"merge-min-overlap",    required_argument, 0,                     0 },
    {"auto-validate",        no_argument,       &auto_validate,        1 },
    {"gnomonic",             no_argument,       &gnomonic_enabled,     1 },
    {0, 0, 0, 0}
};


/*
 * Edit modes.
 *
 */

#define MODE_NONE     0
#define MODE_SCENE    1
#define MODE_OBJECT   2


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-validate input-image.tiff [input-objects.yaml] output-objects.yaml\n\n");

    printf("Validate detected objects in source image.\n\n");

    printf("General options:\n\n");
    printf("--fullscreen : Start validation window in fullscreen\n");
    printf("--show-invalid-objects : Display invalid objects\n");
    printf("--merge-disable: don't merge overlapping rectangles\n");
    printf("--merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects\n");
    printf("--auto-validate : enable auto-validation instead of manual validation\n");
    printf("\n");

    printf("Gnomonic projection options:\n\n");
    printf("--gnomonic : activate gnomonic reprojection for visualization\n");
    printf("\n");
 }

/**
 * Display object type configuration window
 *
 */
void configureType(DetectedObject &object)
{
    char trackBarName[] = "0 = Face, 1 = Sign";

    cv::namedWindow("Type config", cv::WINDOW_NORMAL);
    cv::createTrackbar( trackBarName, "Type config", &type_slider, 1, NULL );

    if(object.className == "face")
    {
        cv::setTrackbarPos(trackBarName, "Type config", 0);
        type_slider = 0;
    } 
    else if(object.className == "sign")
    {
        cv::setTrackbarPos(trackBarName, "Type config", 1);
        type_slider = 1;
    }

    int config_cond = 1;
    while (config_cond) {
        int key2 = cv::waitKey(0);

        switch (key2 & 0xff) {
            case 0x1b:
                config_cond = 0;
            case 'y':
            case 'Y':
            case '\n':
            {
                switch(type_slider)
                {
                    case 0:
                    {
                        object.className = "face";
                        break;
                    }

                    case 1:
                    {
                        object.className = "sign";
                        break;
                    }
                }

                config_cond = 0;
            }
        }
    }

    cv::destroyWindow("Type config");
}

/**
 * Program entry-point.
 *
 */
int main(int argc, char **argv) {
    // parse arguments
    while (true) {
        int index = -1;

        getopt_long(argc, argv, "", options, &index);
        if (index == -1) {

            if ((argc - optind) < 2) {
                usage();
                return 1;
            }

            if((argc - optind) == 2)
                manual_mode = 1;

            source_file = argv[optind++];
            if (access(source_file, R_OK)) {
                fprintf(stderr, "Error: source file not readable: %s\n", source_file);
                return 2;
            }

            if(!manual_mode)
            {
                objects_file = argv[optind++];
                if (access(objects_file, R_OK)) {
                    fprintf(stderr, "Error: detected objects file not readable: %s\n", objects_file);
                    return 2;
                }
            }

            target_file = argv[optind++];
            if (access(target_file, W_OK) && errno == EACCES) {
                fprintf(stderr, "Error: target file not writable: %s\n", target_file);
                return 2;
            }
            break;
        }

        switch (index) {
        case OPTION_FULLSCREEN:
            break;

        case OPTION_SHOW_INVALID_OBJECTS:
            break;

        case OPTION_MERGE_DISABLE:
            break;

        case OPTION_MERGE_MIN_OVERLAP:
            merge_min_overlap = atoi(optarg);
            break;

        case OPTION_AUTO_VALIDATE:
            break;

        case OPTION_GNOMONIC:
            break;

        default:
            usage();
            return 1;
        }
    }

    // read source file
    cv::Mat source = cv::imread(source_file);

    if (source.rows <= 0 || source.cols <= 0) {
        fprintf(stderr, "Error: cannot read image in source file: %s\n", source_file);
        return 2;
    }

    // read detected objects
    std::list<DetectedObject> objects;

    if(!manual_mode)
    {
        if (!ObjectDetector::load(objects_file, objects)) {
            fprintf(stderr, "Error: cannot read objects in file: %s\n", objects_file);
            return 2;
        }

        // merge detected objects
        if (merge_enabled) {
            ObjectDetector::merge(objects, merge_min_overlap);
        }
    }

    // object editors
    int eqrWidth = 1600;
    int eqrHeight = eqrWidth * source.rows / source.cols;
    std::list<DetectedObject> validObjects;
    std::list<DetectedObject> invalidObjects;
    std::list<DetectedObject> userObjects;
    auto mouseCallback = [] (int event, int x, int y, int button, void *userdata) {
       (*reinterpret_cast<std::function<void(int, int, int, int)>*>(userdata))(event, x, y, button);
    };
    auto editObject = [&] (DetectedObject &object, bool insert) {
        // reproject or crop area
        cv::Mat validationRegion;
        cv::Point offset;
        GnomonicTransform transform;
        cv::Rect rect;

        if (gnomonic_enabled) {
            validationRegion = object.getGnomonicRegion(source, transform, rect, 1024, 4.0 * M_PI / 180.0);
        } else {
            validationRegion = object.getRegion(source, offset, rect, 200);
        }

        // validate object with user
        auto draw = [&] () {
            cv::Mat preview(validationRegion.clone());

            putText(preview, object.className, cv::Point2i(5, 15), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255), 2, 8, false);
            rectangle(preview, rect, cv::Scalar(0, 255, 255), 2);

            cv::imshow("preview", preview);
        };
        std::function<void(int, int, int, int)> mouseHandler = [&] (int event, int x, int y, int button) {
            switch (event) {
            case cv::EVENT_LBUTTONDOWN:
                rect.x = x;
                rect.y = y;
                draw();
                break;
            case cv::EVENT_RBUTTONDOWN:
                rect.width = MAX(x - rect.x, 1);
                rect.height = MAX(y - rect.y, 1);
                draw();
                break;
            }
        };
        cv::setMouseCallback("preview", mouseCallback, &mouseHandler);
        draw();

        while (true) {
            int key = cv::waitKey(0);

            switch (key & 0xff) {
            case 't':
            {
                configureType(object);
                break;
            }
            case 'y':
            case 'Y':
            case '\n':
                // remap coordinates
                if (gnomonic_enabled) {
                    transform.toEqr(rect.x, rect.y, object.area.p1.x, object.area.p1.y);
                    transform.toEqr(rect.x + rect.width, rect.y + rect.height, object.area.p2.x, object.area.p2.y);
                } else {
                    object.area.p1.x = offset.x + rect.x;
                    object.area.p1.y = offset.y + rect.y;
                    object.area.p2.x = offset.x + rect.x + rect.width;
                    object.area.p2.y = offset.y + rect.y + rect.height;
                }
                object.falsePositive = "No";
                
                if (insert) {
                    configureType(object);
                    userObjects.push_back(object);
                } else {
                    validObjects.push_back(object);
                }
                return;

            case 'n':
            case 'N':
            case 0x1b:
                if (!insert) {
                    // remap coordinates
                    if (gnomonic_enabled) {
                        transform.toEqr(rect.x, rect.y, object.area.p1.x, object.area.p1.y);
                        transform.toEqr(rect.x + rect.width, rect.y + rect.height, object.area.p2.x, object.area.p2.y);
                    } else {
                        object.area.p1.x = offset.x + rect.x;
                        object.area.p1.y = offset.y + rect.y;
                        object.area.p2.x = offset.x + rect.x + rect.width;
                        object.area.p2.y = offset.y + rect.y + rect.height;
                    }
                    object.falsePositive = "Yes";
                    invalidObjects.push_back(object);
                }
                return;
            }
        }
    };
    auto pixelsToSpherical = [&] (const cv::Point &p) {
        return cv::Point2d(
            (double)p.x / (double)eqrWidth * 2 * M_PI,
            (2.0 * (double)p.y / (double)eqrHeight - 1.0) * M_PI / 2
        );
    };
    auto insertObject = [&] (const cv::Point &p1, const cv::Point &p2) {
        DetectedObject object;

        if (gnomonic_enabled) {
            object = DetectedObject(
                "manual",
                BoundingBox(BoundingBox::SPHERICAL, pixelsToSpherical(p1), pixelsToSpherical(p2)),
                "No"
            );
        } else {
            object = DetectedObject(
                "manual",
                BoundingBox(cv::Rect(p1.x, p1.y, p1.x + p2.x, p1.y + p2.y)),
                "No"
            );
        }
        editObject(object, true);
    };
    auto preview = [&] () {
        std::list<DetectedObject> editObjects;
        std::list<DetectedObject> invalidateObjects;
        cv::Point2i coordinates[2];
        bool leftClick = false;
        bool done = false;
        bool edit = false;
        auto draw = [&] () {
            cv::Mat preview(eqrWidth, eqrHeight, source.type());

            cv::resize(source, preview, cv::Size(eqrWidth, eqrHeight));
            
            if(show_invalid_objects)
            {
                std::for_each(invalidObjects.begin(), invalidObjects.end(), [&] (const DetectedObject &object) {
                    std::vector<cv::Rect> rects = object.area.rects(preview.cols, preview.rows);

                    for (auto it = rects.begin(); it != rects.end(); ++it) {
                        rectangle(preview, *it, cv::Scalar(0, 0, 255), 2);
                    }
                });
            }
            std::for_each(validObjects.begin(), validObjects.end(), [&] (const DetectedObject &object) {
                std::vector<cv::Rect> rects = object.area.rects(preview.cols, preview.rows);

                for (auto it = rects.begin(); it != rects.end(); ++it) {
                    rectangle(preview, *it, cv::Scalar(0, 255, 0), 2);
                }
            });
            std::for_each(userObjects.begin(), userObjects.end(), [&] (const DetectedObject &object) {
                std::vector<cv::Rect> rects = object.area.rects(preview.cols, preview.rows);

                for (auto it = rects.begin(); it != rects.end(); ++it) {
                    rectangle(preview, *it, cv::Scalar(0, 255, 255), 2);
                }
            });
            cv::imshow("preview", preview);
        };
        std::function<void(int, int, int, int)> mouseHandler = [&] (int event, int x, int y, int button) {
            cv::Point2d p;
            auto selectObjects = [&] (std::list<DetectedObject> &list) {
                std::list<DetectedObject> list2(list.begin(), list.end());

                list.clear();
                for (auto it = list2.begin(); it != list2.end(); ++it) {
                    DetectedObject &object = *it;
                    bool xok = false;
                    bool yok = false;
                    double xeps = gnomonic_enabled ? 1.0 * M_PI / 180.0 : 10;
                    double yeps = gnomonic_enabled ? 0.5 * M_PI / 180.0 : 10;

                    if (object.area.p1.x <= object.area.p2.x) {
                        xok = p.x + xeps >= object.area.p1.x && p.x - xeps <= object.area.p2.x;
                    } else {
                        xok = p.x + xeps >= object.area.p1.x || p.x - xeps <= object.area.p2.x;
                    }
                    if (object.area.p1.x <= object.area.p2.x) {
                        yok = p.y + yeps >= object.area.p1.y && p.y - yeps <= object.area.p2.y;
                    } else {
                        yok = p.y + yeps >= object.area.p1.y || p.y - yeps <= object.area.p2.y;
                    }

                    if (xok && yok) {
                        editObjects.push_back(object);
                    } else {
                        list.push_back(object);
                    }
                }
            };

            switch (event) {
            case cv::EVENT_LBUTTONDOWN:
                coordinates[0].x = x;
                coordinates[0].y = y;

                // lookup existing objects
                editObjects.clear();
                if (gnomonic_enabled) {
                    p = pixelsToSpherical(cv::Point(x, y));
                } else {
                    p = cv::Point2d(x, y);
                }
                selectObjects(invalidObjects);
                selectObjects(validObjects);
                selectObjects(userObjects);
                if (editObjects.size() > 0) {
                    leftClick = false;
                    edit = true;
                } else {
                    leftClick = true;
                }
                break;

            case cv::EVENT_RBUTTONDOWN:
                coordinates[1].x = x;
                coordinates[1].y = y;

                // lookup existing objects
                editObjects.clear();
                if (gnomonic_enabled) {
                    p = pixelsToSpherical(cv::Point(x, y));
                } else {
                    p = cv::Point2d(x, y);
                }
                selectObjects(validObjects);
                selectObjects(userObjects);
                if (editObjects.size() > 0) {
                    leftClick = false;

                    invalidObjects.insert(invalidObjects.end(), editObjects.begin(), editObjects.end());
                    editObjects.clear();
                    draw();
                } else if (leftClick) {
                    leftClick = false;
                    edit = true;
                }
                break;
            }
        };
        cv::setMouseCallback("preview", mouseCallback, &mouseHandler);
        draw();

        while (!done) {
            if (edit) {
                if (editObjects.size() > 0) {
                    std::for_each(editObjects.begin(), editObjects.end(), [&] (DetectedObject &object) {
                        editObject(object, false);
                    });
                    editObjects.clear();
                } else {
                    cv::Point2i p1(
                        MIN(coordinates[0].x, coordinates[1].x),
                        MIN(coordinates[0].y, coordinates[1].y)
                    );
                    cv::Point2i p2(
                        MAX(MAX(coordinates[0].x, coordinates[1].x), p1.x + 1),
                        MAX(MAX(coordinates[0].y, coordinates[1].y), p1.y + 1)
                    );

                    insertObject(p1, p2);
                }
                cv::setMouseCallback("preview", mouseCallback, &mouseHandler);
                draw();
                edit = false;
            } else {
                int key = cv::waitKey(10);

                switch (key & 0xff) {
                case '\n':
                case 0x1b:
                    done = true;
                    break;
                }
            }
        }
    };

    // validate each detected object and review
    cv::namedWindow("preview", cv::WINDOW_NORMAL);

    if(fullscreen)
    {
        cv::setWindowProperty( "preview", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN );
    }

    if(!manual_mode)
    {
        if (auto_validate) {
            std::for_each(objects.begin(), objects.end(), [&] (DetectedObject &object) {
                if(object.falsePositive == "No")
                {
                    validObjects.push_back(object); 
                } else {
                    invalidObjects.push_back(object); 
                }
            });
        } else {
            std::for_each(objects.begin(), objects.end(), [&] (DetectedObject &object) {
                editObject(object, false);
            });
        }
    }
    preview();
    cv::destroyWindow("preview");

    // save validated objects
    cv::FileStorage fs(target_file, cv::FileStorage::WRITE);

    if(!manual_mode)
    {
        cv::FileStorage fsr(objects_file, cv::FileStorage::READ);

        fs << "algorithm" << (std::string)fsr["algorithm"];
        // fs << "haar" << fsr["haar"];
        if (fsr["gnomonic"].isMap()) {
            fs << "gnomonic" << "{";
            fs << "width" << (int)fsr["gnomonic"]["width"];
            fs << "aperture_x" << (int)fsr["gnomonic"]["aperture_x"];
            fs << "aperture_y" << (int)fsr["gnomonic"]["aperture_y"];
            fs << "}";
        }
    }

    fs << "source" << source_file;
    fs << "objects" << "[";
    std::for_each(validObjects.begin(), validObjects.end(), [&] (const DetectedObject &object) {
        object.write(fs);
    });
    std::for_each(userObjects.begin(), userObjects.end(), [&] (const DetectedObject &object) {
        object.write(fs);
    });
    fs << "]";
    fs << "invalidObjects" << "[";
    std::for_each(invalidObjects.begin(), invalidObjects.end(), [&] (const DetectedObject &object) {
        object.write(fs);
    });
    fs << "]";

    return 0;
}
