#include "ImageStitching.h"

StitchingResult ImageStitching::ImageStitching_kernel(Mat& soureImg, Mat& sinkImg,
    std::function<bool(int y, int x)> isROI,
    std::function<bool(int y, int x)> isSourceConstrain,
    std::function<bool(int y, int x)> isSinkConstrain,
    std::function<int(Point2d s, Point2d t)> edgeEnergy,
    std::function<void(Point2d p, int& soureEnergy,int& sinkEnergy)> dataterm
) {

    int w = soureImg.size().width;
    int h = soureImg.size().height;

    if (w != sinkImg.size().width || h != sinkImg.size().height) {
        std::printf("Input Images Resolution Not Matched");
        return {};
    }

    std::map<int, int> pixelIndex2nodeInex;

    // calculate node count
    int nodes_count = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            if (isROI(i, j)) {
                pixelIndex2nodeInex[index] = nodes_count++;
            }
        }
    }

    //calculate edge count
    int edge_count = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            if (pixelIndex2nodeInex.find(index) != pixelIndex2nodeInex.end()) {
                int leftIndex = i * w + (j - 1);
                if (pixelIndex2nodeInex.find(leftIndex) != pixelIndex2nodeInex.end()) {
                    edge_count++;
                }
                int upIndex = (i - 1) * w + j;
                if (pixelIndex2nodeInex.find(upIndex) != pixelIndex2nodeInex.end()) {
                    edge_count++;
                }
            }
        }
    }

    //fill out graph
    Graph_III G = Graph_III(nodes_count, edge_count);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            if (isROI(i, j)) {
                G.add_node();
                Point2d p_node(i, j);
                int nodeIndex = pixelIndex2nodeInex.find(index)->second;

                // sink source edge
                if (isSourceConstrain(i, j)) {
                    G.add_tweights(nodeIndex, INT_MAX, 0);
                }
                else if (isSinkConstrain(i, j)) {
                    G.add_tweights(nodeIndex, 0, INT_MAX);
                }
                else {
                /*    int sourceWeight, sinkWeight = 0;
                    dataterm(p_node, sourceWeight, sinkWeight);
                    G.add_tweights(nodeIndex, sourceWeight, sinkWeight);*/
                }

                // left edge weight
                int leftIndex = i * w + (j - 1);
                Point2d p_left(i, j - 1);
                auto leftNodeMap = pixelIndex2nodeInex.find(leftIndex);
                if (leftNodeMap != pixelIndex2nodeInex.end()) {
                    int e = edgeEnergy(p_node, p_left);
                    G.add_edge(nodeIndex, leftNodeMap->second, e, e);
                }

                // up edge weight
                int upIndex = (i - 1) * w + j;
                Point2d p_up(i - 1, j);
                auto upNodeMap = pixelIndex2nodeInex.find(upIndex);
                if (upNodeMap != pixelIndex2nodeInex.end()) {
                    int e = edgeEnergy(p_node, p_up);
                    G.add_edge(nodeIndex, upNodeMap->second, e, e);
                }


            }
        }
    }
    // solve min-cut
    int flow = G.maxflow();
    std::printf("Flow = %d\n", flow);

    // output Result label,image,(image with cut seam)
    Mat nodeType(h, w, CV_8UC4, Scalar(0, 0, 0));
    Mat label(h, w, CV_8UC4, Scalar(0, 0, 0));
    Mat image(h, w, CV_8UC4, Scalar(0, 0, 0, 0));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            auto nodeMap = pixelIndex2nodeInex.find(index);
            if (nodeMap != pixelIndex2nodeInex.end()) {
                if (G.what_segment(nodeMap->second) == Graph_III::SOURCE) {
                    label.at < Vec4b >(i, j) = Vec4b(255, 0, 0, 127);
                    image.at < Vec4b >(i, j) = soureImg.at < Vec4b >(i, j);
                }
                else if (G.what_segment(nodeMap->second) == Graph_III::SINK) {
                    label.at < Vec4b >(i, j) = Vec4b(0, 255, 0, 127);
                    image.at < Vec4b >(i, j) = sinkImg.at < Vec4b >(i, j);
                }
                if (isSourceConstrain(i, j)) {
                    nodeType.at < Vec4b >(i, j) = Vec4b(255, 0, 0, 127);
                }
                else if (isSinkConstrain(i, j)) {
                    nodeType.at < Vec4b >(i, j) = Vec4b(0, 255, 0, 127);
                }
                else {
                    nodeType.at < Vec4b >(i, j) = Vec4b(0, 0, 255, 127);
                }
            }


        }
    }

    //
    cv::flip(nodeType, nodeType, -1);
    Mat allInone(h*2, w*2, CV_8UC4, Scalar(0, 0, 0));
    cv::flip(image, image, -1);
    cv::flip(label, label, -1);
    cv::flip(soureImg, soureImg, -1);
    cv::flip(sinkImg, sinkImg, -1);

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;

            Vec4b bakcgounrd= Vec4b(129, 245, 66, 255);
            if (image.at < Vec4b >(i, j)[3]==0) {
                image.at < Vec4b >(i, j) = bakcgounrd;
            }
            if (soureImg.at < Vec4b >(i, j)[3] == 0) {
                soureImg.at < Vec4b >(i, j) = bakcgounrd;
            }
            if (sinkImg.at < Vec4b >(i, j)[3] == 0) {
                sinkImg.at < Vec4b >(i, j) = bakcgounrd;
            }
        }
    }

    // Copy images in correct position
    soureImg.copyTo(allInone(Rect(0, 0, w, h)));
    sinkImg.copyTo(allInone(Rect(w, 0, w, h)));
    image.copyTo(allInone(Rect(w, h, w,h)));
    label.copyTo(allInone(Rect(0, h, w, h)));

    return {
        nodeType,
        label,
        image,
        allInone
    };
}

