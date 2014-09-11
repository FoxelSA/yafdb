/*
 * edgeify - highlight edges in images.
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
#include <libgen.h>
#include <math.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>


/*
 * Program arguments.
 *
 */

// #define OPTION_                  0

static const char *input_file = NULL;
static const char *output_file = NULL;


static struct option options[] = {
    // {"width",                required_argument, 0,                  0 },
    // {"show",                 no_argument,       &show_enabled,      1 },
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-edgeify input.jpg output.jpg\n\n");

    printf("Detect edge in image for geometric object detection.\n\n");
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

            input_file = argv[optind++];
            if (access(input_file, R_OK)) {
                fprintf(stderr, "Error: input file not readable: %s\n", input_file);
                return 2;
            }

            output_file = argv[optind++];
            if (access(output_file, W_OK) && errno == EACCES) {
                fprintf(stderr, "Error: output file not writable: %s\n", output_file);
                return 2;
            }
            break;
        }

        switch (index) {
        // case OPTION_WIDTH:

        default:
            usage();
            return 1;
        }
    }

    // read input file
    cv::Mat input = cv::imread(input_file);

    // normalize input
    cv::Mat greyInput = input;

    if (input.channels() != 1) {
        cv::cvtColor(input, greyInput, cv::COLOR_RGB2GRAY);
    }

    // dicretize input
    greyInput /= 10;
    greyInput *= 10;
    cv::equalizeHist(greyInput, greyInput);

    cv::namedWindow("preview", cv::WINDOW_NORMAL);
    cv::imshow("preview", greyInput);
    while ((cv::waitKey(0) & 0xff) != '\n');

    // detect edges
    cv::Mat edges, greyEdges, finalEdges;

    cv::Laplacian(greyInput, edges, CV_32F, 1.0, 1.0);
    edges.convertTo(greyEdges, CV_8UC1);
    cv::equalizeHist(greyEdges, greyEdges);
    cv::threshold(greyEdges, finalEdges, 150.0, 0, cv::THRESH_TOZERO);

    // blur edges
    cv::Mat blurredEdges = finalEdges;

    cv::blur(finalEdges, blurredEdges, cv::Size(2, 2));

    cv::namedWindow("preview", cv::WINDOW_NORMAL);
    cv::imshow("preview", blurredEdges);
    while ((cv::waitKey(0) & 0xff) != '\n');
    return 0;
}
