#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

Mat src, imgHSV, head, tail;

int h11 = 59;
int s11 = 79;
int v11 = 58;
int h12 = 128;
int s12 = 138;
int v12 = 226;

int h21 = 116;
int s21 = 141;
int v21 = 153;
int h22 = 238;
int s22 = 243;
int v22 = 268;

const int TMAX = 350;

void init_trackbars() {
	namedWindow("Head", 1);
	namedWindow("Tail", 1);
	createTrackbar("H1", "Head", &h11, TMAX);
	createTrackbar("S1", "Head", &s11, TMAX);
	createTrackbar("V1", "Head", &v11, TMAX);
	createTrackbar("H2", "Head", &h12, TMAX);
	createTrackbar("S2", "Head", &s12, TMAX);
	createTrackbar("V2", "Head", &v12, TMAX);
	createTrackbar("H1", "Tail", &h21, TMAX); 
	createTrackbar("S1", "Tail", &s21, TMAX);
	createTrackbar("V1", "Tail", &v21, TMAX);
	createTrackbar("H2", "Tail", &h22, TMAX);
	createTrackbar("S2", "Tail", &s22, TMAX);
	createTrackbar("V2", "Tail", &v22, TMAX);
}

Point find_center(Mat img, Mat& drawing, Scalar col) {
	vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
	Rect bounding_rect;
	int largest_area = 0; 
    findContours(img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
    for(int i = 0; i< contours.size(); i++)
    {
    	double a = contourArea(contours[i], false); 
       	if(a > largest_area){
       		largest_area = a;
       		bounding_rect = boundingRect(contours[i]); 
       	}
	}
	Point p1 = bounding_rect.tl();
	Point p2 = bounding_rect.br();
	Point p((p1.x + p2.x)/2, (p1.y + p2.y)/2);
	circle(drawing, p, 5, col, CV_FILLED, 8, 0);
	return p;
}

int main(int argc, char const *argv[])
{
	init_trackbars();
	VideoCapture cap("vids/vid4.wmv");
	cap >> src;
	Mat drawing(src.rows, src.cols, CV_8UC3, Scalar(0, 0, 0));
	while (1) {
		cvtColor(src, imgHSV, CV_BGR2HSV);
		inRange(imgHSV, Scalar(h11, s11, v11), Scalar(h12, s12, v12), head);
		inRange(imgHSV, Scalar(h21, s21, v21), Scalar(h22, s22, v22), tail);
		//imshow("HEAD", head);
		//imshow("TAIL", tail);
		Point c1 = find_center(head, drawing, Scalar(255, 0, 0));
		Point c2 = find_center(tail, drawing, Scalar(255, 255, 15));
		imshow("Main Vid", src);
		imshow("Dots", drawing);
		if (waitKey(30) >= 0) {
			break;
		}
	}
	return 0;
}