Mat ImageStitching::RGBDiffenceStitching(stitchingArgs) {

    auto isROI = [&soureImg, &sinkImg](int y, int x) {
        return soureImg.at<Vec4b>(y, x)[3] || sinkImg.at<Vec4b>(y, x)[3];
    };

    auto isSourceConstrain = [&soureImg, &sinkImg](int y, int x) {
        return (soureImg.at<Vec4b>(y, x)[3] != 0 && sinkImg.at<Vec4b>(y, x)[3] == 0);
    };

    auto isSinkConstrain = [&soureImg, &sinkImg](int y, int x) {
        return (sinkImg.at<Vec4b>(y, x)[3] != 0 && soureImg.at<Vec4b>(y, x)[3] == 0);
    };

    auto colorCost = [&soureImg, &sinkImg](Point2d s, Point2d t) {

        Vec4b& colorA_s = soureImg.at<Vec4b>(s.x, s.y);
        Vec4b& colorB_s = sinkImg.at<Vec4b>(s.x, s.y);

        Vec4b& colorA_t = soureImg.at<Vec4b>(t.x, t.y);
        Vec4b& colorB_t = sinkImg.at<Vec4b>(t.x, t.y);

        int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
        int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
        int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);

        int energy = (cr + cg + cb) / 3;
        return energy;
    };

    auto dataterm = [&soureImg, &sinkImg](Point2d s, int& sourceWeight,int& sinkWeight) {

        Vec4b& colorSource = soureImg.at<Vec4b>(s.x, s.y);
        Vec4b& colorSink = sinkImg.at<Vec4b>(s.x, s.y);

        sourceWeight = -colorSource[0]*0.1;
        sinkWeight = -colorSink[0]*0.1;
    };

    auto start = high_resolution_clock::now();

    auto result = ImageStitching_kernel(soureImg, sinkImg, isROI, isSourceConstrain, isSinkConstrain, colorCost, dataterm);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    auto secs = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
    cout << "\nExecution time: " << secs.count() << "s" << endl;

    cv::imwrite(OUTPUT_FOLDER("label.png"), result.label);
    cv::imwrite(OUTPUT_FOLDER("sink.png"), sinkImg);
    cv::imwrite(OUTPUT_FOLDER("source.png"), soureImg);
    cv::imwrite(OUTPUT_FOLDER("stitchImage.png"), result.image);
    cv::imwrite(OUTPUT_FOLDER("nodeType.png"), result.nodeType);
    cv::imwrite(OUTPUT_FOLDER("allInOne.png"), result.allInOne);
    cvtColor(result.allInOne, result.allInOne, CV_BGRA2RGBA);

    return result.allInOne;
}

void ImageStitching::example() {
    /*
          SOURCE
        /       \
    INF/         \INF
      /      0    \
    node0 -----> node1
     ¡ô|   <-----  ¡ô|
    X||X    0    5||5
     |¡õ     1     |¡õ
    node2 -----> node3
     ¡ô|   <-----  ¡ô|
    4||4    1    6||6
     |¡õ     2     |¡õ
    node4 -----> node5
     |   <-----   |
     |      2     |
     \            /
   INF\          /INF
       \        /
          SINK
    */
    int X = 3;
    X = 9; // uncommt this to see differnet result
    Graph_III* g = new Graph_III(/*estimated # of nodes*/ 2, /*estimated # of edges*/ 1);

    g->add_node(); // node0
    g->add_node(); // node1
    g->add_node(); // node2
    g->add_node(); // node3
    g->add_node(); // node4
    g->add_node(); // node5

    g->add_tweights(0,   /* capacities */  INT_MAX, 0);
    g->add_tweights(1,   /* capacities */  INT_MAX, 0);
    g->add_tweights(4,   /* capacities */  0, INT_MAX);
    g->add_tweights(5,   /* capacities */  0, INT_MAX);

    g->add_edge(0, 1,    /* capacities */  0, 0);
    g->add_edge(0, 2,    /* capacities */  X, X);
    g->add_edge(2, 4,    /* capacities */  4, 4);
    g->add_edge(1, 3,    /* capacities */  5, 5);
    g->add_edge(3, 5,    /* capacities */  6, 6);
    g->add_edge(2, 3,    /* capacities */  1, 1);
    g->add_edge(4, 5,    /* capacities */  2, 2);

    int flow = g->maxflow();

    printf("Flow = %d\n", flow);
    printf("Minimum cut:\n");

    for (int i = 0; i < 6; i++) {
        if (g->what_segment(i) == Graph_III::SOURCE)
            printf("node%d is in the SOURCE set\n", i);
        else
            printf("node%d is in the SINK set\n", i);
    }

    delete g;

}