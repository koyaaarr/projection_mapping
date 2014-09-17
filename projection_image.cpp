#include "headers.h"

STimerList st;

using namespace std;
using namespace cv;

#define DEBUG

#define SRC_IMAGE "C:/Users/koyajima/Pictures/Projection_Mapping/id0103.png"
#define ARMARKER "C:/Users/koyajima/Pictures/Projection_Mapping/id0102.png"
#define TEMPLATE "C:/Users/koyajima/Pictures/Projection_Mapping/lenna.png"

#define UPDATE_FRAME
#define A4_SCREEN
#define PROJECTOR_SIZE_SCREEN

#define CLICK_PROCEED
#define FRAME_PROCEED

#define RECORDING
#define RECORD_FRAME


//////////////////////////    ARtoolikitの設定    //////////////////////////////

// tracker に渡す画像のサイズ
#define WIDTH	640
#define HEIGHT	480

// 近遠クリッピングの距離（通常このままでよい）
#define NEAR_LEN	1.0
#define FAR_LEN		10000.0

///////////////////////////    Fullscreenの設定    /////////////////////////////

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

//////////////////////      cv::VideoWriter        /////////////////////////
 
void
verbose_printf(int verbose, const char *fmt, int arg = 0)
{
    if (verbose) {
        printf(fmt, arg);
        fflush(stdout);
    }
}

void
writeTimedImages(cv::VideoWriter& writer, cv::Mat *video, double *timestamp,
                 int nframe, double fps, int verbose = 0)
{
    double timebase = timestamp[0];
    verbose_printf(verbose, "written frames (out of %d):   0", nframe);

    for (int i = 0; ; ) {
        writer << video[i];
        verbose_printf(verbose, "\b\b\b%3d", i + 1);
        if (i >= nframe - 1) {
            break;
        }

        timebase += (1000.0 / fps);
        while (i < nframe - 1 && timebase > timestamp[i + 1]) {
            i++;
        }
    }

    verbose_printf(verbose, "\ndone\n");
}

//////////////////////   ESM   ///////////////////////////

enum track_state { TRACK_OFF, TRACK_READY, TRACK_SEL };
struct mouse_param {
    cv::Point topleft;
    cv::Rect trect;
    enum track_state tstate;
    bool init_requested;
};

void
onMouse(int event, int x, int y, int flags, void *param)
{
    struct mouse_param *mpp = (struct mouse_param *)param;

    if (mpp->tstate == TRACK_SEL) {
        if (x > mpp->topleft.x && y > mpp->topleft.y) {
            mpp->trect.width = x - mpp->topleft.x;
            mpp->trect.height = y - mpp->topleft.y;
        } else {
            mpp->trect.width = 0;
            mpp->trect.height = 0;
        }
    }

    switch (event) {
    case CV_EVENT_LBUTTONDOWN:
        mpp->topleft = cv::Point(x, y);
        mpp->trect.x = x;
        mpp->trect.y = y;
        mpp->trect.width = 0;
        mpp->trect.height = 0;
        mpp->tstate = TRACK_SEL;
        break;
    case CV_EVENT_LBUTTONUP:
        if (mpp->trect.width > 0 && mpp->trect.height > 0) {
            mpp->init_requested = true;
            mpp->tstate = TRACK_READY;
        } else {
            mpp->tstate = TRACK_OFF;
        }
        break;
    }
}

void
visualizeROI(const Mat& image, const Mat& target,
             const Mat& G, Mat& roi_disp)
{
    int gap = 5;
    int trows = target.rows;
    int tcols = target.cols;
    roi_disp = Mat::zeros(trows, tcols * 3 + 2 * gap, CV_8U);
    target.copyTo(roi_disp(Rect(0, 0, tcols, trows)));

    Mat cur_roi;
    warpPerspective(image, cur_roi, G, target.size(),
                    INTER_LINEAR | WARP_INVERSE_MAP);
    cur_roi.copyTo(roi_disp(Rect(tcols + gap, 0, tcols, trows)));

    Mat diff(trows, tcols, CV_16S);
    diff = target - cur_roi;
    diff.convertTo(diff, CV_8U, 4, 128);
    diff.copyTo(roi_disp(Rect(2 * tcols + 2 * gap, 0, tcols, trows)));
}


/////////////////////main////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){

//      Fullscreenの初期設定
	cv::VideoCapture cap(1);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	cv::Mat src;
	//cv::namedWindow("Capture", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	cv::Mat marker = cv::imread(ARMARKER);
	cv::Mat lenna = cv::imread(SRC_IMAGE);
	cv::namedWindow("projection");
	undecorateWindow("projection"); 
	ScreenInfo si;
    getScreenInfo(1, &si);
	setWindowFullscreen("projection", &si);
	cv::Mat window_fit;
    cv::resize(lenna, window_fit, cv::Size(si.width, si.height),0, 0, cv::INTER_CUBIC);

