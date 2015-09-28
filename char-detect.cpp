#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>
#include <bits/stdc++.h>

using namespace cv;
using namespace std;

vector<pair<char, pair<int, int> > > expr_digits;
vector<pair<char, pair<int, int> > > expr_ops;
string expr;

Mat src; Mat src_gray;
int thresh = 100;
int max_thresh = 255;
RNG rng(12345);
tesseract::TessBaseAPI tess;

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
	Mat sub = src_gray(rr);
	tess.SetImage((uchar*)sub.data, sub.size().width, sub.size().height, sub.channels(), sub.step1());
	tess.Recognize(0);
	char* out = tess.GetUTF8Text();
	char c = out[0];
	int cx = (rr.tl().x + rr.br().x) / 2;
	int cy = (rr.tl().y + rr.br().y) / 2;
	if (c == 'X' || c == 'x') c = '*';
	if (c >= '0' && c <= '9')
		expr_digits.push_back(make_pair(c, make_pair(cx, cy)));
	else
		expr_ops.push_back(make_pair(c, make_pair(cx, cy)));
}

/** @function main */
int main( int argc, char** argv )
{
	tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
	tess.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
	src = imread("img/sheldon_data.jpg", 1 );
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
	// for(int i = 0; i < goToQ.size(); i++)
	// 	cout << goToQ[i].first << ", " << goToQ[i].second << endl;
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
		if (a >= 3800 && a <= 4000)
			boundRect.push_back(boundingRect( Mat(contours_poly[i]) ));
	}
	/// Draw polygonal contour + bonding rects + circles
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	Mat zz;
	for(int i = 0; i< boundRect.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
		rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		//zz = src_gray(boundRect[i]);
		predictDigit(boundRect[i]);
	}
	/// Show in a window
	namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	imshow("Contours", drawing);
}