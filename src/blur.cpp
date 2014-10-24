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
#include <math.h>

#include "detectors/detector.hpp"


/*
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE         0
#define ALGORITHM_GAUSSIAN     1
#define ALGORITHM_PROGRESSIVE  2


/*
 * Program arguments.
 *
 */

#define OPTION_ALGORITHM              0
#define OPTION_MERGE_DISABLE          1
#define OPTION_MERGE_MIN_OVERLAP      2
#define OPTION_GAUSSIAN_KERNEL        3
#define OPTION_GAUSSIAN_STEPS         4
#define OPTION_MAGNIFY_FACTOR         5
#define OPTION_RESIZE_WIDTH           6
#define OPTION_RESIZE_HEIGHT          7

static int resize_width = 0;
static int resize_height = 0;
static int algorithm = ALGORITHM_GAUSSIAN;
static int merge_min_overlap = 1;
static int merge_enabled = 1;
static double gaussian_kernel_size = 65;
static double gaussian_steps = 1;
static double magnify_factor = 1.0;
static const char *source_file = NULL;
static const char *objects_file = NULL;
static const char *target_file = NULL;


static struct option options[] = {
    {"algorithm",           required_argument, 0,                 'a'},
    {"merge-disable",       no_argument,       &merge_enabled,     0 },
    {"merge-min-overlap",   required_argument, 0,                  0 },
    {"gaussian-kernel",     required_argument, 0,                  0 },
    {"gaussian-steps",      required_argument, 0,                  0 },
    {"magnify-factor",      required_argument, 0,                  0 },
    {"resize-width",        required_argument, 0,                  0 },
    {"resize-height",       required_argument, 0,                  0 },
    {0, 0, 0, 0}
};

//! Progressive rectangular blurring

//! Apply a progresive blur on the desired rectangle.
//!
//! @param pbBitmap Bitmap array pointer to result image
//! @param pbSource Bitmap array pointer to source image
//! @param pbWidth Bitmap width, in pixels
//! @param pbHeight Bitmap height, in pixels
//! @param pbChannel Layer on which blur apply
//! @param pbRx1 Rectangle upper left corner x coordinates
//! @param pbRy1 Rectangle upper left corner y coordinates
//! @param pbRx2 Rectangle lower right corner x coordinates
//! @param pbRy2 Rectangle lower right corner y coordinates

