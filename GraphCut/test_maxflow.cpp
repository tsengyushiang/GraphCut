
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

enum Direction {
    NONE = 0,
    LEFT,
    RIGHT,
    UP,
    BOTTOM,
};

//find node index
int nodeIndex(int x,int y,int width,int height, enum Direction dir) {

    if (dir == Direction::NONE) {
        return x * height + y;
    }
    else if (dir == Direction::LEFT) {
        if (x - 1 < 0)
            return (x - 1) * height + y;
        else
            return -1;
    }
    else if(dir == Direction::RIGHT){
        if (x + 1 < width)
            return (x + 1) * height + y;
        else
            return -1;
    }  
    else if (dir == Direction::UP) {
        if (y - 1 < 0) 
            return x * height + y - 1;
        else
            return -1;       
    }
    else if (dir == Direction::BOTTOM) {
        if (y + 1 < height)
            return x * height + y + 1;
        else
            return -1;
    }
    return -1;
}

//compute Data term{
int dataTerm(Point2d point,Mat img1,Mat img2){
    Vec4b& color1 = img1.at<Vec4b>(point.y, point.x);
    Vec4b& color2 = img2.at<Vec4b>(point.y, point.x);
    
    int alpha1 = color1[3];
    int alpha2 = color2[3];
    int img_index = 0;
    int cost = 0;

    if (alpha1 == 255 && alpha2 == 0) {
        img_index = 0;     
    }
    else if (alpha1 == 0 && alpha2 == 255) {
        img_index = 1;
    }
    else if (alpha1 == 0 && alpha2 == 0) {
        img_index = 2;
    }
    else {
        img_index = 3;
    }
    
    return img_index;
}

//compute energy term (Basic Color Difference)
int colorCost(Point2d s, Point2d t,Mat img1,Mat img2) {
 
    Vec4b& colorA_s = img1.at<Vec4b>(s.y, s.x);
    Vec4b& colorB_s = img2.at<Vec4b>(s.y, s.x);

    Vec4b& colorA_t = img1.at<Vec4b>(t.y, t.x);
    Vec4b& colorB_t = img2.at<Vec4b>(t.y, t.x);

    int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
    int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
    int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);;


    int energy = (cr + cg + cb)/3 ;
    return energy;
}

//check overlap region
vector<vector<int>> findOverlap(Mat imgL,Mat imgR) {

    int h = imgL.rows;
    int w = imgR.cols;
    vector<vector<int>> overlap_region(h, vector<int>(w,0));

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (imgL.at<Vec4b>(y, x)[3] != 0)
                overlap_region[y][x] += 1;
            if (imgR.at<Vec4b>(y, x)[3] != 0)
                overlap_region[y][x] += 2;           
        }
    }
    
    Mat mask_L(h, w, CV_8UC1, Scalar(0, 0, 0));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (overlap_region[y][x] == 1)
                mask_L.at<uchar>(y, x) = 255;
        }
    }
    imwrite("./result/debug_L.jpg",mask_L);

    return overlap_region;
}

tuple<bool, int>  checkIsBoundary(vector<vector<int>>tag, int x, int y) {
    
    //check the node is boundary node
    int h = tag.size();
    int w = tag[0].size();

    bool check;
    int img_index=0;

    if (x - 1 < 0 || x + 1 >= w) {
        cout << "here" << endl;
        check = true;
        if (x - 1 < 0)
            img_index = 1;
        else if (x + 1 >= w)
            img_index = 2;    
    }
    else if(tag[y][x - 1]!=3 || tag[y][x + 1] != 3 ){
        check = true;
        if (tag[y][x - 1] != 3)
            img_index = tag[y][x - 1];
        else if (tag[y][x + 1] != 3)
            img_index = tag[y][x + 1];

        //cout << "true" << " " << img_index << endl;
       
    }
    else {
        check = false;
        img_index = 0;
    }
    return { check, img_index };
}


