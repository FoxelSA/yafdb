/*
 * gendataset - Generate a vector set for cascade training.
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

#define OPTION_WIDTH                  0
#define OPTION_HEIGHT                 1
#define OPTION_COUNT                  2
#define OPTION_ROTATE_STDDEV_X        3
#define OPTION_ROTATE_STDDEV_Y        4
#define OPTION_ROTATE_STDDEV_Z        5
#define OPTION_LUMINOSITY_STDDEV      6
#define OPTION_BACKGROUNDS            7

static int sample_width = 25;
static int sample_height = 25;
static int sample_count = 100;
static double rotate_stddev_x = 0.0 / 180.0 * M_PI;
static double rotate_stddev_y = 0.0 / 180.0 * M_PI;
static double rotate_stddev_z = 0.0 / 180.0 * M_PI;
static double luminosity_stddev = 0.0;
static const char *input_file = NULL;
static const char *output_file = NULL;
static const char *backgrounds_file = NULL;


static struct option options[] = {
    {"width",                required_argument, 0,                  0 },
    {"height",               required_argument, 0,                  0 },
    {"count",                required_argument, 0,                  0 },
    {"rotate-stddev-x",      required_argument, 0,                  0 },
    {"rotate-stddev-y",      required_argument, 0,                  0 },
    {"rotate-stddev-z",      required_argument, 0,                  0 },
    {"luminosity-stddev",    required_argument, 0,                  0 },
    {"backgrounds",          required_argument, 0,                  0 },
    {0, 0, 0, 0}
};


/**
 * Helpers.
 *
 */

static bool parseFiles(const std::string &input, std::vector<std::string> &output) {
    char tmp[8192];

    strncpy(tmp, input.c_str(), 8192);

    std::string basepath = dirname(tmp);
    std::stringstream buffer;
    auto add = [&] {
        std::string path(basepath + "/" + buffer.str());

        if (path.length() > 0) {
            if (access(path.c_str(), R_OK)) {
                fprintf(stderr, "Error: listed file not readable: %s\n", path.c_str());
                return false;
            }

            cv::Mat image = cv::imread(path);

            if (image.rows > 0 && image.cols > 0) {
                output.push_back(path);
            }
            buffer.str("");
        }
        return true;
    };
    std::ifstream file(input);
    int c;

    while ((c = file.get()) != EOF) {
        if (c != '\n') {
            buffer.put((char)c);
        } else {
            if (!add()) {
                return false;
            }
        }
    }
    return add();
}

void icvWriteVecHeader( FILE* file, int count, int width, int height )
{
    int vecsize;
    short tmp;

    /* number of samples */
    fwrite( &count, sizeof( count ), 1, file );
    /* vector size */
    vecsize = width * height;
    fwrite( &vecsize, sizeof( vecsize ), 1, file );
    /* min/max values */
    tmp = 0;
    fwrite( &tmp, sizeof( tmp ), 1, file );
    fwrite( &tmp, sizeof( tmp ), 1, file );
}

