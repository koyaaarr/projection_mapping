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


int main( int argc, char** argv )
{
    //cv::Mat src;

	cv::VideoCapture cap(1);
	if(!cap.isOpened()){
		cout << "error:camera not found" << endl;
		return -1;
	}
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	//cv::namedWindow("Capture", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);


	int loop=0;

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

		Mat src;
		cap >> src;

		if(src.empty())
		{
			cout << "error" << endl;
		}

		while(1){
			int key;
			if ((key = cv::waitKey(10)) > 0) {
				if (key == 'q' || key == 0x1b) {
					break;
				}
			}
		}

		std::ostringstream os;
		os << loop;
		std::string number = os.str();
		cv::putText(proj, number , cv::Point(200,500), cv::FONT_HERSHEY_SIMPLEX,
					10.0, cv::Scalar(255,0,255), 10, CV_AA);
		loop++;

		cout << "loop=" << loop << endl;
		cv::imshow( "projection", proj);
		cv::imshow("capture", src);

		if (waitKey(2) > 0) 
		{
            break;
        }
	}

    return 0;
}