#include"headers.h"

STimerList st;

using namespace std;
using namespace cv;

#define SRC_IMAGE "C:/Users/koyajima/Pictures/Projection_Mapping/id0102.png"

#define P_WIDTH	1280
#define P_HEIGHT 800

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


////////////  main  /////////////

int main(int argc, char *argv[]){

//      Fullscreenの初期設定
	cv::VideoCapture cap(1);
	if(!cap.isOpened()){
		cout << "error:camera not found" << endl;
		return -1;
	}
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	//cv::namedWindow("Capture", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	cv::Mat prjcted = cv::imread(SRC_IMAGE);
	if(prjcted.empty()){
		cout << "error:image not found" << endl;
		return -1;
	}
	cv::namedWindow("projection");
	undecorateWindow("projection"); 
	ScreenInfo si;
    getScreenInfo(1, &si);
	setWindowFullscreen("projection", &si);
    cv::resize(prjcted, prjcted, cv::Size(si.width, si.height),0, 0, cv::INTER_CUBIC);

//       輪郭検出の初期設定
	
	Mat src, gray, bin;
	vector< vector<cv::Point> > contours;
	cv::vector<cv::vector<cv::Point>> squares;
	cv::vector<cv::vector<cv::Point> > poly;
	const int idx=-1;
	const int thick=2;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> approx;

	//cv::Point2f src_pt[4];
	cv::Point2f dst_pt[]={
		cv::Point2f( 0.0, 0.0),
		cv::Point2f((P_WIDTH-1)/2, 0),
		cv::Point2f((P_WIDTH-1)/2 , (P_HEIGHT-1)/2),
		cv::Point2f(0, (P_HEIGHT-1)/2)};

	namedWindow("Capture", CV_WINDOW_AUTOSIZE);

	while(1) {
		st.laptime(0);

		cv::Point2f src_pt[4];

		cap >> src;  // キャプチャ
		//グレースケール変換
		cv::cvtColor(src, gray, CV_BGR2GRAY); 

////////////////////////////////////   輪郭検出   //////////////////////////////// 

		//2値化
		cv::threshold(gray, bin, 100, 255, cv::THRESH_BINARY|cv::THRESH_OTSU); 

		//収縮・膨張
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3); 
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

		//輪郭検出
		cv::findContours (bin, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		//cv::drawContours (src, contours, -1, cv::Scalar(100), 2, 8);
		//検出した輪郭ごとに見て回る
		for (unsigned int j = 0; j < contours.size(); j++){
			approx = contours[j];
			//輪郭を近似する
			cv::approxPolyDP(contours[j], approx, cv::arcLength(contours[j], true)*0.02, true);
			//頂点が4つの場合
			if (approx.size() == 4 && hierarchy[j][2] != -1){
				//4つの頂点を描く
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5,  CV_RGB(255,0,0), 2, 8, 0);
				}
				/*外枠取得用*/
				src_pt[0] = cv::Point2f(approx[0].x,approx[0].y);
				src_pt[1] = cv::Point2f(approx[1].x,approx[1].y);
				src_pt[2] = cv::Point2f(approx[2].x,approx[2].y);
				src_pt[3] = cv::Point2f(approx[3].x,approx[3].y);
				if(approx[0].x > approx[2].x && approx[0].y > approx[2].y){
					src_pt[0] = cv::Point2f(approx[2].x,approx[2].y);
					src_pt[1] = cv::Point2f(approx[1].x,approx[1].y);
					src_pt[2] = cv::Point2f(approx[0].x,approx[0].y);
					src_pt[3] = cv::Point2f(approx[3].x,approx[3].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y > approx[2].y){
					src_pt[0] = cv::Point2f(approx[1].x,approx[1].y);
					src_pt[1] = cv::Point2f(approx[0].x,approx[0].y);
					src_pt[2] = cv::Point2f(approx[3].x,approx[3].y);
					src_pt[3] = cv::Point2f(approx[2].x,approx[2].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y < approx[2].y){
					src_pt[0] = cv::Point2f(approx[0].x,approx[0].y);
					src_pt[1] = cv::Point2f(approx[3].x,approx[3].y);
					src_pt[2] = cv::Point2f(approx[2].x,approx[2].y);
					src_pt[3] = cv::Point2f(approx[1].x,approx[1].y);
				}		
				else if(approx[0].x > approx[2].x && approx[0].y < approx[2].y){
					src_pt[0] = cv::Point2f(approx[3].x,approx[3].y);
					src_pt[1] = cv::Point2f(approx[2].x,approx[2].y);
					src_pt[2] = cv::Point2f(approx[1].x,approx[1].y);
					src_pt[3] = cv::Point2f(approx[0].x,approx[0].y);
				}
			}
		}

		if(src_pt[0].x != 0){
			// homography 行列を計算
			cv::Mat homography_matrix = cv::getPerspectiveTransform(src_pt, dst_pt);

			// 変換
			cv::warpPerspective( src, src, homography_matrix,src.size());
		}

		/*表示*/
		cv::imshow("projection", prjcted);
		cv::imshow("Capture", src);
		//cv::imshow("bin", bin);

		if (waitKey(2) > 0){
            break;
        }

		}//ループ終わり

		return 0;
}