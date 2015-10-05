#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <ctime>

using namespace cv;
using namespace std;

Mat src2, imgHSV, head, tail;

int h11 = 154;
int s11 = 67;
int v11 = 62;
int h12 = 190;
int s12 = 294;
int v12 = 302;

int h21 = 65;
int s21 = 193;
int v21 = 207;
int h22 = 243;
int s22 = 276;
int v22 = 286;

const int TMAX = 350;
char state;
int fd;
Point finalLoc(400, 200);

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
void sendCommand(const char* command) {
	if (command[0] != state) {
		write(fd, command, 1);
		state = command[0];
		cout << "sending " << state << endl;
	}
}

float dist(Point p1, Point p2) { return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)); }

int main(int argc, char const *argv[])
{
	state = 'b';
    fd = open("/dev/ttyACM1", O_RDWR | O_NOCTTY);  //Opening device file
    printf("fd opened as %i\n", fd);
   	usleep(2500000);
	init_trackbars();
	VideoCapture cap(1);
	while (1) {
		cap >> src2;
		Mat drawing(src2.rows, src2.cols, CV_8UC3, Scalar(0, 0, 0));
		circle(drawing, finalLoc, 5, Scalar(22, 44, 66), CV_FILLED, 8, 0);
		cvtColor(src2, imgHSV, CV_BGR2HSV);
		inRange(imgHSV, Scalar(h11, s11, v11), Scalar(h12, s12, v12), head);
		inRange(imgHSV, Scalar(h21, s21, v21), Scalar(h22, s22, v22), tail);
		inRange(imgHSV, Scalar(h31, s31, v31), Scalar(h32, s32, v32), digits);
		imshow("HEAD", head);
		imshow("TAIL", tail);
		// imshow("DIGITS", digits);
		Point c1 = find_center(head, drawing, Scalar(255, 0, 0));
		Point c2 = find_center(tail, drawing, Scalar(255, 255, 15));
		Point botLoc((c1.x + c2.x) / 2, (c1.y + c2.y) / 2);

		float m1 = (float)(c2.y - c1.y) / (float)(c2.x - c1.x);
		float m2 = (float)(finalLoc.y - c1.y) / (float)(finalLoc.x - c1.x);
		float anglediff = atan((m1 - m2) / (1.0 + m1*m2)) * 180. / 3.14;
		cout << "Current Bot: " << botLoc.x << ", " << botLoc.y << endl;
		cout << "GOTO: " << finalLoc.x << ", " << finalLoc.y << endl;
		cout << "Angle Diff: " << anglediff << endl;

		if (fabs(anglediff) > 20) {
			sendCommand("a");
		} else {
			cout << "------------------------------------" << endl;
			sendCommand("b");
		}

		if (!(state == 'a' || state == 'd')) {
			if (dist(botLoc, finalLoc) < 30) {
				sendCommand("b");
			} else {
				if (dist(c1, finalLoc) < dist(c2, finalLoc)) {
					sendCommand("w");
				} else {
					sendCommand("s");
				}
			}
		}

		if (dist(botLoc, finalLoc) < 20) {
			sendCommand("b");
		}

		imshow("Main Vid", src2);
		imshow("Dots", drawing);
		if (waitKey(60) >= 0) {
			break;
		}
	}
	return 0;
}