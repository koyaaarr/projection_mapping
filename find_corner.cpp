#include"headers.h"

STimerList st;

using namespace std;
using namespace cv;

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
	if(!cap.isOpened())
	{
		cout << "error:camera not found" << endl;
		return -1;
	}
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	cv::Mat src;
	//cv::namedWindow("Capture", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	cv::Mat marker = cv::imread("C:/Users/koyajima/Pictures/Projection_Mapping/id0102.png");
	if(marker.empty())
	{
		cout << "error:image not found" << endl;
		return -1;
	}
	cv::namedWindow("projection");
	undecorateWindow("projection"); 
	ScreenInfo si;
    getScreenInfo(1, &si);
	setWindowFullscreen("projection", &si);
	cv::Mat marker_fit;
    cv::resize(marker, marker_fit, cv::Size(si.width, si.height),0, 0, cv::INTER_CUBIC);

//       輪郭検出の初期設定

	Mat gray, bin;
	vector< vector<cv::Point> > contours;
	cv::vector<cv::vector<cv::Point>> squares;
	cv::vector<cv::vector<cv::Point> > poly;
	const int idx=-1;
	const int thick=2;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> approx;
	cv::Point2f pt_cs[4];

//        ARtoolkitの初期設定

	// ARToolKitPlus側
	ARToolKitPlus::TrackerMultiMarker *tracker;

	// tracker を作る
	tracker = new ARToolKitPlus::TrackerMultiMarkerImpl<6,6,12,32,64>(WIDTH, HEIGHT);
			// <marker_width, marker_height, sampling step, max num in cfg, max num in scene>
	tracker->init("calibration_data.cal", "marker_dummy.cfg", NEAR_LEN, FAR_LEN);

	// tracker オブジェクトの各種設定（通常このままでよい）
	tracker->setPixelFormat(ARToolKitPlus::PIXEL_FORMAT_BGR);
	tracker->setBorderWidth(0.125f); // thin
	tracker->activateAutoThreshold(true);
	tracker->setUndistortionMode(ARToolKitPlus::UNDIST_NONE);
	tracker->setPoseEstimator(ARToolKitPlus::POSE_ESTIMATOR_RPP);
	tracker->setMarkerMode(ARToolKitPlus::MARKER_ID_BCH);

	// マーカ座標系からカメラ座標系への変換行列を格納するための配列
	//float m34[ 3 ][ 4 ];
	// マーカ中心のオフセット（通常このままでよい）
	float c[ 2 ] = { 0.0f, 0.0f };
	// マーカの一辺のサイズ [mm] （実測値）
	float w = 59.0f;
	cv::Point2f corner[4];
	cv::Point2f outer[4];
	int radius = 5; // キーポイントの半径
	cv::Point2f pt_cp[4];