void progblur ( unsigned char * pbBitmap, int pbWidth, int pbHeight, int pbChannel, int pbRx1, int pbRy1, int pbRx2, int pbRy2 ) {

    /* Parsing variables */
    int pbX = 0;
    int pbY = 0;
    int pbI = 0;
    int pbJ = 0;

    /* Function parameters */
    float pbU = 0.0;
    float pbV = 0.0;

    /* Area boudaries */
    int pbAX1 = 0;
    int pbAY1 = 0;
    int pbAX2 = 0;
    int pbAY2 = 0;

    /* Compute rectangle size */
    int pbrWidth  = ( pbRx2 - pbRx1 );
    int pbrHeight = ( pbRy2 - pbRy1 );

    /* Compute rectangle center */
    float pbCX = ( float ) pbrWidth  / 2.0 + pbRx1;
    float pbCY = ( float ) pbrHeight / 2.0 + pbRy1;

    /* Compute automatic factor */
    float pbFactor = pbrWidth < pbrHeight ? pbrWidth : pbrHeight;

    /* Chromaitc accumulator */
    float pbAccumR = 0.0;
    float pbAccumG = 0.0;
    float pbAccumB = 0.0;
    int   pbForce  = 0;
    int   pbCount  = 0;

    /* Optimization for bitmap offset */
    unsigned char * pbOffset = NULL;

    /* Optimization for bitmap sub-offset */
    int pbYOffset = 0;
    int pbJOffset = 0;

    /* Increase rectangle size */
    pbRx1 = pbCX - pbFactor;
    pbRx2 = pbCX + pbFactor;
    pbRy1 = pbCY - pbFactor;
    pbRy2 = pbCY + pbFactor;

    /* Recompute adapted width and height */
    pbrWidth  = ( pbRx2 - pbRx1 );
    pbrHeight = ( pbRy2 - pbRy1 );

    /* Optimization operation */
    pbFactor *= 0.2;

    /* Blurring y-component loop */
    for ( pbY = pbRy1; pbY <= pbRy2; pbY ++ ) {

        /* Compute optimization sub-offset */
        pbYOffset = pbChannel * pbWidth * pbY;

        /* Compute coordinates v-parameters */
        pbV = ( ( float ) ( pbY - pbRy1 ) / pbrHeight ) * 2.0 - 1.0;

        /* Blurring x-component loop */
        for ( pbX = pbRx1; pbX <= pbRx2; pbX ++ ) {

            /* Reset chromatic accumulator */
            pbAccumR = 0.0;
            pbAccumG = 0.0;
            pbAccumB = 0.0;

            /* Compute coordinates u-parameters */
            pbU = ( ( float ) ( pbX - pbRx1 ) / pbrWidth  ) * 2.0 - 1.0;

            /* Compute recursive condition value */
            pbForce = int( exp( - pbU * pbU * 3.5 ) * exp( - pbV * pbV * 3.5 ) * pbFactor );

            pbForce = ( pbForce > 32 ) ? 32 : pbForce;

            /* Create area boundaries */
            pbAX1 = pbX - pbForce; pbAX1 = ( pbAX1 <         0 ) ?            0 : pbAX1;
            pbAX2 = pbX + pbForce; pbAX2 = ( pbAX2 >=  pbWidth ) ? pbWidth  - 1 : pbAX2;
            pbAY1 = pbY - pbForce; pbAY1 = ( pbAY1 <         0 ) ?            0 : pbAY1;
            pbAY2 = pbY + pbForce; pbAY2 = ( pbAY2 >= pbHeight ) ? pbHeight - 1 : pbAY2;

            /* Accumulates y-components */
            for ( pbJ = pbAY1; pbJ <= pbAY2; pbJ ++ ) {

                /* Compute optimization sub-offset */
                pbJOffset = pbChannel * pbWidth * pbJ;

                /* Accumulates x-components */
                for ( pbI = pbAX1; pbI <= pbAX2; pbI ++ ) {

                    /* Compute optimiztion offset */
                    pbOffset = pbBitmap + pbJOffset + pbChannel * pbI;

                    /* Accumulate chromatic value */
                    pbAccumR += * ( pbOffset ++ );
                    pbAccumG += * ( pbOffset ++ );
                    pbAccumB += * ( pbOffset    );

                }

            }

            /* Compute number of considered pixels */
            pbCount = ( pbAX2 - pbAX1 + 1 ) * ( pbAY2 - pbAY1 + 1 );

            /* Compute optimization offset */
            pbOffset = pbBitmap + pbYOffset + pbChannel * pbX;

            /* Assign mean value */
            * ( pbOffset ++ ) = pbAccumR / pbCount;
            * ( pbOffset ++ ) = pbAccumG / pbCount;
            * ( pbOffset    ) = pbAccumB / pbCount;

        }

    }

}

//! Rectangle magnifier

//! Magnify specified rectangle by a multiplier
//!
//! @param multiplier Multiplier
//! @param source_x x cordinates of source rectangle
//! @param source_y y cordinates of source rectangle
//! @param source_w Width of source rectangle
//! @param source_h Height of source rectangle
//! @param dest_x   Destination variable to store magnified x coordinates
//! @param dest_y   Destination variable to store magnified y coordinates
//! @param dest_xw  Destination variable to store magnified x + Width
//! @param dest_yh  Destination variable to store magnified y + Height

void magnifyRect
(
    float multiplier,
    float source_x,  float source_y,
    float source_w,  float source_h,
    float * dest_x,  float * dest_y,
    float * dest_xw, float * dest_yh
)
{
    /* Determine center of coordinates */
    float xc = ((source_x * 2) + source_w) / 2;
    float yc = ((source_y * 2) + source_h) / 2;

    /* Determine the frame of coordinates */
    float x_p = (source_x - xc) * multiplier;
    float y_p = (source_y - yc) * multiplier;

    /* Apply multiplier */
    float x_p2 = ((source_x + source_w) - xc) * multiplier;
    float y_p2 = ((source_y + source_h) - yc) * multiplier;

    /* Compute results */
    *dest_x = (x_p + xc);
    *dest_y = (y_p + yc);

    *dest_xw = (x_p2 + xc);
    *dest_yh = (y_p2 + yc);
}

