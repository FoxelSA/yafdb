/*
 * features - extract features from given images.
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

#include <algorithm>
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


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-features input.txt output.yaml\n\n");

    printf("Detect features in images for matching later.\n\n");
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

    // read input files
    std::vector<std::string> samples;

    if (!parseFiles(input_file, samples)) {
        fprintf(stderr, "Error: cannot parse file listing: %s\n", input_file);
        return 2;
    }

    // detect features in input files
    std::for_each(samples.begin(), samples.end(), [] (const std::string &sample_file) {
        // read input file
        cv::Mat sample = cv::imread(sample_file);
        double min, max;

        // normalize sample
        cv::minMaxIdx(sample, &min, &max);
        sample -= min;
        sample /= (max - min) / 255.0;

        // greyify sample
        cv::Mat greySample = sample;

        if (sample.channels() != 1) {
            cv::cvtColor(sample, greySample, cv::COLOR_RGB2GRAY);
        }
        cv::GaussianBlur(greySample, greySample, cv::Size(5, 5), 1);

        // preview sample
        cv::namedWindow("preview", cv::WINDOW_NORMAL);
        cv::imshow("preview", greySample);
        while ((cv::waitKey(0) & 0xff) != '\n');

        // // find edges
        // cv::Mat edges, greyEdges;

        // cv::Laplacian(greySample, edges, CV_32F, 1.0, 1.0);
        // edges.convertTo(greyEdges, CV_8UC1);

        // // normalize edges
        // cv::minMaxIdx(greyEdges, &min, &max);
        // greyEdges -= min;
        // greyEdges /= (max - min) / 255.0;

        // cv::adaptiveThreshold(greyEdges, threshold, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 15, 5);

        // // preview edges
        // cv::namedWindow("preview", cv::WINDOW_NORMAL);
        // cv::imshow("preview", greyEdges);
        // while ((cv::waitKey(0) & 0xff) != '\n');

        // find lines
        cv::Mat threshold = greySample;
        // cv::Mat el = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::Mat el = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4));

        cv::adaptiveThreshold(greySample, threshold, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 15, 10);
        // cv::erode(threshold, threshold, el, cv::Point(-1, -1), 1);
        cv::dilate(threshold, threshold, el, cv::Point(-1, -1), 1);
        cv::erode(threshold, threshold, el, cv::Point(-1, -1), 1);

        // preview lines
        cv::namedWindow("preview", cv::WINDOW_NORMAL);
        cv::imshow("preview", threshold);
        while ((cv::waitKey(0) & 0xff) != '\n');

        // find contours
        // std::vector<std::vector<cv::Point> > contours;
        // std::vector<cv::Vec4i> hierarchy;

        // cv::findContours(threshold, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_TC89_L1);
        // cv::drawContours(sample, contours, -1, cv::Scalar(0, 0, 255), 1, CV_AA, hierarchy);

        // printf("contours: %d\n", contours.size());

        // find lines
        // std::vector<cv::Vec4i> lines;
        std::vector<cv::Vec2f> lines;

        cv::HoughLines(threshold, lines, 1, CV_PI / 180, (int)cv::min(sample.rows / 1.5, sample.cols / 1.5));
        std::for_each(lines.begin(), lines.end(), [&] (const cv::Vec2f &line) {
            float rho = line[0];
            float theta = line[1];
            double a = cos(theta);
            double b = sin(theta);
            double xi = a * rho;
            double yi = b * rho;
            double w = (sample.cols / 2 - xi) / b;
            double h = (sample.rows / 2 - yi) / a;
            // double x0a = xi + w * b;
            double y0a = yi - w * a;
            double x0b = xi - h * b;
            // double y0b = yi + h * a;
            cv::Point p1(
                cvRound(xi - 1000 * b),
                cvRound(yi + 1000 * a)
            );
            cv::Point p2(
                cvRound(xi + 1000 * b),
                cvRound(yi - 1000 * a)
            );

            if (x0b >= (sample.cols / 4) && x0b <= (3 * sample.cols / 4)) {
                return;
            }
            if (y0a >= (sample.rows / 4) && y0a <= (3 * sample.rows / 4)) {
                return;
            }

            // cv::circle(sample, cv::Point(xi, yi), 3, cv::Scalar(255, 0, 0), 1);
            // cv::circle(sample, cv::Point(x0b, y0b), 3, cv::Scalar(0, 255, 0), 1);
            cv::line(sample, p1, p2, cv::Scalar(0, 0, 255), 1);
        });

        // cv::HoughLinesP(threshold, lines, 1, CV_PI / 100, 10, cv::min(sample.rows / 2, sample.cols / 2), 0);
        // std::for_each(lines.begin(), lines.end(), [&] (const cv::Vec4i &line) {
        //     cv::line(sample, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), cv::Scalar(0, 0, 255), 1);
        // });

        // preview contours
        cv::namedWindow("preview", cv::WINDOW_NORMAL);
        cv::imshow("preview", sample);
        while ((cv::waitKey(0) & 0xff) != '\n');

        // cv::resize(threshold, threshold, sample.size());
        // cv::cvtColor(threshold, greySample, cv::COLOR_GRAY2RGB);

        // cv::Mat preview = greySample;

        // cv::multiply(greySample, sample, preview, 1 / 255.0);

        // // detect features
        // cv::BRISK detector;
        // std::vector<cv::KeyPoint> keypoints;

        // detector.detect(greySample, keypoints);

        // // compute descriptors
        // cv::Mat descriptors;

        // detector.compute(greySample, keypoints, descriptors);

        // printf("keypoints: %d, descriptors: %d x %d\n", keypoints.size(), descriptors.rows, descriptors.cols);

        // // harris
        // cv::Mat harris;
        // double min, max;

        // cv::cornerHarris(greySample, harris, 2, 1, 10);
        // cv::minMaxIdx(harris, &min, &max);
        // harris -= min;
        // harris /= max - min;

        // preview features
        // cv::Mat preview;

        // cv::drawKeypoints(sample, keypoints, preview, cv::Scalar(0, 0, 255), cv:: DrawMatchesFlags::DEFAULT);

        // cv::namedWindow("preview", cv::WINDOW_NORMAL);
        // cv::imshow("preview", preview);
        // while ((cv::waitKey(0) & 0xff) != '\n');
    });

    // train
    return 0;
}
