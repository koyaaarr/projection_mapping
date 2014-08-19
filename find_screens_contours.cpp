#include "headers.h"

using namespace std;
using namespace cv;

int main( int argc, char** argv )
{
    //cv::Mat src;
	vector< vector<cv::Point> > contours;
	//vector<cv::vector<cv::Point>> squares;
	//vector<cv::vector<cv::Point> > poly;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> approx;
	cv::Point2f pt_cs[4];
	const int idx=-1;
	const int thick=2;

	//カメラオープン
	cv::VideoCapture cap(1);
    if(!cap.isOpened())
        return -1;
    cv::namedWindow("contours",1);

	while(1)
	{
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
			if (approx.size() == 4 && hierarchy[j][2] != -1){
				//4つの頂点を描く
				for (unsigned int k = 0; k < approx.size(); k++){
					cv::circle(src, approx[k], 5,  CV_RGB(255,0,0), 2, 8, 0);
				}
				//外枠取得用
				pt_cs[0] = cv::Point(approx[0].x, approx[0].y);
				pt_cs[1] = cv::Point(approx[1].x, approx[1].y);
				pt_cs[2] = cv::Point(approx[2].x, approx[2].y);
				pt_cs[3] = cv::Point(approx[3].x, approx[3].y);
			}
		}
		// 表示、キー入力で終了
		imshow("contours", src);
		//imshow("gray", gray);
		//imshow("bin", bin);
		if (waitKey(2) > 0) 
		{
            break;
        }
	}

    return 0;
}