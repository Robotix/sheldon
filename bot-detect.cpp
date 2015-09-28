#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <bits/stdc++.h>

using namespace cv;
using namespace std;

Mat src, imgHSV, head, tail, digits;

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

int h31 = 0;
int s31 = 0;
int v31 = 201;
int h32 = 146;
int s32 = 26;
int v32 = 270;

const int TMAX = 350;

void init_trackbars() {
	namedWindow("Head", 1);
	namedWindow("Tail", 1);
	// namedWindow("Digits", 1);
	// createTrackbar("H1", "Head", &h11, TMAX);
	// createTrackbar("S1", "Head", &s11, TMAX);
	// createTrackbar("V1", "Head", &v11, TMAX);
	// createTrackbar("H2", "Head", &h12, TMAX);
	// createTrackbar("S2", "Head", &s12, TMAX);
	// createTrackbar("V2", "Head", &v12, TMAX);
	// createTrackbar("H1", "Tail", &h21, TMAX); 
	// createTrackbar("S1", "Tail", &s21, TMAX);
	// createTrackbar("V1", "Tail", &v21, TMAX);
	// createTrackbar("H2", "Tail", &h22, TMAX);
	// createTrackbar("S2", "Tail", &s22, TMAX);
	// createTrackbar("V2", "Tail", &v22, TMAX);
	// createTrackbar("H1", "Digits", &h31, TMAX); 
	// createTrackbar("S1", "Digits", &s31, TMAX);
	// createTrackbar("V1", "Digits", &v31, TMAX);
	// createTrackbar("H2", "Digits", &h32, TMAX);
	// createTrackbar("S2", "Digits", &s32, TMAX);
	// createTrackbar("V2", "Digits", &v32, TMAX);
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
	VideoCapture cap("vids/vid3.wmv");
	cap >> src;
	Mat drawing(src.rows, src.cols, CV_8UC3, Scalar(0, 0, 0));
	while (1) {
		cvtColor(src, imgHSV, CV_BGR2HSV);
		inRange(imgHSV, Scalar(h11, s11, v11), Scalar(h12, s12, v12), head);
		inRange(imgHSV, Scalar(h21, s21, v21), Scalar(h22, s22, v22), tail);
		inRange(imgHSV, Scalar(h31, s31, v31), Scalar(h32, s32, v32), digits);
		// imshow("HEAD", head);
		// imshow("TAIL", tail);
		//imshow("DIGITS", digits);
		Point c1 = find_center(head, drawing, Scalar(255, 0, 0));
		Point c2 = find_center(tail, drawing, Scalar(255, 255, 15));

		float angle = atan((float)(c2.y - c1.y) / (float)(c2.x - c1.x)) * 180. / 3.14;
		cout << angle << endl;

		imshow("Main Vid", src);
		imshow("Dots", drawing);
		if (waitKey(30) >= 0) {
			break;
		}
	}
	return 0;
}