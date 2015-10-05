#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <ctime>

using namespace cv;
using namespace std;

vector<pair<char, pair<int, int> > > expr_digits;
vector<pair<char, pair<int, int> > > expr_ops;
string expr;

Mat src; Mat src_gray; Mat src_binary;
int thresh = 228;
int max_thresh = 255;
RNG rng(12345);
tesseract::TessBaseAPI tess;

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

/// Function header
void thresh_callback(int, void*);

bool hasPrecedence(char op1, char op2) {
    if (op2 == '(' || op2 == ')')
        return false;
    if ((op1 == '*' || op1 == '/') && (op2 == '+' || op2 == '-'))
        return false;
    else
        return true;
}

int applyOp(char op, int b, int a)
{
    switch (op)
    {
    case '+':
        return a + b;
    case '-':
        return a - b;
    case '*':
        return a * b;
    case '/':
    	assert(b != 0);
        return a / b;
    }
    return 0;
}

int evaluate_epxr(const char *A) {
	stack<int> values;
	stack<char> ops;
	char c; int x, y;
	for(int i = 0; A[i] != '\0'; i++) {
		if (A[i] >= '0' && A[i] <= '9') {
			values.push(A[i]-'0');
		} else {
			while (!ops.empty() && hasPrecedence(A[i], ops.top())) {
            	c = ops.top();
            	ops.pop();
            	x = values.top();
            	values.pop();
            	y = values.top();
            	values.pop();
            	values.push(applyOp(c, x, y));
			}
            ops.push(A[i]);
		}
	}
	while (!ops.empty()) {
		c = ops.top();
    	ops.pop();
    	x = values.top();
    	values.pop();
    	y = values.top();
    	values.pop();
        values.push(applyOp(c, x, y));
	}
    return values.top();
}

void predictDigit(Rect rr) {
	//Mat sub = src_gray(rr);
	tess.SetImage((uchar*)src_binary.data, src_binary.size().width, src_binary.size().height, src_binary.channels(), src_binary.step1());
	tess.Recognize(0);
	char* out = tess.GetUTF8Text();
	char c = out[0];
	int cx = (rr.tl().x + rr.br().x) / 2;
	int cy = (rr.tl().y + rr.br().y) / 2;
	cout << c << endl;
	if (c == 'X' || c == 'x') c = '*';
	if (c >= '0' && c <= '9')
		expr_digits.push_back(make_pair(c, make_pair(cx, cy)));
	else
		expr_ops.push_back(make_pair(c, make_pair(cx, cy)));
}

/** @function main */
int main( int argc, char** argv )
{
	VideoCapture cap(1);
	tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
	tess.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
	//src = imread("img/sample1.jpg", 1 );
	cap >> src;
	src_binary = src;
	cvtColor( src, src_gray, CV_BGR2GRAY );
	blur( src_gray, src_gray, Size(3,3) );
	namedWindow( "source_window", CV_WINDOW_AUTOSIZE );
	imshow( "source_window", src );
	createTrackbar("Threshold:", "Source", &thresh, max_thresh, thresh_callback );
	thresh_callback(0, 0);

	sort(expr_digits.begin(), expr_digits.end());
	sort(expr_ops.begin(), expr_ops.end());
	int max_val = -1;
	string max_expr;
	do {
		do {
			expr = "";
			for(int i = 0, j = 0; i < expr_digits.size() || j < expr_ops.size(); (j < i) ? j++ : i++) {
				if (j < i)
					expr += expr_ops[j].first;
				else
					expr += expr_digits[i].first;
			}
			if (evaluate_epxr(expr.c_str()) > max_val) {
				max_val = evaluate_epxr(expr.c_str());
				max_expr = expr;
			}
			// puts(expr.c_str());
		} while (next_permutation(expr_ops.begin(), expr_ops.end()));
	} while (next_permutation(expr_digits.begin(), expr_digits.end()));
	// cout << max_expr << " : " << max_val << endl;
	vector<pair<int, int> > goToQ;
	for(int i = 0; i < max_expr.size(); i++) {
		if (max_expr[i] >= '0' && max_expr[i] <= '9') {
			for(int j = 0; j < expr_digits.size(); j++) {
				if (expr_digits[j].first == max_expr[i]) {
					goToQ.push_back(make_pair(expr_digits[j].second.first, expr_digits[j].second.second));
					break;
				}
			}
		} else {
			for(int j = 0; j < expr_ops.size(); j++) {
				if (expr_ops[j].first == max_expr[i]) {
					goToQ.push_back(make_pair(expr_ops[j].second.first, expr_ops[j].second.second));
					break;
				}
			}
		}
	}
	for(int i = 0; i < goToQ.size(); i++)
		cout << goToQ[i].first << ", " << goToQ[i].second << endl;

	state = 'b';
    fd = open("/dev/ttyACM1", O_RDWR | O_NOCTTY);  //Opening device file
    printf("fd opened as %i\n", fd);
   	usleep(2500000);
	init_trackbars();
	finalLoc.x = goToQ[0].first;
	finalLoc.y = goToQ[0].second;
	int vec_idx = 0;
	while (1) {
		cap >> src2;
		Mat drawing(src2.rows, src2.cols, CV_8UC3, Scalar(0, 0, 0));
		circle(drawing, finalLoc, 5, Scalar(22, 44, 66), CV_FILLED, 8, 0);
		cvtColor(src2, imgHSV, CV_BGR2HSV);
		inRange(imgHSV, Scalar(h11, s11, v11), Scalar(h12, s12, v12), head);
		inRange(imgHSV, Scalar(h21, s21, v21), Scalar(h22, s22, v22), tail);
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
		cout << "BOT DIST: " << dist(botLoc, finalLoc) << endl;

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

		if (dist(botLoc, finalLoc) < 40) {
			sendCommand("b");
			sendCommand("l");
			usleep(3000000);
			vec_idx++;
			finalLoc.x = goToQ[vec_idx].first;
			finalLoc.y = goToQ[vec_idx].second;
			if (vec_idx > goToQ.size())
				return 0;
		}

		imshow("Main Vid", src2);
		imshow("Dots", drawing);
		if (waitKey(60) >= 0) {
			break;
		}
	}

	waitKey(0);
	return(0);
}

/** @function thresh_callback */
void thresh_callback(int, void* )
{
	Mat threshold_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	threshold( src_gray, threshold_output, thresh, 255, THRESH_BINARY );
	/// Find contours
	findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	
	/// Approximate contours to polygons + get bounding rects and circles
	vector<vector<Point> > contours_poly( contours.size() );
	vector<Rect> boundRect;

	for( int i = 0; i < contours.size(); i++ )
	{
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
		double a = contourArea(contours[i]);
		cout << a << endl;
		if (a >= 300 && a <= 800)
			boundRect.push_back(boundingRect( Mat(contours_poly[i]) ));
	}
	/// Draw polygonal contour + bonding rects + circles
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	Mat zz;
	for(int i = 0; i< boundRect.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
		rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		zz = src_gray(boundRect[i]);
		Mat binaryMat(zz.size(), zz.type());
		threshold(zz, binaryMat, 150, 255, THRESH_BINARY);
		src_binary.rows = binaryMat.rows;
		src_binary.cols = binaryMat.cols;
		src_binary = binaryMat;
		imshow("sample" + i, src_binary);
		predictDigit(boundRect[i]);
	}
	/// Show in a window
	namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	imshow("Contours", drawing);
}