//       輪郭検出の初期設定

	Mat bin;
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
	Point2f pt_pi[4];
	Point2f pt_po[4];
	//HDMI用		
	pt_pi[0] = cv::Point2f(327.0f,140.0f);
	pt_pi[1] = cv::Point2f(327.0f,340.0f);
	pt_pi[2] = cv::Point2f(527.0f,340.0f);
	pt_pi[3] = cv::Point2f(527.0f,140.0f);

	pt_po[0] = cv::Point2f(1,1);
	pt_po[1] = cv::Point2f(1,480);
	pt_po[2] = cv::Point2f(854,480);
	pt_po[3] = cv::Point2f(854,1);

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

	//テンプレートT座標設定
	const Point2f pt_t[] = {cv::Point2f(0,0),cv::Point2f(0,47),cv::Point2f(47,47),
		cv::Point2f(47,0)};

	//IからS
	cv::Mat iHsi = cv::getPerspectiveTransform( pt_ii, pt_si);
	iHsi.convertTo(iHsi, CV_32F);
	//TからI
	cv::Mat tHi = cv::getPerspectiveTransform( pt_t, pt_ii);
	tHi.convertTo(tHi, CV_32F);
	//iHp_origin
	cv::Mat iHp_origin = cv::getPerspectiveTransform( pt_ii, pt_pi);
	iHp_origin.convertTo(iHp_origin, CV_32F);

	int temp=0;
	int frame=0;
	const int check=10;
	int flag;
	Mat warp;
	vector<Point2f> p_corner(4);
	cv::Mat iHp = cv::Mat(3,3,CV_32F);
	cv::Mat iHp_old = cv::Mat(3,3,CV_32F);
	cv::Mat soHcs = cv::Mat(3,3,CV_32F);
	cv::Mat cpHp = cv::Mat(3,3,CV_32F);
	iHp_origin.copyTo(iHp_old);

/////////////////////////     ESM     //////////////////////

	//テンプレートの設定
	cv::Mat tpl = cv::imread(TEMPLATE);
	cvtColor(tpl, tpl, CV_RGB2GRAY);
	cv::Rect tpl_rect;
	tpl_rect.x = 0;
	tpl_rect.y = 0;
	tpl_rect.width = 480;
	tpl_rect.height = 480;
	//探索範囲の設定
	cv::Rect srh;

    //namedWindow("disp", CV_WINDOW_AUTOSIZE);
    //namedWindow("roi", CV_WINDOW_AUTOSIZE);

    struct mouse_param mparam;
    mparam.init_requested = false;
    mparam.tstate = TRACK_OFF;

    //cv::setMouseCallback("disp", onMouse, &mparam);

    Mat disp, roi_disp;
    Mat image, target;

    // create (but do not initialize) esm_tracker
    TrackerESM esm_tracker;
    esm_tracker.setIter(10);
    Mat G = cv::Mat(3,3,CV_32F);

