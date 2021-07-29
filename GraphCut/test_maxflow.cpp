
#include <iostream>
#include "ImageStitching.h"
#include <opencv2/core/utils/filesystem.hpp>
#include <filesystem>
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
    std::function<void(stitchingArgs)> stitchingAlgorithm
) {    
    cv::utils::fs::createDirectory(output);

    std::vector<std::string> dataFolders;
    for (const auto& entry : fs::directory_iterator(input)) {
        std::string path = entry.path().string();
        if (std::filesystem::is_directory(path)) {
            dataFolders.push_back(path);
        }
    }

    for (auto dataFoler : dataFolders) {
        
        Mat sourceImg, sinkImg;
        loadImageFromFolder(dataFoler, sourceImg, sinkImg);

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

        stitchingAlgorithm(sourceImg, sinkImg, OUTPUT_FOLDER);
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
    runBatchTest("./input", "./BatchTestResult", 
        [](stitchingArgs) {
            ImageStitching::RGBDiffenceStitching(soureImg, sinkImg, OUTPUT_FOLDER);
    });

    runSingleTest("./input", "./result",
        [](stitchingArgs) {
        ImageStitching::RGBDiffenceStitching(soureImg, sinkImg, OUTPUT_FOLDER);
    });
}

