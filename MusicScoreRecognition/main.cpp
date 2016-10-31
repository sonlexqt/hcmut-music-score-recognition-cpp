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
char* WTITLE_ROTATED_IMAGE = "Rotated image";
Point point1, point2; /* vertical points of the bounding box */
Rect roiRect; /* bounding box */
Mat img,
    roiImg,
    roiImgGray,
    roiImgThresholded,
    blankRoiImg;
int MAX_ROTATION_ANGLE = 30;
int MIN_ROTATION_ANGLE = - MAX_ROTATION_ANGLE;


/************************************
 Helper functions
 ************************************/
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

float calculateEntropy(float PthetasAvg[], int length)
{
    float res = 0;
    for (int i = 0; i < length; i++)
    {
        if (PthetasAvg[i] != 0) res += PthetasAvg[i] * log(PthetasAvg[i]);
    }
    return -res;
}

/************************************
 Steps
 ************************************/

/*
 Step 1 - Read source image
 */
void readSourceImage()
{
    img = imread("test.jpg", 1);
    if (img.empty()) {
        throw invalid_argument("! ERROR: Image not found.");
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
    blankRoiImg = Mat(roiImgThresholded.size(), CV_8U);
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
    waitKey(0);
}

/*
 Step 4 - Rotation angle estimation
 */

void rotationAngleEstimation()
{
    int width = blankRoiImg.cols;
    int height = blankRoiImg.rows;
    int entropyPsLength = MAX_ROTATION_ANGLE - MIN_ROTATION_ANGLE + 1;
    float entropyPs[entropyPsLength];
    Mat rotatedImg;
    for (int a = 0; a < entropyPsLength; a++)
    {
        float Pthetas[height];
        float PthetasAvg[height];
        float sumPthetas = 0;
        int angle = a - MAX_ROTATION_ANGLE;
        Point2f center = Point2f(blankRoiImg.cols / 2., blankRoiImg.rows / 2.);
        Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
        warpAffine(blankRoiImg, rotatedImg, rotationMatrix, blankRoiImg.size());
        // The next calculation will use rotatedImg instead
        for (int h = 0; h < height; h++)
        {
            float sumOfRows = 0;
            for (int w = 0; w < width; w++)
            {
                sumOfRows += rotatedImg.at<uchar>(h, w);
            }
            Pthetas[h] = sumOfRows;
            sumPthetas += Pthetas[h];
        }
        for (int h = 0; h < height; h++)
        {
            PthetasAvg[h] = Pthetas[h] / sumPthetas;
        }
        entropyPs[a] = calculateEntropy(PthetasAvg, height);
    }
    
    int minA = 0;
    int minEntropy = entropyPs[minA];
    int minAngle = minA - MAX_ROTATION_ANGLE;
    for (int a = 0; a < entropyPsLength; a++)
    {
        if (entropyPs[a] < minEntropy)
        {
            minA = a;
            minEntropy = entropyPs[a];
            minAngle = minA - MAX_ROTATION_ANGLE;
        }
    }
    cout << "> Estimated rotation angle (deg): " << minAngle << endl;
    Point2f center = Point2f(blankRoiImg.cols / 2., blankRoiImg.rows / 2.);
    Mat rotationMatrix = getRotationMatrix2D(center, minAngle, 1.0);
    warpAffine(blankRoiImg, rotatedImg, rotationMatrix, blankRoiImg.size());
    imshow(WTITLE_ROTATED_IMAGE, rotatedImg);
    waitKey(0);
}

/************************************
 Main program
 ************************************/
int main()
{
    readSourceImage();
    roiSelection();
    candidatePointsExtraction();
    rotationAngleEstimation();
    return 0;
}
