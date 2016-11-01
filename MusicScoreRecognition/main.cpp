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

/************************************
 Global variables declaration
 ************************************/
int isDragging = 0;
int isRoiSelected = 0;
int REC_LINE_WIDTH = 2;
char* WTITLE_SOURCE_IMAGE = "Source Image";
char* WTITLE_ROI_IMAGE = "ROI Image";
char* WTITLE_ROI_IMAGE_THRESHOLDED = "ROI Image Thresholded";
char* WTITLE_CANDIDATE_POINTS = "Candidate Points";
char* WTITLE_ROTATED_IMAGE = "Rotated image";
char* WTITLE_IMG_WO_STAFFLINES = "Image without stafflines";
Point point1, point2; /* vertical points of the bounding box */
Rect roiRect; /* bounding box */
Mat img,
    roiImg,
    roiImgGray,
    roiImgThresholded,
    blankRoiImg,
    rotatedImg,
    rotatedImgGray,
    rotatedImgThresholded,
    blankRotatedImg,
    withoutStaffLines;
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

bool isThisPixelRemoved(int i, int j, int value, Mat img)
{
    int maxCols = img.cols;
    if (value > 0)
    {
        if (i == 0 || i == 1) return true;
        if ((int)img.at<uchar>(i - 1, j) > 0)
        {
            if ((int)img.at<uchar>(i - 2, j) > 0) return false;
            if (j >= 1 && (int)img.at<uchar>(i - 2, j - 1) > 0) return false;
            if (j < maxCols && (int)img.at<uchar>(i - 2, j + 1) > 0) return false;
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
    //imshow(WTITLE_ROI_IMAGE_THRESHOLDED, roiImgThresholded);
    blankRoiImg = Mat(roiImgThresholded.size(), CV_8U);
    for (int i = 0; i < roiImgThresholded.rows; i++)
        for (int j = 0; j < roiImgThresholded.cols; j++)
        {
            int pixelGrayScaleValue = (int) roiImgThresholded.at<uchar>(i, j);
            if (isThisPixelRemoved(i, j, pixelGrayScaleValue, roiImgThresholded))
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
    Mat rotatedRoiImg;
    for (int a = 0; a < entropyPsLength; a++)
    {
        float Pthetas[height];
        float PthetasAvg[height];
        float sumPthetas = 0;
        int angle = a - MAX_ROTATION_ANGLE;
        Point2f center = Point2f(blankRoiImg.cols / 2., blankRoiImg.rows / 2.);
        Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
        warpAffine(blankRoiImg, rotatedRoiImg, rotationMatrix, blankRoiImg.size());
        // The next calculation will use rotatedRoiImg instead
        for (int h = 0; h < height; h++)
        {
            float sumOfRows = 0;
            for (int w = 0; w < width; w++)
            {
                sumOfRows += rotatedRoiImg.at<uchar>(h, w);
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
    Point2f center = Point2f(img.cols / 2., img.rows / 2.);
    Mat rotationMatrix = getRotationMatrix2D(center, minAngle, 1.0);
    // Maintain white background when using opencv warpAffine
    warpAffine(img, rotatedImg, rotationMatrix, img.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(255, 255, 255));
    imshow(WTITLE_ROTATED_IMAGE, rotatedImg);
    waitKey(0);
}

/*
 Step 5 - Adaptive removal
 */

void adaptiveRemoval()
{
    int width = rotatedImg.cols;
    int height = rotatedImg.rows;
    float Pthetas[height];
    // Convert rotatedImg to rotatedImgThresholded
    cvtColor(rotatedImg, rotatedImgGray, CV_BGR2GRAY);
    threshold(rotatedImgGray, rotatedImgThresholded, 127, 255, CV_THRESH_BINARY_INV);
    // Calculate horizontal projection
    for (int h = 0; h < height; h++)
    {
        float sumOfRows = 0;
        for (int w = 0; w < width; w++)
        {
            sumOfRows += rotatedImgThresholded.at<uchar>(h, w);
        }
        Pthetas[h] = sumOfRows;
    }
    // Get the maximum of the projection (T)
    float T = Pthetas[0];
    for (int h = 0; h < height; h++)
    {
        if (Pthetas[h] > T) T = Pthetas[h];
    }
    // Then the projection is binarized using a threshold of half of the maximum
    for (int h = 0; h < height; h++)
    {
        // TODO XIN currently using a hardcoded number 4 here
        if (Pthetas[h] > (T / 4)) Pthetas[h] = 255;
        else Pthetas[h] = 0;
    }
    // Estimate the staff line width
    float W = 0;
    int numberOfRows = 0;
    int numberOfLines = 0;
    for (int h = 0; h < height; h++)
    {
        if (Pthetas[h] == 255)
        {
            numberOfRows += 1;
            if (h >= 1 && Pthetas[h - 1] != 255)
            {
                // Count this case as a new line
                numberOfLines += 1;
            }
        }
    }
    W = numberOfRows / numberOfLines;
    // Z is the number of times we need to run the staff line removal method
    int Z = ceil(W / 2);
    withoutStaffLines = rotatedImgThresholded.clone();
    blankRotatedImg = Mat(withoutStaffLines.size(), CV_8U);
    for (int z = 0; z < Z; z++)
    {
        for (int i = 0; i < withoutStaffLines.rows; i++)
        {
            for (int j = 0; j < withoutStaffLines.cols; j++)
            {
                int pixelGrayScaleValue = (int) withoutStaffLines.at<uchar>(i, j);
                if (isThisPixelRemoved(i, j, pixelGrayScaleValue, withoutStaffLines))
                {
                    // Yes: copy this pixel to blankRoiImg
                    blankRotatedImg.at<uchar>(i, j) = pixelGrayScaleValue;
                }
            }
        }
        withoutStaffLines = withoutStaffLines - blankRotatedImg;
    }
    threshold(withoutStaffLines, withoutStaffLines, 127, 255, CV_THRESH_BINARY_INV);
    imshow(WTITLE_IMG_WO_STAFFLINES, withoutStaffLines);
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
    adaptiveRemoval();
    return 0;
}
