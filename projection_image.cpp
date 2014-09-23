#include "headers.h"

STimerList st;

using namespace std;
using namespace cv;

#define DEBUG

#define SRC_IMAGE "C:/Users/koyajima/Pictures/Projection_Mapping/id0103.png"
#define ARMARKER "C:/Users/koyajima/Pictures/Projection_Mapping/id0102.png"
#define TEMPLATE "C:/Users/koyajima/Pictures/Projection_Mapping/lenna.png"

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



////////////  main  /////////////

int main(int argc, char *argv[]){

//      Fullscreen�̏����ݒ�
//      Fullscreen�̏����ݒ�
	cv::VideoCapture cap(1);
	if(!cap.isOpened()){
		cout << "error:camera not found" << endl;
		return -1;
	}
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	//cv::namedWindow("Capture", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	cv::Mat proj = cv::imread(ARMARKER);
	if(proj.empty()){
		cout << "error:image not found" << endl;
		return -1;
	}
	cv::Mat cont = cv::imread(SRC_IMAGE);
	if(cont.empty()){
		cout << "error:image not found" << endl;
		return -1;
	}
	cv::namedWindow("projection");
	undecorateWindow("projection"); 
	ScreenInfo si;
    getScreenInfo(1, &si);
	setWindowFullscreen("projection", &si);
	Mat proj_fit;
    cv::resize(proj, proj_fit, cv::Size(si.width, si.height),0, 0, cv::INTER_CUBIC);

//       �֊s���o�̏����ݒ�

	Mat src, gray, bin;
	vector< vector<cv::Point> > contours;
	cv::vector<cv::vector<cv::Point>> squares;
	cv::vector<cv::vector<cv::Point> > poly;
	const int idx=-1;
	const int thick=2;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> approx;
	cv::Point2f pt_cs[4];

//        ARtoolkit�̏����ݒ�

	// ARToolKitPlus��
	ARToolKitPlus::TrackerMultiMarker *tracker;

	// tracker �����
	tracker = new ARToolKitPlus::TrackerMultiMarkerImpl<6,6,12,32,64>(WIDTH, HEIGHT);
			// <marker_width, marker_height, sampling step, max num in cfg, max num in scene>
	tracker->init("calibration_data.cal", "marker_dummy.cfg", NEAR_LEN, FAR_LEN);

	// tracker �I�u�W�F�N�g�̊e��ݒ�i�ʏ킱�̂܂܂ł悢�j
	tracker->setPixelFormat(ARToolKitPlus::PIXEL_FORMAT_BGR);
	tracker->setBorderWidth(0.125f); // thin
	tracker->activateAutoThreshold(true);
	tracker->setUndistortionMode(ARToolKitPlus::UNDIST_NONE);
	tracker->setPoseEstimator(ARToolKitPlus::POSE_ESTIMATOR_RPP);
	tracker->setMarkerMode(ARToolKitPlus::MARKER_ID_BCH);

	// �}�[�J���W�n����J�������W�n�ւ̕ϊ��s����i�[���邽�߂̔z��
	//float m34[ 3 ][ 4 ];
	// �}�[�J���S�̃I�t�Z�b�g�i�ʏ킱�̂܂܂ł悢�j
	float c[ 2 ] = { 0.0f, 0.0f };
	// �}�[�J�̈�ӂ̃T�C�Y [mm] �i�����l�j
	float w = 59.0f;
	cv::Point2f corner[4];
	cv::Point2f outer[4];
	int radius = 5; // �L�[�|�C���g�̔��a
	cv::Point2f pt_cp[4];

//         �z���O���t�B�ϊ��̏����ݒ�

	//P���W�ݒ�
	Point2f ex_pt_pi[4];
	Point2f ex2_pt_pi[4];
	Point2f ex3_pt_pi[4];
	Point2f pt_pi[4];
	Point2f pt_po[4];

	pt_po[0] = cv::Point2f(1,1);
	pt_po[1] = cv::Point2f(1,720);
	pt_po[2] = cv::Point2f(1280,720);
	pt_po[3] = cv::Point2f(1280,1);

	pt_pi[0] = cv::Point2f(565.0f,285.0f);
	pt_pi[1] = cv::Point2f(565.0f,435.0f);
	pt_pi[2] = cv::Point2f(715.0f,435.0f);
	pt_pi[3] = cv::Point2f(715.0f,285.0f);
	//I���W�ݒ�
	//���g
	const cv::Point2f pt_ii[] = {cv::Point2f(406,206),cv::Point2f(406,306),cv::Point2f(506,306),
		cv::Point2f(506,206)};
	//�O�g	
	const cv::Point2f pt_io[] = {cv::Point2f(1,1),cv::Point2f(1,912),cv::Point2f(513,912),
		cv::Point2f(513,1)};

	//S���W�ݒ�
	cv::Point2f pt_si[4];
	cv::Point2f pt_so[4];

	pt_so[0] = cv::Point2f(1,1);
	pt_so[1] = cv::Point2f(1,720);
	pt_so[2] = cv::Point2f(1280,720);
	pt_so[3] = cv::Point2f(1280,1);

	pt_si[0] = cv::Point2f(390.0f,110.0f);
	pt_si[1] = cv::Point2f(390.0f,610.0f);
	pt_si[2] = cv::Point2f(890.0f,610.0f);
	pt_si[3] = cv::Point2f(890.0f,110.0f);

	//A4�Ɏʂ��p
	//pt_si[0] = cv::Point2f(112,72);
	//pt_si[1] = cv::Point2f(112,147);
	//pt_si[2] = cv::Point2f(187,147);
	//pt_si[3] = cv::Point2f(187,72);

	//pt_so[0] = cv::Point2f(0,0);
	//pt_so[1] = cv::Point2f(0,210);
	//pt_so[2] = cv::Point2f(297,210);
	//pt_so[3] = cv::Point2f(297,0);

	//�e���v���[�gT���W�ݒ�
	const Point2f pt_t[] = {cv::Point2f(0,0),cv::Point2f(0,47),cv::Point2f(47,47),
		cv::Point2f(47,0)};

	//I����S
	cv::Mat iHsi = cv::getPerspectiveTransform( pt_ii, pt_si);
	iHsi.convertTo(iHsi, CV_32F);
	//T����I
	cv::Mat tHi = cv::getPerspectiveTransform( pt_t, pt_ii);
	tHi.convertTo(tHi, CV_32F);
	//iHp_origin
	cv::Mat iHp_origin = cv::getPerspectiveTransform( pt_ii, pt_pi);
	iHp_origin.convertTo(iHp_origin, CV_32F);

	int temp=0;
	int frame=0;
	const int check=6;
	int flag;
	Mat warp;
	vector<Point2f> p_corner(4);
	cv::Mat iHp = cv::Mat(3,3,CV_32F);
	cv::Mat iHp_old = cv::Mat(3,3,CV_32F);
	cv::Mat soHcs = cv::Mat(3,3,CV_32F);
	cv::Mat cpHp = cv::Mat(3,3,CV_32F);
	iHp_origin.copyTo(iHp_old);

/////////////////////////     ESM     //////////////////////

	//�e���v���[�g�̐ݒ�
	cv::Mat tpl = cv::imread(TEMPLATE);
	cvtColor(tpl, tpl, CV_RGB2GRAY);
	cv::Rect tpl_rect;
	tpl_rect.x = 0;
	tpl_rect.y = 0;
	tpl_rect.width = 480;
	tpl_rect.height = 480;
	//�T���͈͂̐ݒ�
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

	while(1) {

		cap >> src;  // �L���v�`��

		src.copyTo(disp);
		//�O���[�X�P�[���ϊ�
		cv::cvtColor(disp, image, CV_BGR2GRAY);

////////////////////////////////////   �֊s���o   //////////////////////////////// 

		//2�l��
		cv::threshold(image, bin, 100, 255, cv::THRESH_BINARY|cv::THRESH_OTSU); 

		//���k�E�c��
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3); 
		cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 3);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
		cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

		//�֊s���o
		cv::findContours (bin, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		//cv::drawContours (src, contours, -1, cv::Scalar(100), 2, 8);
		//���o�����֊s���ƂɌ��ĉ��
		for (unsigned int j = 0; j < contours.size(); j++){
		approx = contours[j];
		//�֊s���ߎ�����
		cv::approxPolyDP(contours[j], approx, cv::arcLength(contours[j], true)*0.02, true);
			//���_��4�̏ꍇ
			if (approx.size() == 4 && hierarchy[j][2] != -1){
				//4�̒��_��`��
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5*k,  CV_RGB(255,0,0), 2, 8, 0);
				}
			//�O�g�擾�p
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
		
/////////////////////////   ARtoolkit     ///////////////////////////

		// �摜������}�[�J�����o
		tracker->calc((unsigned char *)(src.data));
		
		// ���o���ꂽ�}�[�J�̐�
		unsigned int num = tracker->getNumDetectedMarkers();

		// ���o���ꂽ�e�}�[�J�������Ă���
		for(unsigned int m = 0; m < num; m++){

			// i �Ԗڂ̃}�[�J�����擾�iid �~���ɕ��ׂ��Ă���l�q�j
			ARToolKitPlus::ARMarkerInfo a = tracker->getDetectedMarker(m);

			//�擾�������W
			if(a.id==1){
			corner[0].x = a.vertex[0][0];//�E��
			corner[0].y = a.vertex[0][1];
			corner[1].x = a.vertex[1][0];
			corner[1].y = a.vertex[1][1];
			corner[2].x = a.vertex[2][0];
			corner[2].y = a.vertex[2][1];
			corner[3].x = a.vertex[3][0];
			corner[3].y = a.vertex[3][1];
			}
		}
		//4�_��`��
		for(int l = 0; l < 4; l++){
			cv::circle(src, corner[l], radius*l,  CV_RGB(0,0,255), 5, 8, 0);
		}
		//���g�擾�p
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



//////////////////////////    ESM    /////////////////////////////

        if (!esm_tracker.empty()) {
            G = esm_tracker.track(image, G);
        }

        if (frame>45) {
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

//////////////////////////�z���O���t�B�ϊ�//////////////////////

		//3�t���[���O��pt_pi���g��
		for(int n = 0; n < 4; n++){
		ex3_pt_pi[n] = ex2_pt_pi[n];
		}
		for(int n = 0; n < 4; n++){
		ex2_pt_pi[n] = ex_pt_pi[n];
		}
		for(int n = 0; n < 4; n++){
		ex_pt_pi[n] = pt_pi[n];
		}
		//�z���O���t�B�s��쐬
		//S����C
		soHcs = cv::getPerspectiveTransform( pt_so, pt_cs);
		soHcs.convertTo(soHcs, CV_32F);
		//C����P
		cpHp = cv::getPerspectiveTransform( pt_cp, ex3_pt_pi);
		cpHp.convertTo(cpHp, CV_32F);

		//�t���[��
		frame++;
		/*std::cout << "frame=" << frame << std::endl;*/
		flag = frame%check;

		//I����P�֓����ϊ�
		if(frame>20&&frame<40){
		iHp = cpHp * soHcs * iHsi;
		iHp.copyTo(iHp_old);
		}

		if(frame>=50&&flag==0){
		iHp = iHp_old * tHi * G.inv() * soHcs * iHsi;
		iHp.copyTo(iHp_old);
		}

		if(frame<21){
		iHp_origin = cv::getPerspectiveTransform( pt_ii, pt_pi);
		cv::warpPerspective( proj, warp, iHp_origin, proj_fit.size());
		}

		//frame>10����g���b�L���O
		if(frame>20&&frame<40)
		{
			cv::warpPerspective( proj, warp, iHp, proj_fit.size());
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
		//frame>10����g���b�L���O
		if(frame>=40&&frame<50)
		{
			cv::warpPerspective( cont, warp, iHp, proj_fit.size());
			/*cout << iHp << endl;*/
		}
		//frame>10����g���b�L���O
		if(frame>=50)
		{
			cv::warpPerspective( cont, warp, iHp, proj_fit.size());
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

        if (frame==45) {
		srh.x = pt_cp[0].x;
		srh.y = pt_cp[0].y;
		srh.width = pt_cp[3].x - pt_cp[0].x;
		srh.height = pt_cp[1].y - pt_cp[0].y;
		G = esm_tracker.init(tpl, srh, Size(48, 48));
            esm_tracker.copyTargetTo(target);
            mparam.init_requested = false;
        }
		//�\��
		cv::imshow("projection", warp);
		cv::imshow("Capture", src);
		//cv::imshow("bin", bin);

		if (waitKey(2) > 0) {
            break;
        }

		//debug�p
		temp++;
 		//std::cout << temp << std::endl;
		if(temp==1000){
		break;
		}


		}//���[�v�I���

		return 0;
}