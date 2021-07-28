
#include <iostream>
#include "maxflow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>


using namespace std;
using namespace cv;
using namespace std::chrono;
using maxflow::Graph_III; 

void ImageStitching(Mat soureImg, Mat sinkImg,
    std::function<bool(Mat& img1,Mat& img2, int y, int x)> isROI,
    std::function<bool(Mat& img1,Mat& img2, int y, int x)> isSourceConstrain,
    std::function<bool(Mat& img1,Mat& img2, int y, int x)> isSinkConstrain,
    std::function<int(Point2d s, Point2d t, Mat& img1, Mat& img2)> edgeEnergy
) {

    int w = soureImg.size().width;
    int h = soureImg.size().height;
    
    if (w != sinkImg.size().width || h != sinkImg.size().height) {
        printf("Input Images Resolution Not Matched");
        return;          
    }

    std::map<int, int> pixelIndex2nodeInex;

    // calculate node count
    int nodes_count = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            if (isROI(soureImg, sinkImg, i, j)) {
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
            if (isROI(soureImg, sinkImg, i, j)) {
                G.add_node();
                Point2d p_node(i, j);
                int nodeIndex = pixelIndex2nodeInex.find(index)->second;

                // sink source edge
                if (isSourceConstrain(soureImg,sinkImg,i,j)) {
                    G.add_tweights(nodeIndex, INT_MAX, 0);
                }
                else if (isSinkConstrain(soureImg, sinkImg, i, j)) {
                    G.add_tweights(nodeIndex, 0, INT_MAX);
                }

                // left edge weight
                int leftIndex = i * w + (j - 1);
                Point2d p_left(i,j-1);
                auto leftNodeMap = pixelIndex2nodeInex.find(leftIndex);
                if (leftNodeMap != pixelIndex2nodeInex.end()) {
                    int e = edgeEnergy(p_node,p_left,soureImg,sinkImg);
                    G.add_edge(nodeIndex, leftNodeMap->second, e, e);
                }

                // up edge weight
                int upIndex = (i - 1) * w + j;
                Point2d p_up(i-1,j);
                auto upNodeMap = pixelIndex2nodeInex.find(upIndex);
                if (upNodeMap != pixelIndex2nodeInex.end()) {
                    int e = edgeEnergy(p_node,p_up,soureImg,sinkImg);
                    G.add_edge(nodeIndex, upNodeMap->second, e, e);
                }


            }
        }
    }
    // solve min-cut
    int flow = G.maxflow();
    printf("Flow = %d\n", flow);

    // output Result label,image,(image with cut seam)
    Mat nodeType(h, w, CV_8UC3, Scalar(0, 0, 0));
    Mat label(h, w, CV_8UC3, Scalar(0, 0, 0));
    Mat image(h, w, CV_8UC4, Scalar(0, 0, 0, 0));
    Mat imageWithSeam(h, w, CV_8UC4, Scalar(0, 0, 0, 0));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int index = i * w + j;
            auto nodeMap = pixelIndex2nodeInex.find(index);
            if (nodeMap != pixelIndex2nodeInex.end()) {
                if (G.what_segment(nodeMap->second) == Graph_III::SOURCE) {
                    label.at < Vec3b >(i,j)= Vec3b(255, 0, 0);
                    image.at < Vec4b >(i, j) = soureImg.at < Vec4b >(i, j);
                }
                else if(G.what_segment(nodeMap->second) == Graph_III::SINK) {
                    label.at < Vec3b >(i, j) = Vec3b(0, 255, 0);
                    image.at < Vec4b >(i, j) = sinkImg.at < Vec4b >(i, j);
                }
                if (isSourceConstrain(soureImg, sinkImg, i, j)) {
                    nodeType.at < Vec3b >(i, j) = Vec3b(255, 0, 0);
                }
                else if (isSinkConstrain(soureImg, sinkImg, i, j)) {
                    nodeType.at < Vec3b >(i, j) = Vec3b(0, 255, 0);
                }
                else {
                    nodeType.at < Vec3b >(i, j) = Vec3b(255, 255, 255);
                }
            }
            

        }
    }

    imwrite("./result/label.png", label);
    imwrite("./result/stitchImage.png", image);
    imwrite("./result/nodeType.png", nodeType);
}

void Example() {
    /*
          SOURCE
        /       \
    INF/         \INF
      /      0    \
    node0 -----> node1
     ↑|   <-----  ↑|
    X||X    0    5||5
     |↓     1     |↓   
    node2 -----> node3
     ↑|   <-----  ↑|
    4||4    1    6||6
     |↓     2     |↓
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
            printf("node%d is in the SOURCE set\n",i);
        else
            printf("node%d is in the SINK set\n",i);
    }

    delete g;

}

int main()
{
    Mat soureImg = imread("./input/0.png", CV_LOAD_IMAGE_UNCHANGED);
    Mat sinkImg = imread("./input/1.png", CV_LOAD_IMAGE_UNCHANGED);

    auto isROI = [](Mat& soureImg,Mat& sinkImg,int y,int x) {
        return soureImg.at<Vec4b>(y, x)[3] || sinkImg.at<Vec4b>(y, x)[3];
    };

    auto isSourceConstrain = [](Mat& source, Mat& sink, int y, int x) {
        return (source.at<Vec4b>(y, x)[3] != 0 && sink.at<Vec4b>(y, x)[3] == 0);
    };

    auto isSinkConstrain = [](Mat& source, Mat& sink, int y, int x) {
        return (sink.at<Vec4b>(y, x)[3] != 0 && source.at<Vec4b>(y, x)[3] == 0);
    };

    auto colorCost = [](Point2d s, Point2d t, Mat img1, Mat img2) {

        Vec4b& colorA_s = img1.at<Vec4b>(s.x, s.y);
        Vec4b& colorB_s = img2.at<Vec4b>(s.x, s.y);

        Vec4b& colorA_t = img1.at<Vec4b>(t.x, t.y);
        Vec4b& colorB_t = img2.at<Vec4b>(t.x, t.y);

        int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
        int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
        int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);

        int energy = (cr + cg + cb) / 3;
        return energy;
    };

    auto start = high_resolution_clock::now();

    ImageStitching(soureImg, sinkImg, isROI, isSourceConstrain, isSinkConstrain, colorCost);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    auto secs = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
    cout << "\nExecution time: " << secs.count() << "s" << endl;
}