int main()
{
    auto start = high_resolution_clock::now();
    //img1 img2 position
    Point2f pos_img1(0, 5);
    Point2f pos_img2(186, 0);
  

    //read img
    Mat imgL = imread("./input/0.png", IMREAD_UNCHANGED);
    Mat imgR = imread("./input/1.png", IMREAD_UNCHANGED);

    cout << "img1 path: ./0.png" << endl;
    cout << "img2 path: ./1.png" << endl;

    cout << "img1 position offset: 0 5"<<endl;
    cout << "img2 position offset: 186 0" << endl;
   

    int panorama_width = max(imgL.cols + pos_img1.x, imgR.cols + pos_img2.x);
    int panorama_height = max(imgL.rows + pos_img1.y, imgR.rows + pos_img2.y);
    cout << "panorama size: " << panorama_width << " " << panorama_height << endl;

    int minx = pos_img2.x;
    int overlap_width = imgL.cols + pos_img1.x - minx;

    int miny = 0;
    int overlap_height = panorama_height;
    cout << "overlap_size : " << overlap_width << " " << overlap_height << endl;

    Mat imgL_panorama(panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));
    Mat imgR_panorama(panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));

    //translate image
    imgL(Rect(0, 0, imgL.cols, imgL.rows)).copyTo(imgL_panorama(Rect(pos_img1.x, pos_img1.y, imgL.cols, imgL.rows)));
    imgR(Rect(0, 0, imgR.cols, imgR.rows)).copyTo(imgR_panorama(Rect(pos_img2.x, pos_img2.y, imgR.cols, imgR.rows)));

    //crop overlap region
    Mat imgL_overlap, imgR_overlap;
    Rect myROI(minx, 0, overlap_width, overlap_height);
    imgL_overlap = imgL_panorama(myROI);
    imgR_overlap = imgR_panorama(myROI);


    //imwrite("./result/imgL_overlap.png", imgL_overlap);
    //imwrite("./result/imgR_overlap.png", imgR_overlap);


    //find overlap region
    vector<vector<int>> overlap_region_tag = findOverlap(imgL_overlap, imgR_overlap);

    Mat overlap_mask(overlap_height, overlap_width, CV_8UC1, Scalar(0, 0, 0));
    for (int y = 0; y < overlap_height; y++) {
        for (int x = 0; x < overlap_width; x++) {
            if (overlap_region_tag[y][x] == 3)
                overlap_mask.at<uchar>(y, x) = 255;
        }
    }

        
    vector<vector<Point> > contours;
    findContours(overlap_mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //找轮廓
    
    Mat boundaryMask(overlap_mask.size(), CV_8U, Scalar(0));
    drawContours(boundaryMask, contours, -1, Scalar(255), 1);
    //imwrite("./result/overlap_mask.png", boundaryMask);
  
    int nodes_num = overlap_width * overlap_height;
    int edges_num = (overlap_height - 1) * overlap_width + (overlap_width - 1) * overlap_height;
  
    cout << "Graph nodes: " << nodes_num << endl;
    cout << "Graph edges: " << edges_num << endl;

    //Graphcut
    Graph_III G = Graph_III(nodes_num, edges_num);
    map<string, int> nodemap;
 
    int nodeIndex = 0;
    for (int y = 0; y < overlap_height; y++) {
        for (int x = 0; x < overlap_width; x++) {
            if (overlap_region_tag[y][x] == 3) {
                string key = to_string(y)+to_string(x);
                nodemap.insert(pair<string, int>(key, nodeIndex));       
                G.add_node();
                //Check boundary
                int boundary = boundaryMask.at<uchar>(y, x);
                
                if (boundary == 255) {
                    if (x < overlap_width / 2)
                        G.add_tweights(nodeIndex, INT_MAX, 0);
                    else
                        G.add_tweights(nodeIndex, 0, INT_MAX);                 
                }
                
                //check left up node
                string leftkey = to_string(y) + to_string(x-1);  
                string upkey = to_string(y-1) + to_string(x);
                map<string, int>::iterator it;

                Point2d p_node(x, y);
                Point2d p_left(x - 1, y);
                Point2d p_up(x, y - 1);

                it = nodemap.find(leftkey);

                if (it != nodemap.end()) {
                    float cost = colorCost(p_node, p_left, imgL_overlap, imgR_overlap);
                    G.add_edge(nodeIndex, it->second, cost, cost);
                }
                
                it = nodemap.find(upkey);
                if (it != nodemap.end()) {
                    float cost = colorCost(p_node, p_up, imgL_overlap, imgR_overlap);
                    G.add_edge(nodeIndex, it->second, cost, cost);
                }
                nodeIndex++;
            }
        }
    }
    cout << "end" << endl;
    G.maxflow();


    Mat mask(overlap_height, overlap_width, CV_8UC1, Scalar(0, 0, 0));

    for (int y = 0; y < overlap_height; y++) {
        for (int x = 0; x < overlap_width; x++) {
            if (overlap_region_tag[y][x] == 3) {
                string key = to_string(y) + to_string(x);
                map<string, int>::iterator it;          
                it = nodemap.find(key);
                if (it != nodemap.end()) {
                    if (G.what_segment(it->second) == Graph_III::SOURCE)                
                        mask.at<uchar>(y, x) = 255;
                }
                else 
                    cout << "error!" << endl;              
            }              
        }
    }


    Mat element = getStructuringElement(MORPH_RECT, Size(5,5));
    //dilate(mask, mask, element);
    //erode(mask, mask, element);


    imwrite("./result/mask.png", mask);
    Mat overlap_result(overlap_height, overlap_width, CV_8UC4, Scalar(0, 0, 0));
    Mat overlap_result_seam(overlap_height, overlap_width, CV_8UC4, Scalar(0, 0, 0));
    Mat colorMap(overlap_height, overlap_width, CV_8UC3, Scalar(0, 0, 0));
    for (int y = 0; y < overlap_height; y++) {
        for (int x = 0; x < overlap_width; x++) {
            if (overlap_region_tag[y][x] == 1)
                overlap_result.at<Vec4b>(y, x) = imgL_overlap.at<Vec4b>(y, x);
            else if (overlap_region_tag[y][x] == 2)
                overlap_result.at<Vec4b>(y, x) = imgR_overlap.at<Vec4b>(y, x);
            else if (overlap_region_tag[y][x] == 3) {
                if (mask.at<uchar>(y, x) == 255) {
                    overlap_result.at<Vec4b>(y, x) = imgL_overlap.at<Vec4b>(y, x);
                    colorMap.at<Vec3b>(y, x) = Vec3b(255, 0, 0);
                }
                else {
                    overlap_result.at<Vec4b>(y, x) = imgR_overlap.at<Vec4b>(y, x);
                    colorMap.at<Vec3b>(y, x) = Vec3b(0, 255, 0);
                };
            }
        }
    }
    overlap_result.copyTo(overlap_result_seam);
    int blend_width = 0;

    //blending
    for (int y = 0; y < overlap_height; y++) {
        for (int x = 0; x < overlap_width; x++) {
            if (overlap_region_tag[y][x] == 3) {
                int green = colorMap.at<Vec3b>(y, x)[1];
                int blue_L = colorMap.at<Vec3b>(y, x - 1)[0];
                int blue_U = colorMap.at<Vec3b>(y - 1, x)[0];
                int blue_R = colorMap.at<Vec3b>(y, x + 1)[0];
                int blue_D = colorMap.at<Vec3b>(y + 1, x)[0];


                if (green==255&&blue_L==255||green==255&&blue_U==255|| green == 255 && blue_R == 255 || green == 255 && blue_D == 255) {
                    overlap_result_seam.at<Vec4b>(y, x) = Vec4b(0,0,255,255);
                    Vec4b colorL = imgL_overlap.at<Vec4b>(y, x);
                    Vec4b colorR = imgR_overlap.at<Vec4b>(y, x);

                    overlap_result.at<Vec4b>(y, x)[0] = colorL[0] * 0.5 + colorR[0] * 0.5;
                    overlap_result.at<Vec4b>(y, x)[1] = colorL[1] * 0.5 + colorR[1] * 0.5;
                    overlap_result.at<Vec4b>(y, x)[2] = colorL[2] * 0.5 + colorR[2] * 0.5;

                    //Linear Blending
                    for (int i = 0; i < blend_width; i++) {

                        int pos_x = x + 1 - blend_width / 2 + i;
                        if (pos_x < 0 || pos_x >= overlap_width) {
                            continue;
                        }
                        else {

                            float alpha = 1 - (i / (float)(blend_width));
                            float beta = 1 - alpha;

                            colorL = imgL_overlap.at<Vec4b>(y, pos_x);
                            colorR = imgR_overlap.at<Vec4b>(y, pos_x);

                            overlap_result.at<Vec4b>(y, pos_x)[0] = colorL[0] * alpha + colorR[0] * beta;
                            overlap_result.at<Vec4b>(y, pos_x)[1] = colorL[1] * alpha + colorR[1] * beta;
                            overlap_result.at<Vec4b>(y, pos_x)[2] = colorL[2] * alpha + colorR[2] * beta;

                            if (colorL[3] == 0 && colorR[3] == 0) {
                                overlap_result.at<Vec4b>(y, pos_x)[3] = 0;
                            }
                            else {
                                overlap_result.at<Vec4b>(y, pos_x)[3] = 255;
                            }
                        }

                    }
                }
            }
        }
    }
 
 

    //graph cut result image
    Mat panorama (panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));
    Mat panorama_seam (panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));

    //Panorama
    imgL(Rect(0, 0, imgL.cols - overlap_width, imgL.rows)).copyTo(panorama(Rect(pos_img1.x, pos_img1.y, imgL.cols - overlap_width, imgL.rows)));
    imgR(Rect(overlap_width, 0, imgR.cols - overlap_width, imgR.rows)).copyTo(panorama(Rect(pos_img2.x + overlap_width, pos_img2.y, imgR.cols - overlap_width, imgR.rows)));
    overlap_result(Rect(0, 0, overlap_width, overlap_height)).copyTo(panorama(Rect(imgL.cols - overlap_width, 0, overlap_width, overlap_height)));

    //Panorama with seam
    imgL(Rect(0, 0, imgL.cols - overlap_width, imgL.rows)).copyTo(panorama_seam(Rect(pos_img1.x, pos_img1.y, imgL.cols - overlap_width, imgL.rows)));
    imgR(Rect(overlap_width, 0, imgR.cols - overlap_width, imgR.rows)).copyTo(panorama_seam(Rect(pos_img2.x + overlap_width, pos_img2.y, imgR.cols - overlap_width, imgR.rows)));
    overlap_result_seam(Rect(0, 0, overlap_width, overlap_height)).copyTo(panorama_seam(Rect(imgL.cols - overlap_width, 0, overlap_width, overlap_height)));

 
    //imwrite("./result/overlap_result.png", overlap_result);
    //imwrite("./result/overlap_result_seam.png", overlap_result_seam);
    imwrite("./result/colorMap.png", colorMap);

    imwrite("./result/panorama.png",panorama);
    imwrite("./result/panorama_seam.png", panorama_seam);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    typedef std::chrono::duration<float> float_seconds;
    auto secs = std::chrono::duration_cast<float_seconds>(duration);
    cout <<"\nexecution time: " <<secs.count() << "s" << endl;
 
    return 0;
}