/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-blur --algorithm algo input-image.tiff input-objects.yaml output-image.tiff\n\n");

    printf("Blurs detected objects and write modified image as output.\n\n");

    printf("General options:\n\n");
    printf("--algorithm algo : algorithm to use for blurring ('gaussian', 'progressive')\n");
    printf("--merge-disable: don't merge overlapping rectangles\n");
    printf("--merge-min-overlap 1 : minimum occurrence of overlap to keep detected objects\n");
    printf("\n");

    printf("Gaussian options:\n\n");
    printf("--gaussian-kernel 65 : gaussian kernel size\n");
    printf("--gaussian-steps 1 : gaussian blurring steps\n");
    printf("\n");

    printf("Progressive options:\n\n");
    printf("--magnify-factor 1.0 : Rectangles magnify factor\n");
    printf("\n");

    printf("Resizing options:\n\n");
    printf("--resize-width 800: Resizing width\n");
    printf("--resize-height 600: Resizing height\n");
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
            } else if (strcmp(optarg, "progressive") == 0) {
                algorithm = ALGORITHM_PROGRESSIVE;
            } else {
                fprintf(stderr, "Error: unsupported algorithm: %s\n", optarg);
            }
            break;

        case OPTION_MERGE_DISABLE:
            break;
        case OPTION_MERGE_MIN_OVERLAP:
            merge_min_overlap = atoi(optarg);
            break;
        case OPTION_GAUSSIAN_KERNEL:
            gaussian_kernel_size = atof(optarg);
            break;
        case OPTION_GAUSSIAN_STEPS:
            gaussian_steps = atof(optarg);
            break;
        case OPTION_MAGNIFY_FACTOR:
            magnify_factor = atof(optarg);
            break;
        case OPTION_RESIZE_WIDTH:
            resize_width = atoi(optarg);
            break;
        case OPTION_RESIZE_HEIGHT:
            resize_height = atoi(optarg);
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

    // apply blur operation
    for (int i = 0; i < gaussian_steps; ++i)
    {
        std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
            if(object.falsePositive != "Yes")
            {
                auto rects = object.area.rects(source.cols, source.rows);

                std::for_each(rects.begin(), rects.end(), [&] (const cv::Rect &rect) {
                    cv::Mat region(source, rect);

                    switch (algorithm) {
                    case ALGORITHM_NONE:
                        break;

                    case ALGORITHM_GAUSSIAN:
                    {
                        GaussianBlur(
                            region,
                            region,
                            cv::Size(gaussian_kernel_size, gaussian_kernel_size),
                            0,
                            0
                        );
                    }
                    break;

                    case ALGORITHM_PROGRESSIVE:
                    {

                        // Check presence of magnify parameter
                        if(magnify_factor != 1.0)
                        {
                            // Magnifier variables
                            float magnified_x  = 0.0;
                            float magnified_y  = 0.0;
                            float magnified_xw = 0.0;
                            float magnified_yh = 0.0;

                            // Magnify rectangle
                            magnifyRect(
                                magnify_factor,
                                rect.x,
                                rect.y,
                                rect.width,
                                rect.height,
                                &magnified_x,
                                &magnified_y,
                                &magnified_xw,
                                &magnified_yh
                            );

                            // Apply progressive blur
                            progblur (
                                source.data,
                                source.cols,
                                source.rows,
                                source.channels(),
                                magnified_x,
                                magnified_y,
                                magnified_xw,
                                magnified_yh
                            );
                        } else {

                            // Apply progressive blur
                            progblur (
                                source.data,
                                source.cols,
                                source.rows,
                                source.channels(),
                                rect.x,
                                rect.y,
                                rect.x + rect.width,
                                rect.y + rect.height
                            );
                        }
                    }
                    break;

                    default:
                        fprintf(stderr, "Error: unsupported blur algorithm!\n");
                        break;
                    }
                });
            }
        });
    }

    // Configure the quality level for jpeg images
    std::vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

    // Resize the image if specified
    if(resize_width > 0 && resize_height > 0)
    {
        // Create the resized image
        cv::Size size(resize_width, resize_height);
        cv::Mat resized_image;
        cv::resize(source, resized_image, size);

        // save target file
        cv::imwrite(target_file, resized_image, compression_params);
    } else {
        // save target file
        cv::imwrite(target_file, source, compression_params);
    }

    return 0;
}
