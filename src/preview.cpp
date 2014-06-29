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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "detectors/detector.hpp"

/*
 * Program arguments.
 *
 */

static const char *source_file = NULL;
static const char *objects_file = NULL;


static struct option options[] = {
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-preview input-image.tiff input-objects.yml\n\n");

    printf("Preview detected objects in source image.\n\n");
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
            if (argc != optind + 2) {
                usage();
                return 1;
            }
            source_file = argv[optind++];
            if (access(source_file, R_OK)) {
                fprintf(stderr, "Error: source file not readable: %s\n", source_file);
                return 2;
            }

            objects_file = argv[optind++];
            if (access(objects_file, R_OK)) {
                fprintf(stderr, "Error: detected objects file not readable: %s\n", objects_file);
                return 2;
            }
            break;
        }

        usage();
        return 1;
    }

    // read source file
    cv::Mat source = cv::imread(source_file);

    if (source.rows <= 0 || source.cols <= 0) {
        fprintf(stderr, "Error: cannot read image in source file: %s\n", source_file);
        return 2;
    }

    // read detected objects
    std::list<DetectedObject> objects;

    if (!ObjectDetector::load(objects_file, objects)) {
        fprintf(stderr, "Error: cannot read objects in file: %s\n", objects_file);
        return 2;
    }

    // merge detected objects
    ObjectDetector::merge(objects);

    // draw detected objects
    const int borderSize = 20;
    cv::Scalar colors[] = {
        cv::Scalar(0, 255, 255),
        cv::Scalar(255, 0, 255),
        cv::Scalar(0, 255, 255),
        cv::Scalar(0, 0, 255),
        cv::Scalar(0, 255, 0),
        cv::Scalar(255, 0, 0)
    };
    std::function<void(const DetectedObject&, int)> drawObjectWithDepth = [&] (const DetectedObject &object, int depth) {
        std::vector<cv::Rect> rects = object.area.rects(source.cols, source.rows);

        for (auto it = rects.begin(); it != rects.end(); ++it) {
            putText(source, object.className, (*it).tl(), CV_FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(255, 255, 255), 3);
            rectangle(source, *it, colors[depth], borderSize);
        }
        for (auto it = object.children.begin(); it != object.children.end(); ++it) {
            drawObjectWithDepth(*it, MAX(depth + 1, 5));
        }
    };
    auto drawObject = [&] (const DetectedObject &object) {
        drawObjectWithDepth(object, 0);
    };

    std::for_each(objects.begin(), objects.end(), drawObject);

    cv::namedWindow("preview", cv::WINDOW_NORMAL);
    cv::imshow("preview", source);
    while (cv::waitKey(0) != '\n');
    return 0;
}
