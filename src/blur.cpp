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

#include <string>
#include <list>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>


/*
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE      0
#define ALGORITHM_DEFAULT   1


/*
 * Program arguments.
 *
 */

#define OPTION_ALGORITHM              0


static int algorithm = ALGORITHM_DEFAULT;
static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *target_file = NULL;


static struct option options[] = {
    {"algorithm",           required_argument, 0,                 'a'},
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-blur --algorithm algo input-image.tiff input-objects.txt output-image.tiff\n\n");

    printf("Blurs detected objects and write modified image as output.\n\n");

    printf("General options:\n\n");
    printf("--algorithm algo: algorithm to use for blurring ('default')\n");
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
            } else if (strcmp(optarg, "default") == 0) {
                algorithm = ALGORITHM_DEFAULT;
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

    // TODO: read detected objects

    // TODO: apply blur operation
    switch (algorithm) {
    case ALGORITHM_NONE:
        break;

    case ALGORITHM_DEFAULT:
        break;

    default:
        fprintf(stderr, "Error: no blur operation performed!\n");
        return 3;
    }
    return 0;
}