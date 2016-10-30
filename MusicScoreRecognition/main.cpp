//
//  main.cpp
//  MusicScoreRecognition
//
//  Created by Son Le on 9/30/16.
//  Copyright © 2016 Son Le. All rights reserved.
//

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>

using namespace std;
using namespace cv;

//Mat src;
//Mat dst;
//Mat src_gray;
//Mat src_thresholded;
//char window_name1[] = "Unprocessed Image";
//char window_name2[] = "Processed Image";
//
//int main( int argc, char** argv )
//{
//    src = imread("test.jpg", 1);
//    if (src.empty()) {
//        cerr << "ERROR: Image not found." << endl;
//        return -1;
//    }
//    
//    cvtColor(src, src_gray, CV_BGR2GRAY);
//    adaptiveThreshold(src_gray, src_thresholded, 255, CV_ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 25, 0);
//    threshold(src_gray, src_thresholded, 127, 255, 0);
//    
//    //    vector<Mat> bgr_planes_original;
//    //    split( src, bgr_planes_original);
//    //    cout << "bgr_planes_original " << bgr_planes_original.size() << endl;
//    //
//    //    vector<Mat> bgr_planes_gray;
//    //    split( src, bgr_planes_gray);
//    //    cout << "bgr_planes_gray " << bgr_planes_gray.size() << endl;
//    
//    vector<Mat> bgr_planes_thresholded;
//    split( src_thresholded, bgr_planes_thresholded);
//    cout << "bgr_planes_thresholded " << bgr_planes_thresholded.size() << endl;
//    
//    namedWindow("result");
//    imshow("result", src_thresholded);
//    
//    waitKey();
//    return 0;
//}

int isDragging = 0;
int isRoiSelected = 0;
int REC_LINE_WIDTH = 2;
char* WTITLE_SOURCE_IMAGE = "Source Image";
char* WTITLE_ROI_IMAGE = "ROI Image";
Point point1, point2; /* vertical points of the bounding box */
Rect roiRect; /* bounding box */
Mat img, roiImg; /* roiImg - the part of the image in the bounding box */

void mouseDragHandler(int event, int x, int y, int flags, void* param)
{
    if (event == CV_EVENT_LBUTTONDOWN && !isDragging)
    {
        /* left button clicked. ROI selection begins */
        point1 = Point(x, y);
        isDragging = 1;
    }
    
    if (event == CV_EVENT_MOUSEMOVE && isDragging)
    {
        /* mouse dragged. ROI being selected */
        Mat img1 = img.clone();
        point2 = Point(x, y);
        rectangle(img1, point1, point2, CV_RGB(255, 0, 0), REC_LINE_WIDTH, 8, 0);
        imshow(WTITLE_SOURCE_IMAGE, img1);
    }
    
    if (event == CV_EVENT_LBUTTONUP && isDragging)
    {
        point2 = Point(x, y);
        roiRect = Rect(point1.x + REC_LINE_WIDTH,
                       point1.y + REC_LINE_WIDTH,
                       x - point1.x - REC_LINE_WIDTH,
                       y - point1.y - REC_LINE_WIDTH);
        isDragging = 0;
        roiImg = img(roiRect);
    }
    
    if (event == CV_EVENT_LBUTTONUP)
    {
        /* ROI selected */
        isDragging = 0;
        isRoiSelected = 1;
    }
}

/*
 Step 1 - Read source image
 */
void readSourceImage()
{
    img = imread("happy-birthday-rotated-white-resized.jpg", 1);
    if (img.empty()) {
        throw invalid_argument("ERROR: Image not found.");
        return;
    }
    imshow(WTITLE_SOURCE_IMAGE, img);
}

/*
 Step 2 - ROI selection
 */
void roiSelection()
{
    int key;
    cvSetMouseCallback(WTITLE_SOURCE_IMAGE, mouseDragHandler, NULL);
    while (1)
    {
        if (isRoiSelected == 1)
        {
            imshow(WTITLE_ROI_IMAGE, roiImg); /* show the image bounded by the box */
        }
        key = waitKey(10);
        if (key == 27) break;
    }
}

/*
 Step 3 - Candidate points extraction
 */

void candidatePointsExtraction()
{
    cout << "TODO XIN candidate points extraction" << endl;
    cout << endl;
}

int main()
{
    readSourceImage();
    roiSelection();
    candidatePointsExtraction();
    return 0;
}
