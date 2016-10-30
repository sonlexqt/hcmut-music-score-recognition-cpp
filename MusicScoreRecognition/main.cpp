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

int isDragging = 0;
int isRoiSelected = 0;
int REC_LINE_WIDTH = 2;
char* WTITLE_SOURCE_IMAGE = "Source Image";
char* WTITLE_ROI_IMAGE = "ROI Image";
char* WTITLE_ROI_IMAGE_THRESHOLDED = "ROI Image Thresholded";
char* WTITLE_CANDIDATE_POINTS = "Candidate Points";
Point point1, point2; /* vertical points of the bounding box */
Rect roiRect; /* bounding box */
Mat img,
    roiImg,
    roiImgGray,
    roiImgThresholded;

/*
 Helper functions
 */
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


void waitForEscKey()
{
    int key;
    while (1)
    {
        key = waitKey(10);
        if (key == 27) break;
    }
}

bool isThisPixelRemoved(int i, int j, int value, int maxRows, int maxCols)
{
    if (value > 0)
    {
        if (i == 0 || i == 1) return true;
        if ((int)roiImgThresholded.at<uchar>(i - 1, j) > 0)
        {
            if ((int)roiImgThresholded.at<uchar>(i - 2, j) > 0) return false;
            if (j >= 1 && (int)roiImgThresholded.at<uchar>(i - 2, j - 1) > 0) return false;
            if (j < maxCols && (int)roiImgThresholded.at<uchar>(i - 2, j + 1) > 0) return false;
        }
    }
    return true;
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
    cvtColor(roiImg, roiImgGray, CV_BGR2GRAY);
    threshold(roiImgGray, roiImgThresholded, 127, 255, CV_THRESH_BINARY_INV);
    imshow(WTITLE_ROI_IMAGE_THRESHOLDED, roiImgThresholded);
    Mat blankRoiImg(roiImgThresholded.size(), CV_8U);
    for (int i = 0; i < roiImgThresholded.rows; i++)
        for (int j = 0; j < roiImgThresholded.cols; j++)
        {
            int pixelGrayScaleValue = (int) roiImgThresholded.at<uchar>(i, j);
            if (isThisPixelRemoved(i, j, pixelGrayScaleValue, roiImgThresholded.rows, roiImgThresholded.cols))
            {
                // Yes: copy this pixel to blankRoiImg
                blankRoiImg.at<uchar>(i, j) = pixelGrayScaleValue;
            }
        }
    threshold(blankRoiImg, blankRoiImg, 127, 255, CV_THRESH_BINARY);
    imshow(WTITLE_CANDIDATE_POINTS, blankRoiImg);
    waitForEscKey();
}

int main()
{
    readSourceImage();
    roiSelection();
    candidatePointsExtraction();
    return 0;
}
