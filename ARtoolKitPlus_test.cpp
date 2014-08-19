#include "headers.h"

// tracker に渡す画像のサイズ
#define WIDTH	640
#define HEIGHT	480

// 近遠クリッピングの距離（通常このままでよい）
#define NEAR_LEN	1.0
#define FAR_LEN		10000.0

using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	//// OpenCV側
	cv::VideoCapture cap(0);
    if(!cap.isOpened())
        return -1;

	//cv::Mat frame, img;//20140807 while中へ移動
	namedWindow("camera");


	//// ARToolKitPlus側
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
	float m34[ 3 ][ 4 ];
	// マーカ中心のオフセット（通常このままでよい）
	float c[ 2 ] = { 0.0f, 0.0f };
	// マーカの一辺のサイズ [mm] （実測値）
	float w = 59.0f;

	int temp=0;
	int error=0;

	while(1)
	{
		Mat frame, img;
		cap >> frame;
		frame.copyTo(img);

		// カメラ画像が10回取得できなかったら終わり
		if(error == 10)
		{
			break;
		}

		if(img.empty())
		{
			cout << "empty" << endl;
			error++;
			continue;
		}

		 /////画像中からマーカを検出
		tracker->calc((unsigned char *)(img.data));
		
		// 検出されたマーカの数
		int num = tracker->getNumDetectedMarkers();

		// 検出された各マーカ情報を見ていく
		for(int i = 0; i < num; i++)
		{
			// i 番目のマーカ情報を取得（id 降順に並べられている様子）
			ARToolKitPlus::ARMarkerInfo a = tracker->getDetectedMarker(i);

			//取得した座標をオーバーレイ
			cv::Point2i center; // キーポイントの中心座標
			int radius;   // キーポイントの半径
			center.x = a.vertex[0][0];
			center.y = a.vertex[0][1];
			radius = 20.0;
			cv::circle(img, center, radius, cv::Scalar(255,0,0), 10, 8, 0);

			// id, 画像中のピクセル位置を表示（ARMarkerInfo 構造体のメンバにはいってる）
			printf("id = %d, pos = (%f, %f)\n", a.id, a.vertex[0][0], a.vertex[0][1]);
		
			// マーカ座標系からカメラ座標系への変換行列を求める
			// 3*4 行列であることに注意、4 列目は並進ベクトル [mm]
//			tracker->rppGetTransMat( &a, c, w, m34 );

			// 求めた変換行列を表示
/*			for(int row = 0; row < 3; row++){
			for(int col = 0; col < 4; col++){
				printf("%f, ", m34[row][col]);
			}
			printf("\n");
			}
			printf("\n");
*/		}

		cv::imshow("camera", img);//出張
		if (waitKey(2) > 0) 
		{
            break;
        }
		//temp++;
	}
	return 0;
}