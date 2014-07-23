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
#include <sys/stat.h>

#include "detectors/detector.hpp"


/*
 * List of supported image formats.
 *
 */

#define FORMAT_NONE       0
#define FORMAT_PNG        1
#define FORMAT_JPEG       2
#define FORMAT_TIFF       3


/*
 * Program arguments.
 *
 */

#define OPTION_MERGE_DISABLE          0
#define OPTION_MERGE_MIN_OVERLAP      1
#define OPTION_FORMAT                 2


static int merge_enabled = 1;
static int merge_min_overlap = 1;
static int format = FORMAT_PNG;
static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *output_path = NULL;


static struct option options[] = {
    {"merge-disable",        no_argument,       &merge_enabled,     0 },
    {"merge-min-overlap",    required_argument, 0,                  0 },
    {"format",               required_argument, 0,                  0 },
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-export input-image.tiff input-objects.yaml output-path/\n\n");

    printf("Export detected objects from source image into output folder (yaml+images).\n\n");

    printf("General options:\n\n");
    printf("--merge-disable: don't merge overlapping rectangles\n");
    printf("--merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects\n");
    printf("--format png : set exported object image format ('png', 'jpeg', 'tiff')\n");
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

            output_path = argv[optind++];
            break;
        }

        switch (index) {
        case OPTION_MERGE_DISABLE:
            break;

        case OPTION_MERGE_MIN_OVERLAP:
            merge_min_overlap = atoi(optarg);
            break;

        case OPTION_FORMAT:
            if (strcmp(optarg, "none") == 0) {
                format = FORMAT_NONE;
            } else if (strcmp(optarg, "png") == 0) {
                format = FORMAT_PNG;
            } else if (strcmp(optarg, "jpeg") == 0) {
                format = FORMAT_JPEG;
            } else if (strcmp(optarg, "tiff") == 0) {
                format = FORMAT_TIFF;
            } else {
                fprintf(stderr, "Error: unsupported export format: %s\n", optarg);
            }
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
    if (merge_enabled) {
        ObjectDetector::merge(objects, merge_min_overlap);
    }

    // export detected objects
    switch (format) {
    case FORMAT_PNG:
        ObjectDetector::exportImages(output_path, ".png", source, objects);
        break;
    case FORMAT_JPEG:
        ObjectDetector::exportImages(output_path, ".jpeg", source, objects);
        break;
    case FORMAT_TIFF:
        ObjectDetector::exportImages(output_path, ".tiff", source, objects);
        break;
    default:
        fprintf(stderr, "Error: unsupported export image format!\n");
        return 3;
    }
    return 0;
}
