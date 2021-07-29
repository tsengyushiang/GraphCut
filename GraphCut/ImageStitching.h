#pragma once
#include "maxflow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>

#define stitchingArgs \
Mat& soureImg,\
Mat& sinkImg,\
std::function<std::string(std::string)>OUTPUT_FOLDER

using namespace std;
using namespace cv;
using namespace std::chrono;

using maxflow::Graph_III;

typedef struct StitchingResult {
    Mat nodeType;
    Mat label;
    Mat image;
} StitchingResult;

class ImageStitching {
public:
    static void example();

    static StitchingResult ImageStitching_kernel(Mat& soureImg, Mat& sinkImg,
        std::function<bool(int y, int x)> isROI,
        std::function<bool(int y, int x)> isSourceConstrain,
        std::function<bool(int y, int x)> isSinkConstrain,
        std::function<int(Point2d s, Point2d t)> edgeEnergy);

    static void RGBDiffenceStitching(stitchingArgs);
};