/////////////////    VideoWriterの設定   //////////////////

    // prepare variables
    double fps = 29.97;       // 動画フレームレート (frames/s)
    const int nframe = 500;   // 撮影画像枚数
    int verbose = 1;          // コンソールに進捗状況を出力するかどうか
    cv::Mat video[nframe];    // 画像の配列
    double timestamp[nframe]; // 各画像のタイムスタンプ (単位; ミリ秒)
    double freq = (double)cv::getTickFrequency();
	int i=0;

	while(1) {
		
		while(1){
			int key;
			//if(key == 'a'){
			//	goto RETURN_0;
			//}
			if ((key = cv::waitKey(10)) > 0) {
				if (key == 'q' || key == 0x1b) {
					break;
				}
			}
		}


		st.laptime(0);
		st.start(1);
		cap >> src;  // キャプチャ
		st.stop(1);

		st.start(2);
		src.copyTo(disp);
		//グレースケール変換
		cv::cvtColor(disp, image, CV_BGR2GRAY);


////////////////////////////////////   輪郭検出   //////////////////////////////// 


		//2値化
		cv::threshold(image, bin, 200, 255, cv::THRESH_BINARY|cv::THRESH_OTSU); 

		//収縮・膨張
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3); 
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

		//輪郭検出
		cv::findContours (bin, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

		//検出した輪郭ごとに見て回る
		for (unsigned int j = 0; j < contours.size(); j++){

		//輪郭を近似する
		cv::approxPolyDP(contours[j], approx, cv::arcLength(contours[j], true)*0.02, true);
			//頂点が4つの場合
			if (approx.size() == 4 && hierarchy[j][2] != -1){
				//4つの頂点を描く
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5,  CV_RGB(255,0,0), 2, 8, 0);
				}
			//外枠取得用
				if(approx[0].x > approx[2].x && approx[0].y > approx[2].y){
					pt_cs[0] = cv::Point2f(approx[2].x,approx[2].y);
					pt_cs[1] = cv::Point2f(approx[3].x,approx[3].y);
					pt_cs[2] = cv::Point2f(approx[0].x,approx[0].y);
					pt_cs[3] = cv::Point2f(approx[1].x,approx[1].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y > approx[2].y){
					pt_cs[0] = cv::Point2f(approx[3].x,approx[3].y);
					pt_cs[1] = cv::Point2f(approx[0].x,approx[0].y);
					pt_cs[2] = cv::Point2f(approx[1].x,approx[1].y);
					pt_cs[3] = cv::Point2f(approx[2].x,approx[2].y);
				}
				else if(approx[0].x < approx[2].x && approx[0].y < approx[2].y){
					pt_cs[0] = cv::Point2f(approx[0].x,approx[0].y);
					pt_cs[1] = cv::Point2f(approx[1].x,approx[1].y);
					pt_cs[2] = cv::Point2f(approx[2].x,approx[2].y);
					pt_cs[3] = cv::Point2f(approx[3].x,approx[3].y);
				}		
				else if(approx[0].x > approx[2].x && approx[0].y < approx[2].y){
					pt_cs[0] = cv::Point2f(approx[1].x,approx[1].y);
					pt_cs[1] = cv::Point2f(approx[2].x,approx[2].y);
					pt_cs[2] = cv::Point2f(approx[3].x,approx[3].y);
					pt_cs[3] = cv::Point2f(approx[0].x,approx[0].y);
				}
			}
		}
		st.stop(2);

		