//         ホモグラフィ変換の初期設定

	//P座標設定
	Point2f ex_pt_pi[4];
	Point2f ex2_pt_pi[4];
	Point2f ex3_pt_pi[4];
	Point2f ex4_pt_pi[4];
	Point2f pt_pi[4];
	Point2f pt_po[4];
	//DVI用
	pt_pi[0] = cv::Point2f(483.0f,184.0f);
	pt_pi[1] = cv::Point2f(483.0f,584.0f);
	pt_pi[2] = cv::Point2f(883.0f,584.0f);
	pt_pi[3] = cv::Point2f(883.0f,184.0f);

	pt_po[0] = cv::Point2f(1,1);
	pt_po[1] = cv::Point2f(1,768);
	pt_po[2] = cv::Point2f(1366,768);
	pt_po[3] = cv::Point2f(1366,1);

	//HDMI用		
	//pt_pi[0] = cv::Point2f(327.0f,140.0f);
	//pt_pi[1] = cv::Point2f(327.0f,340.0f);
	//pt_pi[2] = cv::Point2f(527.0f,340.0f);
	//pt_pi[3] = cv::Point2f(527.0f,140.0f);

	//pt_po[0] = cv::Point2f(1,1);
	//pt_po[1] = cv::Point2f(1,480);
	//pt_po[2] = cv::Point2f(854,480);
	//pt_po[3] = cv::Point2f(854,1);

	//I座標設定
	//内枠
	const cv::Point2f pt_ii[] = {cv::Point2f(406,206),cv::Point2f(406,306),cv::Point2f(506,306),
		cv::Point2f(506,206)};
	//外枠	
	const cv::Point2f pt_io[] = {cv::Point2f(1,1),cv::Point2f(1,912),cv::Point2f(513,912),
		cv::Point2f(513,1)};
	//S座標設定
	//const cv::Point2f pt_si[] = {cv::Point2f(20,40),cv::Point2f(20,70),cv::Point2f(50,70),
	//							cv::Point2f(50,40)};
	//const cv::Point2f pt_so[] = {cv::Point2f(0,0),cv::Point2f(0,100),cv::Point2f(100,100),
	//							cv::Point2f(100,0)};
	cv::Point2f pt_si[4];
	cv::Point2f pt_so[4];
	//背景に写す用
	//pt_si[0] = cv::Point2f(327.0f,140.0f);
	//pt_si[1] = cv::Point2f(327.0f,340.0f);
	//pt_si[2] = cv::Point2f(527.0f,340.0f);
	//pt_si[3] = cv::Point2f(527.0f,140.0f);

	//pt_so[0] = cv::Point2f(1,1);
	//pt_so[1] = cv::Point2f(1,480);
	//pt_so[2] = cv::Point2f(854,480);
	//pt_so[3] = cv::Point2f(854,1);

	//A4に写す用
	pt_si[0] = cv::Point2f(112,72);
	pt_si[1] = cv::Point2f(112,147);
	pt_si[2] = cv::Point2f(187,147);
	pt_si[3] = cv::Point2f(187,72);

	pt_so[0] = cv::Point2f(0,0);
	pt_so[1] = cv::Point2f(0,210);
	pt_so[2] = cv::Point2f(297,210);
	pt_so[3] = cv::Point2f(297,0);

	//IからS
	const cv::Mat iHsi = cv::getPerspectiveTransform( pt_ii, pt_si);

	int temp=0;
	int frame=0;
	Mat sHc, pHc, warp;
	vector<Point2f> p_corner(4);
	cv::Mat iHp = cv::Mat(3,3,CV_32F);
	cv::Mat iHp_origin = cv::Mat(3,3,CV_32F);
	cv::Mat soHcs, cpHp;

	while(1) {
		st.laptime(0);

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
		for (unsigned int j = 0; j < contours.size(); j++)
		{
			approx = contours[j];
			//輪郭を近似する
			cv::approxPolyDP(contours[j], approx, cv::arcLength(contours[j], true)*0.02, true);
			//頂点が4つの場合
			if (approx.size() == 4 && hierarchy[j][2] != -1)
			{
				//4つの頂点を描く
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5,  CV_RGB(255,0,0), 2, 8, 0);
				}
			}
		}
		
/////////////////////////   ARtoolkit     ///////////////////////////

		// 画像中からマーカを検出
		tracker->calc((unsigned char *)(src.data));
		
		// 検出されたマーカの数
		unsigned int num = tracker->getNumDetectedMarkers();

		// 検出された各マーカ情報を見ていく
		for(unsigned int m = 0; m < num; m++){

			// i 番目のマーカ情報を取得（id 降順に並べられている様子）
			ARToolKitPlus::ARMarkerInfo a = tracker->getDetectedMarker(m);

			//取得した座標
			if(a.id==1)
			{
				corner[0].x = a.vertex[0][0];//右下
				corner[0].y = a.vertex[0][1];
				corner[1].x = a.vertex[1][0];
				corner[1].y = a.vertex[1][1];
				corner[2].x = a.vertex[2][0];
				corner[2].y = a.vertex[2][1];
				corner[3].x = a.vertex[3][0];
				corner[3].y = a.vertex[3][1];
			}
		}
		///////4点を描く
		for(int l = 0; l < 4; l++)
		{
			cv::circle(src, corner[l], radius,  CV_RGB(0,0,255), 5, 8, 0);
		}

		/*表示*/
		cv::imshow("projection", marker_fit);
		cv::imshow("Capture", src);
		//cv::imshow("bin", bin);

		if (waitKey(2) > 0) 
		{
            break;
        }

		}//ループ終わり

		return 0;
}