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
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE       0
#define ALGORITHM_GAUSSIAN   1


/*
 * Program arguments.
 *
 */

#define OPTION_ALGORITHM              0
#define OPTION_MERGE_MIN_OVERLAP      1
#define OPTION_GAUSSIAN_KERNEL        2


static int algorithm = ALGORITHM_GAUSSIAN;
static int merge_min_overlap = 1;
static double gaussian_kernel_size = 65;
static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *target_file = NULL;


static struct option options[] = {
    {"algorithm",           required_argument, 0,                 'a'},
    {"merge-min-overlap",   required_argument, 0,                  0 },
    {"gaussian-kernel",     required_argument, 0,                  0 },
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-blur --algorithm algo input-image.tiff input-objects.yml output-image.tiff\n\n");

    printf("Blurs detected objects and write modified image as output.\n\n");

    printf("General options:\n\n");
    printf("--algorithm algo : algorithm to use for blurring ('gaussian')\n");
    printf("--merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects\n");
    printf("\n");

    printf("Gaussian options:\n\n");
    printf("--gaussian-kernel 65 : gaussian kernel size\n");
    printf("\n");
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
            if (argc != optind + 3) {
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

            target_file = argv[optind++];
            if (access(target_file, W_OK) && errno == EACCES) {
                fprintf(stderr, "Error: target file not writable: %s\n", target_file);
                return 2;
            }
            break;
        }

        switch (index) {
        case OPTION_ALGORITHM:
            if (strcmp(optarg, "none") == 0) {
                algorithm = ALGORITHM_NONE;
            } else if (strcmp(optarg, "gaussian") == 0) {
                algorithm = ALGORITHM_GAUSSIAN;
            } else {
                fprintf(stderr, "Error: unsupported algorithm: %s\n", optarg);
            }
            break;

        case OPTION_MERGE_MIN_OVERLAP:
            merge_min_overlap = atoi(optarg);
            break;

        case OPTION_GAUSSIAN_KERNEL:
            gaussian_kernel_size = atof(optarg);
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

    if (!ObjectDetector::load(objects_file, objects)) {
        fprintf(stderr, "Error: cannot read objects in file: %s\n", objects_file);
        return 2;
    }

    // merge detected objects
    ObjectDetector::merge(objects, merge_min_overlap);

    // apply blur operation
    std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
        auto rects = object.area.rects(source.cols, source.rows);

        std::for_each(rects.begin(), rects.end(), [&] (const cv::Rect &rect) {
            cv::Mat region(source, rect);

            switch (algorithm) {
            case ALGORITHM_NONE:
                break;

            case ALGORITHM_GAUSSIAN:
                GaussianBlur(
                    region,
                    region,
                    cv::Size(gaussian_kernel_size, gaussian_kernel_size),
                    0,
                    0
                );
                break;

            default:
                fprintf(stderr, "Error: unsupported blur algorithm!\n");
                break;
            }
        });
    });

    // save target file
    cv::imwrite(target_file, source);
    return 0;
}
