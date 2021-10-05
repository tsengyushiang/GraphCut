
#include <iostream>
#include "ImageStitching.h"
#include <opencv2/core/utils/filesystem.hpp>
#include <filesystem>
#include <opencv2/highgui/highgui.hpp>  // Video write

namespace fs = std::filesystem;

void loadImageFromFolder(std::string input, Mat& soureImg, Mat& sinkImg) {
    std::vector<std::string> imgs;
    for (const auto& entry : fs::directory_iterator(input)) {
        if (entry.path().extension() == ".png") {
            imgs.push_back(entry.path().string());
        }
    }

    if (imgs.size() != 2) {
        std::cout << input << " images number is not two." << std::endl;
        return;
    }
    else {
        std::cout << "Stitch Image :" << std::endl;
        std::cout << imgs[0] << std::endl;
        std::cout << imgs[1] << std::endl;
    }

    soureImg = imread(imgs[0], CV_LOAD_IMAGE_UNCHANGED);
    sinkImg = imread(imgs[1], CV_LOAD_IMAGE_UNCHANGED);
}

/*
    I/O struture
    --- input
    |---data1
    | |---0.png
    | |---1.png
    |---data2
    | |---0.png
    | |---1.png
    --- result(create by program)
    |---data1
    | |---label.png
    | |---nodeType.png
    | |---stitchImage.png
    |---data2...
    */
void runBatchTest(
    std::string input,
    std::string output,
    std::function<Mat(stitchingArgs)> stitchingAlgorithm,
    bool exportVideo,int fps
) {    
    cv::utils::fs::createDirectory(output);

    std::map<std::string, std::string> dataFolders;
    for (const auto& entry : fs::directory_iterator(input)) {
        std::string path = entry.path().string();
        if (std::filesystem::is_directory(path)) {
            dataFolders[entry.path().filename().string()] = path;
        }
    }

    VideoWriter* oVideoWriter = nullptr;
    int w, h;
    for (auto keyAndValue : dataFolders) {
        auto dataFoler = keyAndValue.second;

        Mat sourceImg, sinkImg;
        loadImageFromFolder(dataFoler, sourceImg, sinkImg);

        if (exportVideo && oVideoWriter == nullptr) {
            w = sourceImg.size().width;
            h = sourceImg.size().height;
            Size frame_size(w * 2, h * 2);
            oVideoWriter = new  VideoWriter(cv::utils::fs::join(output, "output.avi"), VideoWriter::fourcc('M', 'J', 'P', 'G'),
                fps, frame_size, true);
        }

        auto eraseSubStr = [](std::string& mainStr, const std::string& toErase)
        {
            // Search for the substring in string
            size_t pos = mainStr.find(toErase);
            if (pos != std::string::npos)
            {
                // If found then erase it from string
                mainStr.erase(pos, toErase.length());
            }
        };
        eraseSubStr(dataFoler, input);
        std::string outputDataFolder = cv::utils::fs::join(output, dataFoler);
        cv::utils::fs::createDirectory(outputDataFolder);
        
        auto OUTPUT_FOLDER = [&outputDataFolder](std::string x) {
            return cv::utils::fs::join(outputDataFolder, x);
        };

        Mat allIneOneResult = stitchingAlgorithm(sourceImg, sinkImg, OUTPUT_FOLDER);
        if (oVideoWriter != nullptr) {
            cv::putText(allIneOneResult, //target image
                keyAndValue.first, //text
                cv::Point(w,h), //top-left position
                cv::FONT_HERSHEY_PLAIN,
                15.0,
                CV_RGB(118, 185, 0), //font color
                2);
            oVideoWriter->write(allIneOneResult);
        }
    }   
    if (oVideoWriter != nullptr) {
        delete oVideoWriter;
    }
}

void runSingleTest(
    std::string input,
    std::string output,
    std::function<void(stitchingArgs)> stitchingAlgorithm
) {
    Mat sourceImg, sinkImg;
    loadImageFromFolder(input, sourceImg, sinkImg);

    cv::utils::fs::createDirectory(output);
    auto OUTPUT_FOLDER = [&output](std::string x) {
        return cv::utils::fs::join(output, x);
    };

    stitchingAlgorithm(sourceImg, sinkImg, OUTPUT_FOLDER);
};

void main()
{
    runBatchTest("./2021-10-05-11-56-39-warppingResult-fps30-duration3", "./BatchTestResult_2021-10-05-11-56-39-warppingResult-fps30-duration3", 
        [](stitchingArgs) {
            return ImageStitching::RGBDiffenceStitching(soureImg, sinkImg, OUTPUT_FOLDER);
    },true,30);

    //runSingleTest("./input", "./result",
    //    [](stitchingArgs) {
    //    ImageStitching::RGBDiffenceStitching(soureImg, sinkImg, OUTPUT_FOLDER);
    //});
}