void icvWriteVecSample( FILE* file, CvArr* sample )
{
    CvMat* mat, stub;
    int r, c;
    short tmp;
    uchar chartmp;

    mat = cvGetMat( sample, &stub );
    chartmp = 0;
    fwrite( &chartmp, sizeof( chartmp ), 1, file );
    for( r = 0; r < mat->rows; r++ )
    {
        for( c = 0; c < mat->cols; c++ )
        {
            tmp = (short) (CV_MAT_ELEM( *mat, uchar, r, c ));
            fwrite( &tmp, sizeof( tmp ), 1, file );
        }
    }
}


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-gendataset --width 25 --height 25 input.txt output.vec\n\n");

    printf("Distords and prepares samples for training.\n\n");

    printf("General options:\n\n");
    printf("--width 25 : sample width in pixels\n");
    printf("--height 25 : sample height in pixels\n");
    printf("--count 100 : maximum number of samples to generate\n");
    printf("--show : show some samples\n");
    printf("\n");

    printf("Geometric distortion options:\n\n");
    printf("--rotate-stddev-x 0.0 : rotation angle 3-sigma (degree, x-axis)\n");
    printf("--rotate-stddev-y 0.0 : rotation angle 3-sigma (degree, y-axis)\n");
    printf("--rotate-stddev-z 0.0 : rotation angle 3-sigma (degree, z-axis)\n");
    printf("\n");

    printf("Noise distortion options:\n\n");
    printf("--luminosity-stddev 0.0 : luminosity factor 3-sigma\n");
    printf("--backgrounds bg.txt : list of background images\n");
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
        case OPTION_WIDTH:
            sample_width = atoi(optarg);
            break;
        case OPTION_HEIGHT:
            sample_height = atoi(optarg);
            break;
        case OPTION_COUNT:
            sample_count = atoi(optarg);
            break;

        case OPTION_ROTATE_STDDEV_X:
            rotate_stddev_x = atof(optarg) / 180.0 * M_PI;
            break;
        case OPTION_ROTATE_STDDEV_Y:
            rotate_stddev_y = atof(optarg) / 180.0 * M_PI;
            break;
        case OPTION_ROTATE_STDDEV_Z:
            rotate_stddev_z = atof(optarg) / 180.0 * M_PI;
            break;

        case OPTION_LUMINOSITY_STDDEV:
            luminosity_stddev = atof(optarg);
            break;

        case OPTION_BACKGROUNDS:
            backgrounds_file = optarg;
            if (access(backgrounds_file, R_OK)) {
                fprintf(stderr, "Error: backgrounds file not readable: %s\n", backgrounds_file);
                return 2;
            }
            break;

        default:
            usage();
            return 1;
        }
    }

    // read input files
    std::vector<std::string> samples;

    if (!parseFiles(input_file, samples)) {
        fprintf(stderr, "Error: cannot parse file listing: %s\n", input_file);
        return 2;
    }

    // read background files
    std::vector<std::string> backgrounds;

    if (backgrounds_file != NULL && !parseFiles(backgrounds_file, backgrounds)) {
        fprintf(stderr, "Error: cannot parse file listing: %s\n", backgrounds_file);
        return 2;
    }

    // create output file
    FILE *fp = fopen(output_file, "wb");

    if (fp == NULL) {
        fprintf(stderr, "Error: cannot open output file for writing: %s\n", output_file);
        return 2;
    }
    icvWriteVecHeader(fp, sample_count, sample_width, sample_height);

    // generate distortions
    std::default_random_engine generator(time(NULL));
    std::normal_distribution<double> xdist(0.0, rotate_stddev_x / 3.0);
    std::normal_distribution<double> ydist(0.0, rotate_stddev_y / 3.0);
    std::normal_distribution<double> zdist(0.0, rotate_stddev_z / 3.0);
    std::normal_distribution<double> ldist(0.0, luminosity_stddev / 3.0);
    cv::Mat el = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    int variations = MAX(1, (int)floor((double)sample_count / (double)samples.size()));
    int idx = 0;
    int i = 0;

    while (i < sample_count) {
        // suffle the input lists
        if (idx % samples.size() == 0) {
            std::shuffle(samples.begin(), samples.end(), generator);
            std::shuffle(backgrounds.begin(), backgrounds.end(), generator);
        }

        // read sample image
        auto const &sample_file(samples[idx % samples.size()]);
        cv::Mat sample = cv::imread(sample_file);
        double sampleRatio = (double)sample.cols / (double)sample.rows;
        double outputRatio = (double)sample_width / (double)sample_height;

        // normalize sample
        cv::Mat greySample = sample;
        double min, max;

        if (sample.channels() != 1) {
            cv::cvtColor(sample, greySample, cv::COLOR_RGB2GRAY);
        }
        cv::minMaxIdx(greySample, &min, &max);
        greySample -= min;
        greySample /= (max - min) / 255.0;

        // generate mask
        cv::Mat mask(cv::Mat::ones(greySample.rows, greySample.cols, greySample.type()));

        // enlarge canvas to fit output ratio
        cv::Mat resizedSample, resizedMask;

        if (backgrounds.size() > 0 && sampleRatio < outputRatio) {
            int width = (int)((double)greySample.rows * outputRatio);
            cv::Rect area(
                (width - greySample.cols) / 2,
                0,
                greySample.cols,
                greySample.rows
            );

            resizedSample = cv::Mat::zeros(greySample.rows, width, greySample.type());
            resizedMask = cv::Mat::zeros(greySample.rows, width, greySample.type());
            greySample.copyTo(resizedSample(area));
            mask.copyTo(resizedMask(area));
        } else if (backgrounds.size() > 0 && sampleRatio > outputRatio) {
            int height = (int)((double)greySample.cols / outputRatio);
            cv::Rect area(
                0,
                (height - greySample.rows) / 2,
                greySample.cols,
                greySample.rows
            );

            resizedSample = cv::Mat::zeros(height, greySample.cols, greySample.type());
            resizedMask = cv::Mat::zeros(height, greySample.cols, greySample.type());
            greySample.copyTo(resizedSample(area));
            mask.copyTo(resizedMask(area));
        } else {
            resizedSample = greySample;
            resizedMask = mask;
        }

        // apply distortions
        cv::Mat target(resizedSample.rows, resizedSample.cols, resizedSample.type());
        cv::Mat targetMask(resizedSample.rows, resizedSample.cols, resizedSample.type());
        double halfWidth = resizedSample.cols / 2.0;
        double halfHeight = resizedSample.rows / 2.0;
        cv::Mat rotationVector(3, 1, CV_64FC1);
        cv::Mat rotation4(cv::Mat::eye(4, 4, CV_64FC1));
        cv::Mat translate4(cv::Mat::eye(4, 4, CV_64FC1));
        cv::Mat translate3(cv::Mat::eye(3, 3, CV_64FC1));
        cv::Mat scale3(cv::Mat::eye(3, 3, CV_64FC1));
        int dx = (resizedSample.cols - greySample.cols) / 2;
        int dy = (resizedSample.rows - greySample.rows) / 2;
        cv::Point2f points1[4] = {
            cv::Point2f(dx,              dy),
            cv::Point2f(dx,              greySample.rows),
            cv::Point2f(greySample.cols, greySample.rows),
            cv::Point2f(greySample.cols, dy)
        };
        cv::Point2f points2[4];

        translate4.at<double>(0, 3) = -halfWidth;
        translate4.at<double>(1, 3) = -halfHeight;
        for (int k = 0; k < variations; k++) {
            double rx = k > 0 && rotate_stddev_x > 0.0 ? xdist(generator) : 0.0;
            double ry = k > 0 && rotate_stddev_y > 0.0 ? ydist(generator) : 0.0;
            double rz = k > 0 && rotate_stddev_z > 0.0 ? zdist(generator) : 0.0;
            double rl = k > 0 && luminosity_stddev > 0.0 ? ldist(generator) : 0.0;

            // compute rotation in 3d
            rotationVector.at<double>(0) = rx;
            rotationVector.at<double>(1) = ry;
            rotationVector.at<double>(2) = rz;
            cv::Rodrigues(rotationVector, cv::Mat(rotation4, cv::Rect(0, 0, 3, 3)));

            // compute transformation in 3d
            cv::Mat transform4(rotation4 * translate4);
            double minx = DBL_MAX, miny = DBL_MAX;
            double maxx = DBL_MIN, maxy = DBL_MIN;

            for (int j = 0; j < 4; j++) {
                cv::Mat point(4, 1, CV_64FC1);

                point.at<double>(0) = points1[j].x;
                point.at<double>(1) = points1[j].y;
                point.at<double>(2) = 0.0;
                point.at<double>(3) = 1.0;
                point = transform4 * point;
                points2[j].x = point.at<double>(0);
                points2[j].y = point.at<double>(1);

                if (points2[j].x < minx) {
                    minx = points2[j].x;
                }
                if (points2[j].x > maxx) {
                    maxx = points2[j].x;
                }
                if (points2[j].y < miny) {
                    miny = points2[j].y;
                }
                if (points2[j].y > maxy) {
                    maxy = points2[j].y;
                }
            }

            // compute transformation in 2d
            cv::Mat projection3(cv::getPerspectiveTransform(points1, points2));
            double scalex = (resizedSample.cols - dx) / (maxx - minx);
            double scaley = (resizedSample.rows - dy) / (maxy - miny);

            translate3.at<double>(0, 2) = halfWidth;
            translate3.at<double>(1, 2) = halfHeight;

            scale3.at<double>(0, 0) = scalex; //MIN(scalex, scaley);
            scale3.at<double>(1, 1) = scaley; //MIN(scalex, scaley);

            // transform sample and mask in 2d
            cv::Mat transform3(translate3 * scale3 * projection3);

            cv::warpPerspective(resizedSample, target, transform3, target.size());
            cv::warpPerspective(resizedMask, targetMask, transform3, targetMask.size());

            // apply luminosity change
            if (rl != 0.0) {
                rl += 1.0;
                target *= rl;
            }

            // read background image
            cv::Mat greyBackground;

            if (backgrounds.size() > 0) {
                auto const &background_file(backgrounds[i % backgrounds.size()]);
                cv::Mat background = cv::imread(background_file);

                // normalize background image
                if (background.channels() != 1) {
                    cv::cvtColor(background, greyBackground, cv::COLOR_RGB2GRAY);
                } else {
                    greyBackground = background;
                }
                cv::minMaxIdx(greyBackground, &min, &max);
                greyBackground -= min;
                greyBackground /= (max - min) / 255.0;

                // reshape background to fit output ratio
                double backgroundRatio = (double)greyBackground.cols / (double)greyBackground.rows;
                cv::Mat tmp;

                if (backgroundRatio < outputRatio) {
                    int height = (int)((double)greyBackground.cols / outputRatio);
                    std::uniform_int_distribution<int> hdist(0, greyBackground.rows - height);

                    tmp = greyBackground(
                        cv::Rect(
                            0,
                            hdist(generator),
                            greyBackground.cols,
                            height
                        )
                    );
                } else if (backgroundRatio > outputRatio) {
                    int width = (int)((double)greyBackground.rows * outputRatio);
                    std::uniform_int_distribution<int> wdist(0, greyBackground.cols - width);

                    tmp = greyBackground(
                        cv::Rect(
                            wdist(generator),
                            0,
                            width,
                            greyBackground.rows
                        )
                    );
                } else {
                    tmp = greyBackground;
                }
                cv::resize(tmp, greyBackground, resizedSample.size(), 0, 0, cv::INTER_CUBIC);
            } else {
                // random noise background
                greyBackground = cv::Mat(target.rows, target.cols, CV_8UC1);
                cv::randn(greyBackground, 255.0 / 2, 255.0 / 2 / 3);
                cv::GaussianBlur(greyBackground, greyBackground, cv::Size(5, 5), 10);
            }

            // blend background
            cv::Mat sampleMask, backgroundMask, tmp;

            cv::threshold(targetMask, sampleMask, 0.1, 255.0, cv::THRESH_BINARY);
            cv::erode(sampleMask, tmp, el);
            cv::blur(tmp, sampleMask, cv::Size(5, 5));

            cv::threshold(targetMask, backgroundMask, 0.1, 255.0, cv::THRESH_BINARY_INV);
            cv::dilate(backgroundMask, tmp, el);
            cv::blur(tmp, backgroundMask, cv::Size(5, 5));

            cv::multiply(target, sampleMask, target, 1.0 / 255.0);
            cv::multiply(greyBackground, backgroundMask, greyBackground, 1.0 / 255.0);

            target += greyBackground;

            // cv::namedWindow("preview", cv::WINDOW_NORMAL);
            // cv::imshow("preview", target);
            // while ((cv::waitKey(0) & 0xff) != '\n');

            // cv::namedWindow("preview", cv::WINDOW_NORMAL);
            // cv::imshow("preview", greyBackground);
            // while ((cv::waitKey(0) & 0xff) != '\n');

            // sample resize
            cv::Mat finalSample;

            cv::resize(target, finalSample, cv::Size(sample_width, sample_height), 0, 0, cv::INTER_CUBIC);

            // cv::namedWindow("preview", cv::WINDOW_NORMAL);
            // cv::imshow("preview", finalSample);
            // while ((cv::waitKey(0) & 0xff) != '\n');

            // sample save
            CvMat targetfinal_ = finalSample;

            icvWriteVecSample(fp, &targetfinal_);

            i++;
            if (i % 100 == 0) {
                fprintf(stdout, "processed %d images, %d samples\n", idx, i);
                fflush(stdout);
            }
        }
        idx++;
    }

    // close output file
    fclose(fp);
    return 0;
}