/////////////////////////   ARtoolkit     ///////////////////////////

		if(frame<20){
		st.start(3);
		// 画像中からマーカを検出
		tracker->calc((unsigned char *)(src.data));
		
		// 検出されたマーカの数
		unsigned int num = tracker->getNumDetectedMarkers();

		// 検出された各マーカ情報を見ていく
		for(unsigned int m = 0; m < num; m++){

			// i 番目のマーカ情報を取得（id 降順に並べられている様子）
			ARToolKitPlus::ARMarkerInfo a = tracker->getDetectedMarker(m);

			//取得した座標
			if(a.id==1){
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
		//4点を描く
		if(frame<20){
		for(int l = 0; l < 4; l++){
			cv::circle(src, corner[l], radius,  CV_RGB(0,0,255), 5, 8, 0);
			cv::circle(disp, corner[l], radius,  CV_RGB(0,0,255), 5, 8, 0);
		}
		}
		//内枠取得用
		if(corner[0].x > corner[2].x && corner[0].y > corner[2].y){
		pt_cp[0] = cv::Point2f(corner[2].x,corner[2].y);
		pt_cp[1] = cv::Point2f(corner[1].x,corner[1].y);
		pt_cp[2] = cv::Point2f(corner[0].x,corner[0].y);
		pt_cp[3] = cv::Point2f(corner[3].x,corner[3].y);
		}
		else if(corner[0].x < corner[2].x && corner[0].y > corner[2].y){
		pt_cp[0] = cv::Point2f(corner[1].x,corner[1].y);
		pt_cp[1] = cv::Point2f(corner[0].x,corner[0].y);
		pt_cp[2] = cv::Point2f(corner[3].x,corner[3].y);
		pt_cp[3] = cv::Point2f(corner[2].x,corner[2].y);
		}
		else if(corner[0].x < corner[2].x && corner[0].y < corner[2].y){
		pt_cp[0] = cv::Point2f(corner[0].x,corner[0].y);
		pt_cp[1] = cv::Point2f(corner[3].x,corner[3].y);
		pt_cp[2] = cv::Point2f(corner[2].x,corner[2].y);
		pt_cp[3] = cv::Point2f(corner[1].x,corner[1].y);
		}		
		else if(corner[0].x > corner[2].x && corner[0].y < corner[2].y){
		pt_cp[0] = cv::Point2f(corner[3].x,corner[3].y);
		pt_cp[1] = cv::Point2f(corner[2].x,corner[2].y);
		pt_cp[2] = cv::Point2f(corner[1].x,corner[1].y);
		pt_cp[3] = cv::Point2f(corner[0].x,corner[0].y);
		}
		st.stop(3);
		}

//////////////////////////    ESM    /////////////////////////////

		st.start(4);
        if (!esm_tracker.empty()) {
            G = esm_tracker.track(image, G);
        }

        if (frame>25) {
            vector<Point> corner_warped_int(4);
            transformCorners(target.size(), G, corner_warped_int);
            polylines(src, corner_warped_int, 1, CV_RGB(255, 0, 0), 3);
        } 
		//else if (mparam.tstate == TRACK_SEL &&
  //                 mparam.trect.width > 0 &&
  //                 mparam.trect.height > 0) {
  //          rectangle(disp, mparam.trect, CV_RGB(255, 0, 0), 2);
  //      }
		//std::cout << "G = " << G << "\n" << std::endl;
		//std::cout << "iHp_old = " << iHp_old << "\n" << std::endl;

		st.stop(4);
//////////////////////////ホモグラフィ変換//////////////////////

		st.start(5);
		////3フレーム前のpt_piを使う
		for(int n = 0; n < 4; n++){
		ex3_pt_pi[n] = ex2_pt_pi[n];
		}
		for(int n = 0; n < 4; n++){
		ex2_pt_pi[n] = ex_pt_pi[n];
		}
		for(int n = 0; n < 4; n++){
		ex_pt_pi[n] = pt_pi[n];
		}
		//ホモグラフィ行列作成
		//SからC
		soHcs = cv::getPerspectiveTransform( pt_so, pt_cs);
		soHcs.convertTo(soHcs, CV_32F);
		//CからP
		cpHp = cv::getPerspectiveTransform( pt_cp, ex3_pt_pi);
		cpHp.convertTo(cpHp, CV_32F);

		//フレーム
		frame++;
		//std::cout << "frame=" << frame << std::endl;
		flag = frame%check;

		//IからPへ透視変換
		if(frame>50&&flag==0){
		iHp = iHp_old * tHi * G.inv() * soHcs * iHsi;
		iHp.copyTo(iHp_old);
		}

		if(frame<11){
		cv::warpPerspective( marker, warp, iHp_origin, window_fit.size());
		}
		if(frame>10&&frame<60){//
		cv::warpPerspective( lenna, warp, iHp_origin, window_fit.size());
		}
		//frame>10からトラッキング
		if(frame>59){
		cv::warpPerspective( lenna, warp, iHp, window_fit.size());
		p_corner[0] = pt_ii[0];
		p_corner[1] = pt_ii[1];
		p_corner[2] = pt_ii[2];
		p_corner[3] = pt_ii[3];
		perspectiveTransform(p_corner, p_corner, iHp);
		pt_pi[0] = p_corner[0];
		pt_pi[1] = p_corner[1];
		pt_pi[2] = p_corner[2];
		pt_pi[3] = p_corner[3];
		}
		st.stop(5);

		st.start(6);

		//今のフレームを表示する
		//std::ostringstream os;
		//os << frame;
		//std::string number = os.str();
		//cv::putText(warp, number , cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX,
		//			1.2, cv::Scalar(255,0,255), 2, CV_AA);

		//表示
		cv::imshow("projection", warp);

		cv::imshow("Capture", src);
		//cv::imshow("bin", bin);
		//imshow("disp", disp);

		////videowriter
		//disp.copyTo(video[i]);
  //      timestamp[i] = 1000.0 * (double)cv::getTickCount() / freq;
		//i++;

        //if (!target.empty()) {
        //    visualizeROI(image, target, G, roi_disp);
        //    imshow("roi", roi_disp);
        //}

		if (waitKey(2) > 0) {
            break;
        }


        if (frame==20) {
		srh.x = pt_cp[0].x;
		srh.y = pt_cp[0].y;
		srh.width = pt_cp[3].x - pt_cp[0].x;
		srh.height = pt_cp[1].y - pt_cp[0].y;
		G = esm_tracker.init(tpl, srh, Size(48, 48));
            esm_tracker.copyTargetTo(target);
            mparam.init_requested = false;
        }
		//debug用
		temp++;
 		//std::cout << temp << std::endl;
		//if(temp==500){
		//break;
		//}

		st.stop(6);
		}//ループ終わり
		
		//// prepare VideoWriter
		//cv::VideoWriter writer("video.avi", CV_FOURCC_PROMPT,
		//	fps, video[0].size());
		//if (!writer.isOpened()) {
		//	fprintf(stderr, "cannot create writer\n");
		//	exit(1);
		//}
		//// and write
		//writeTimedImages(writer, video, timestamp, nframe, fps, verbose);

RETURN_0:
		return 0;
}