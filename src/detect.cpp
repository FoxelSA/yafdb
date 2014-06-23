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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "detectors/haar.hpp"


/*
 * Program arguments.
 *
 */

#define OPTION_ALGORITHM    0


static struct option options[] = {
    {"algorithm", required_argument, 0, 'a'},
    {0, 0, 0, 0}
};


/*
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE      0
#define ALGORITHM_HAAR      1


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-detect --algorithm algo input-panorama.tiff output-objects.txt\n\n");

    printf("Detects objects within input panorama (eqr). Detected objects are\n");
    printf("written to a text file.\n\n");

    printf("--algorithm algo: algorithm to use for object detection ('haar')\n");
    printf("--reprojection-window-size=640x480: window size for reprojection task\n");
    printf("--reprojection-dx=45: horizontal angle increment for reprojection task\n");
    printf("--reprojection-dy=45: vertical angle increment for reprojection task\n");
}


/**
 * Program entry-point.
 *
 */
int main(int argc, char **argv) {
    // parse arguments
    int algorithm = ALGORITHM_HAAR;
    const char *source_file = NULL;
    const char *objects_file = NULL;

    while (true) {
        int index = -1;
        int code = getopt_long(argc, argv, "", options, &index);

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
            if (access(objects_file, W_OK) && errno == EACCES) {
                fprintf(stderr, "Error: detected objects file not writable: %s\n", objects_file);
                return 2;
            }
            break;
        }

        switch (index) {
        case OPTION_ALGORITHM:
            if (strcmp(optarg, "none") == 0) {
                algorithm = ALGORITHM_NONE;
            } else if (strcmp(optarg, "haar") == 0) {
                algorithm = ALGORITHM_HAAR;
            } else {
                fprintf(stderr, "Error: unsupported algorithm: %s\n", optarg);
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

    // instantiate detector
    ObjectDetector *detector;

    switch (algorithm) {
    case ALGORITHM_NONE:
        detector = new ObjectDetector();
        break;
    case ALGORITHM_HAAR:
        detector = new HaarDetector();
        break;
    default:
        fprintf(stderr, "Error: no detector instantiated!\n");
        return 3;
    }

    // detect objects in source image
    bool success = false;

    if (detector != NULL) {
        // display source file
        detector->preview(source);

        // run detection algorithm
        std::vector<DetectedObject> objects;

        if (detector->detect(source, objects)) {
            success = true;

            // TODO: save detected objects

            // display detected objects
            detector->preview(source, objects);
        }

        delete detector;
        detector = NULL;
    }
    return success ? 0 : 4;
}
