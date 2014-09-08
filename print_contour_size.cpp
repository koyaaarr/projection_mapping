#include "headers.h"

using namespace std;
using namespace cv;

#define SRC_IMAGE "C:/Users/koyajima/Pictures/whiteback.png"

///////////////////   fullscreen.cpp   /////////////////////

typedef struct {
    int dispno;
    int arraysz;
    MONITORINFO *minfo;
} MonList;

bool CALLBACK
monCallback(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData);

MonList *
createMonList(HDC hdc, LPCRECT lprcClip);

int
releaseMonList(MonList **ml);

typedef struct {
    int width;
    int height;
    int x0; // x position of top-left corner
    int y0; // y position of top-left corner
    int monitor_id;
} ScreenInfo;


int
getScreenInfo(int monitor_id, ScreenInfo *si);

int
undecorateWindow(const char *win);

int
setWindowFullscreen(const char *win, ScreenInfo *si);

int
setWindowTranslucent(const char *win, unsigned char alpha);

int
setWindowChromaKeyed(const char *win, CvScalar color);

int main(){
    //cv::Mat src;
	vector< vector<cv::Point> > contours;
	//vector<cv::vector<cv::Point>> squares;
	//vector<cv::vector<cv::Point> > poly;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> approx;
	cv::Point2i scr_corner[4];
	const int idx=-1;
	const int thick=2;

	//カメラオープン
	cv::VideoCapture cap(1);
    if(!cap.isOpened())
        return -1;

	while(1)
	{

		cv::Mat proj = cv::imread(SRC_IMAGE);
		if(proj.empty()){
			cout << "error:image not found" << endl;
			return -1;
		}
		cv::namedWindow("projection");
		undecorateWindow("projection"); 
		ScreenInfo si;
		getScreenInfo(1, &si);
		setWindowFullscreen("projection", &si);
		cv::resize(proj, proj, cv::Size(si.width, si.height),0, 0, cv::INTER_CUBIC);

		Mat src, gray, bin;
		cap >> src;

		// カメラ取得画像が空ならcontinue
		if(src.empty())
		{
			cout << "src is empty" << endl;
			continue;
		}

		//グレースケール変換
		cv::cvtColor(src, gray, CV_BGR2GRAY); 

		//2値化
		cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY|cv::THRESH_OTSU); 

		//収縮・膨張
		//cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3); 
		//cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3);
		//cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
		//cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

		//輪郭検出
		cv::findContours (bin, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		//cv::drawContours (src, contours, -1, CV_RGB(0, 255, 0), 1, 8);

		//検出した輪郭ごとに見て回る
		for (unsigned int j = 0; j < contours.size(); j++){

			//輪郭を近似する
			cv::approxPolyDP(contours[j], approx, cv::arcLength(contours[j], true)*0.02, true);
			//頂点が4つの場合
			if (approx.size() == 4 && abs(approx[0].x - approx[2].x) > 100){
				//4つの頂点を描く
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5,  CV_RGB(255,0,0), 2, 8, 0);
				}
				//外枠取得用
				if(approx[0].x > approx[2].x && approx[0].y > approx[2].y){
					scr_corner[0] = cv::Point2f(approx[2].x,approx[2].y);
					scr_corner[1] = cv::Point2f(approx[3].x,approx[3].y);
					scr_corner[2] = cv::Point2f(approx[0].x,approx[0].y);
					scr_corner[3] = cv::Point2f(approx[1].x,approx[1].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y > approx[2].y){
					scr_corner[0] = cv::Point2f(approx[3].x,approx[3].y);
					scr_corner[1] = cv::Point2f(approx[0].x,approx[0].y);
					scr_corner[2] = cv::Point2f(approx[1].x,approx[1].y);
					scr_corner[3] = cv::Point2f(approx[2].x,approx[2].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y < approx[2].y){
					scr_corner[0] = cv::Point2f(approx[0].x,approx[0].y);
					scr_corner[1] = cv::Point2f(approx[1].x,approx[1].y);
					scr_corner[2] = cv::Point2f(approx[2].x,approx[2].y);
					scr_corner[3] = cv::Point2f(approx[3].x,approx[3].y);
				}		
				else if(approx[0].x > approx[2].x && approx[0].y < approx[2].y){
					scr_corner[0] = cv::Point2f(approx[1].x,approx[1].y);
					scr_corner[1] = cv::Point2f(approx[2].x,approx[2].y);
					scr_corner[2] = cv::Point2f(approx[3].x,approx[3].y);
					scr_corner[3] = cv::Point2f(approx[0].x,approx[0].y);
				}
			}
		}
		//高さ、幅　出力
		std::ostringstream os;
		os << "width=" << std::abs(scr_corner[0].x - scr_corner[2].x);
		std::string number = os.str();
		cv::putText(proj, number , cv::Point(100,200), cv::FONT_HERSHEY_SIMPLEX,
			3.2, cv::Scalar(255,0,255), 10, CV_AA);

		std::ostringstream os2;
		os2 << "height=" << std::abs(scr_corner[0].y - scr_corner[2].y);
		std::string number2 = os2.str();
		cv::putText(proj, number2 , cv::Point(100,400), cv::FONT_HERSHEY_SIMPLEX,
			3.2, cv::Scalar(255,0,255), 10, CV_AA);

		// 表示、キー入力で終了
		imshow("projection", proj);
		imshow("contours", src);
		if (waitKey(2) > 0) 
		{
            break;
        }
	}

    return 0;
}