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
#include <sys/stat.h>

#include "detectors/detector.hpp"
#include "detectors/multi.hpp"
#include "detectors/hierarchical.hpp"
#include "detectors/gnomonic.hpp"
#include "detectors/haar.hpp"


/*
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE      0
#define ALGORITHM_HAAR      1


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

#define OPTION_ALGORITHM              0
#define OPTION_EXPORT_FORMAT          1
#define OPTION_EXPORT_PATH            2
#define OPTION_GNOMONIC               3
#define OPTION_GNOMONIC_WIDTH         4
#define OPTION_GNOMONIC_APERTURE_X    5
#define OPTION_GNOMONIC_APERTURE_Y    6
#define OPTION_HAAR_MODEL             7
#define OPTION_HAAR_SCALE             8
#define OPTION_HAAR_MIN_OVERLAP       9


class HaarModel;

static int algorithm = ALGORITHM_HAAR;
static int export_format = FORMAT_PNG;
static const char *export_path = NULL;
static int gnomonic_enabled = 0;
static int gnomonic_width = 2048;
static double gnomonic_aperture_x = 60;
static double gnomonic_aperture_y = 60;
static std::map<std::string, HaarModel> haar_models;
static double haar_scale = 1.1;
static int haar_min_overlap = 3;
static const char *source_file = NULL;
static const char *objects_file = NULL;


static struct option options[] = {
    {"algorithm",            required_argument, 0,                 'a'},
    {"export-format",        required_argument, 0,                  0 },
    {"export-path",          required_argument, 0,                  0 },
    {"gnomonic",             no_argument,       &gnomonic_enabled,  1 },
    {"gnomonic-width",       required_argument, 0,                  0 },
    {"gnomonic-aperture-x",  required_argument, 0,                  0 },
    {"gnomonic-aperture-y",  required_argument, 0,                  0 },
    {"haar-model",           required_argument, 0,                  0 },
    {"haar-scale",           required_argument, 0,                  0 },
    {"haar-min-overlap",     required_argument, 0,                  0 },
    {0, 0, 0, 0}
};


class HaarModel {
public:
    std::string className;
    std::string file;
    int minOccurences;
    int maxOccurences;
    int minChildOccurences;
    int maxChildOccurences;
    std::map<std::string, HaarModel> children;


    HaarModel() : minOccurences(0), maxOccurences(-1), minChildOccurences(0), maxChildOccurences(-1) {
    }

    HaarModel(const HaarModel &ref) : className(ref.className), file(ref.file), minOccurences(ref.minOccurences), maxOccurences(ref.maxOccurences), minChildOccurences(ref.minChildOccurences), maxChildOccurences(ref.maxChildOccurences), children(ref.children) {
    }


    bool check() const {
        if (access(this->file.c_str(), R_OK)) {
            fprintf(stderr, "Error: haar model file not readable: %s (class: %s)\n", this->file.c_str(), this->className.c_str());
            return false;
        }
        std::all_of(this->children.begin(), this->children.end(), [&] (const std::pair<std::string, HaarModel> &pair) {
            return pair.second.check();
        });
        return true;
    }

    void write(cv::FileStorage &fs) const {
        fs << "{";
        fs << "className" << this->className;
        fs << "model" << this->file;
        fs << "minOccurences" << this->minOccurences;
        fs << "maxOccurences" << this->maxOccurences;
        fs << "minChildOccurences" << this->minChildOccurences;
        fs << "maxChildOccurences" << this->maxChildOccurences;
        if (!this->children.empty()) {
            fs << "children" << "[";
            std::for_each(this->children.begin(), this->children.end(), [&] (const std::pair<std::string, HaarModel> &pair) {
                pair.second.write(fs);
            });
            fs << "]";
        }
        fs << "}";
    }

    std::shared_ptr<ObjectDetector> build() const {
        std::shared_ptr<ObjectDetector> detector(
            new HaarDetector(this->className, this->file, haar_scale, haar_min_overlap)
        );

        if (this->children.size() > 0) {
            auto parentDetector = new HierarchicalObjectDetector(detector, this->minChildOccurences, this->maxChildOccurences);

            std::for_each(this->children.begin(), this->children.end(), [&] (const std::pair<std::string, HaarModel> &pair) {
                parentDetector->addChildDetector(
                    pair.second.build(),
                    pair.second.minOccurences,
                    pair.second.maxOccurences
                );
            });
            return std::shared_ptr<ObjectDetector>(parentDetector);
        }
        return detector;
    }


    static bool parse(const std::string &value) {
        std::stringstream stream(value);
        std::vector<std::string> items;

        for (std::string item; std::getline(stream, item, ':'); ) {
            items.push_back(item);
        }
        switch (items.size()) {
        case 1:
            haar_models["any"].className = "any";
            haar_models["any"].file = items[0];
            return true;
        case 2:
            haar_models[items[0]].className = items[0];
            haar_models[items[0]].file = items[1];
            return true;
        case 3:
            haar_models[items[2]].children[items[0]].className = items[0];
            haar_models[items[2]].children[items[0]].file = items[1];
            return true;
        case 4:
            haar_models[items[0]].className = items[0];
            haar_models[items[0]].file = items[1];
            haar_models[items[0]].minChildOccurences = atoi(items[2].c_str());
            haar_models[items[0]].maxChildOccurences = atoi(items[3].c_str());
            return true;
        case 5:
            haar_models[items[2]].children[items[0]].className = items[0];
            haar_models[items[2]].children[items[0]].file = items[1];
            haar_models[items[2]].children[items[0]].minOccurences = atoi(items[3].c_str());
            haar_models[items[2]].children[items[0]].maxOccurences = atoi(items[4].c_str());
            return true;
        }
        return false;
    }
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-detect --algorithm algo input-image.tiff output-objects.yml\n\n");

    printf("Detects objects within input image. Detected objects are written to a yml file.\n\n");

    printf("General options:\n\n");
    printf("--algorithm algo : algorithm to use for object detection ('haar')\n");
    printf("--export-path folder/ : set exported object image path\n");
    printf("--export-format png : set exported object image format ('png', 'jpeg', 'tiff')\n");
    printf("\n");

    printf("Gnomonic projection options:\n\n");
    printf("--gnomonic : activate task\n");
    printf("--gnomonic-width 2048 : projection window width\n");
    printf("--gnomonic-aperture-x 60 : horizontal projection aperture\n");
    printf("--gnomonic-aperture-y 60 : vertical projection aperture\n");
    printf("\n");

    printf("Haar-cascades options:\n\n");
    printf("--haar-model class:file.xml[:minChild:maxChild] : parent haar model file with class name (allowed multiple times)\n");
    printf("--haar-model class:file.xml:parentclass[:min:max] : child haar model file with class name (allowed multiple times)\n");
    printf("--haar-scale 1.1 : haar reduction scale factor\n");
    printf("--haar-min-overlap 3 : haar minimum detection overlap\n");
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

            for (auto it = haar_models.begin(); it != haar_models.end(); ++it) {
                if (!(*it).second.check()) {
                    return 2;
                }
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

        case OPTION_EXPORT_FORMAT:
            if (strcmp(optarg, "none") == 0) {
                export_format = FORMAT_NONE;
            } else if (strcmp(optarg, "png") == 0) {
                export_format = FORMAT_PNG;
            } else if (strcmp(optarg, "jpeg") == 0) {
                export_format = FORMAT_JPEG;
            } else if (strcmp(optarg, "tiff") == 0) {
                export_format = FORMAT_TIFF;
            } else {
                fprintf(stderr, "Error: unsupported export format: %s\n", optarg);
            }
            break;

        case OPTION_EXPORT_PATH:
            export_path = optarg;
            break;

        case OPTION_GNOMONIC:
            break;

        case OPTION_GNOMONIC_WIDTH:
            gnomonic_width = atoi(optarg);
            break;

        case OPTION_GNOMONIC_APERTURE_X:
            gnomonic_aperture_x = atof(optarg) / 180.0 * M_PI;
            break;

        case OPTION_GNOMONIC_APERTURE_Y:
            gnomonic_aperture_y = atof(optarg) / 180.0 * M_PI;
            break;

        case OPTION_HAAR_MODEL:
            if (!HaarModel::parse(optarg)) {
                fprintf(stderr, "Error: invalid haar model given: %s\n", optarg);
                return 2;
            }
            break;

        case OPTION_HAAR_SCALE:
            haar_scale = atof(optarg);
            break;

        case OPTION_HAAR_MIN_OVERLAP:
            haar_min_overlap = atoi(optarg);
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

    // instantiate detector(s)
    std::shared_ptr<ObjectDetector> detector;

    switch (algorithm) {
    case ALGORITHM_NONE:
        detector.reset(new ObjectDetector());
        break;

    case ALGORITHM_HAAR:
        {
            auto multiDetector = new MultiObjectDetector();

            std::for_each(haar_models.begin(), haar_models.end(), [&] (const std::pair<std::string, HaarModel> &pair) {
                multiDetector->addDetector(pair.second.build());
            });
            detector.reset(multiDetector);
        }
        break;

    default:
        fprintf(stderr, "Error: no detector instantiated!\n");
        return 3;
    }

    // setup gnomonic reprojection task
    if (gnomonic_enabled) {
        detector.reset(
            new GnomonicProjectionDetector(detector, gnomonic_width, gnomonic_aperture_x, gnomonic_aperture_y)
        );
    }

    // set export of detected objects in native projection
    if (export_format != FORMAT_NONE && export_path != NULL) {
        // create export folder
        std::stringstream stream(export_path);
        std::string path;

        for (std::string item; std::getline(stream, item, '/'); ) {
            if (path.length() > 0) {
                path.append("/");
            }
            path.append(item);
            mkdir(path.c_str(), 0755);
        }

        // enable detected object export
        switch (export_format) {
        case FORMAT_PNG:
            detector->setObjectExport(export_path, ".png");
            break;
        case FORMAT_JPEG:
            detector->setObjectExport(export_path, ".jpeg");
            break;
        case FORMAT_TIFF:
            detector->setObjectExport(export_path, ".tiff");
            break;
        default:
            fprintf(stderr, "Error: unsupported export image format!\n");
            return 3;
        }
    }

    // detect objects in source image
    bool success = false;

    if (detector) {
        // run detection algorithm
        std::list<DetectedObject> objects;

        if (source.channels() == 1 || detector->supportsColor()) {
            success = detector->detect(source, objects);
            source.release();
        } else {
            cv::Mat graySource;

            cv::cvtColor(source, graySource, cv::COLOR_RGB2GRAY);
            // cv::equalizeHist(graySource, graySource);
            source.release();

            success = detector->detect(graySource, objects);
        }

        // save detected objects
        cv::FileStorage fs(objects_file, cv::FileStorage::WRITE);

        switch (algorithm) {
        case ALGORITHM_NONE:
            fs << "algorithm" << "none";
            break;

        case ALGORITHM_HAAR:
            {
                fs << "algorithm" << "haar";
                fs << "haar" << "{";
                    fs << "models" << "[";
                    std::for_each(haar_models.begin(), haar_models.end(), [&] (const std::pair<std::string, HaarModel> &pair) {
                        pair.second.write(fs);
                    });
                    fs << "]";
                    fs << "scale" << haar_scale;
                    fs << "min_overlap" << haar_min_overlap;
                fs << "}";
            }
            break;
        }
        if (gnomonic_enabled) {
            fs << "gnomonic" << "{" << "width" << gnomonic_width << "aperture_x" << gnomonic_aperture_x << "aperture_y" << gnomonic_aperture_y << "}";
        }
        if (export_format != FORMAT_NONE && export_path != NULL) {
            fs << "export" << "{" << "format" << export_format << "path" << export_path << "}";
        }
        fs << "source" << source_file;
        fs << "objects" << "[";
        std::for_each(objects.begin(), objects.end(), [&] (const DetectedObject &object) {
            object.write(fs);
        });
        fs << "]";
    }
    return success ? 0 : 4;
}
