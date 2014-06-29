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

#define OPTION_PREVIEW              0

static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *mask_file = NULL;

static struct option options[] = {
    {"preview",         required_argument,       0, 0},
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-test input-objects.yml mask-image.png\n\n");

    printf("Compute the detection error rate by comparing optimal area given in\n");
    printf("reference bitmap mask (black=none, white=object) to the detected area\n");
    printf("given in input text file.\n\n");

    printf("General options:\n\n");
    printf("--preview input-image.tiff: preview detection errors\n");
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
            if (argc != optind + 2) {
                usage();
                return 1;
            }

            objects_file = argv[optind++];
            if (access(objects_file, R_OK)) {
                fprintf(stderr, "Error: detected objects file not readable: %s\n", objects_file);
                return 2;
            }

            mask_file = argv[optind++];
            if (access(mask_file, R_OK)) {
                fprintf(stderr, "Error: mask file not readable: %s\n", mask_file);
                return 2;
            }
            break;
        }

        switch (index) {
        case OPTION_PREVIEW:
            if (access(optarg, R_OK)) {
                fprintf(stderr, "Error: source file not readable: %s\n", optarg);
                return 2;
            }
            source_file = optarg;
            break;

        default:
            usage();
            return 1;
        }
    }

    // read mask file
    cv::Mat mask = cv::imread(mask_file);

    if (mask.rows <= 0 || mask.cols <= 0) {
        fprintf(stderr, "Error: cannot read image in mask file: %s\n", mask_file);
        return 2;
    }
    if (mask.channels() != 1) {
        cv::Mat tmp;

        cv::cvtColor(mask, tmp, cv::COLOR_RGB2GRAY);
        mask = tmp;
    }

    // read detected objects
    std::list<DetectedObject> objects;

    if (!ObjectDetector::load(objects_file, objects)) {
        fprintf(stderr, "Error: cannot read objects in file: %s\n", objects_file);
        return 2;
    }

    // compute detected mask
    cv::Mat detected(mask.rows, mask.cols, CV_8UC1, cv::Scalar(0));

    std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
        auto rects = object.area.rects(mask.cols, mask.rows);

        std::for_each(rects.begin(), rects.end(), [&] (const cv::Rect &rect) {
            rectangle(detected, rect, cv::Scalar(255), CV_FILLED);
        });
    });

    // compute false positive mask (type I error)
    cv::Mat falsePositives(mask.rows, mask.cols, CV_8UC1);

    compare(detected, mask, falsePositives, cv::CMP_GT);

    // compute false negative mask (type II error)
    cv::Mat falseNegatives(mask.rows, mask.cols, CV_8UC1);

    compare(detected, mask, falseNegatives, cv::CMP_LT);

    // report error rates
    double totalPixels = mask.cols * mask.rows;
    double positivePixels = sum(mask)[0] / 255.0;
    double falsePositivesRate = (sum(falsePositives)[0] / 255.0) / (totalPixels - positivePixels);
    double falseNegativesRate = (sum(falseNegatives)[0] / 255.0) / positivePixels;

    printf("falsePositivesRatio: %.03f %%\n", falsePositivesRate * 100.0);
    printf("falseNegativesRatio: %.03f %%\n", falseNegativesRate * 100.0);

    // preview errors
    if (source_file != NULL) {
        // compute correct mask
        cv::Mat correctMask(mask.rows, mask.cols, CV_8UC1);

        multiply(detected, mask, correctMask);

        // errors overlay
        cv::Mat colors(mask.rows, mask.cols, CV_8UC3, cv::Scalar(0, 0, 0));

        colors.setTo(cv::Scalar(0, 255.0, 0), correctMask);
        colors.setTo(cv::Scalar(0, 0, 255.0), falsePositives);
        colors.setTo(cv::Scalar(255.0, 0, 0), falseNegatives);

        correctMask.release();
        falsePositives.release();
        falseNegatives.release();

        // read source file
        cv::Mat source = cv::imread(source_file);

        if (source.rows != mask.rows || source.cols != mask.cols) {
            fprintf(stderr, "Error: cannot read image in source file or incompatible with mask size: %s\n", source_file);
            return 2;
        }

        // display errors
        cv::namedWindow("preview", cv::WINDOW_NORMAL);
        cv::imshow("preview", 0.5 * source + colors);
        while (cv::waitKey(0) != '\n');
    }
    return 0;
}
