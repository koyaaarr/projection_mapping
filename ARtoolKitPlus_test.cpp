#include "headers.h"

// tracker �ɓn���摜�̃T�C�Y
#define WIDTH	640
#define HEIGHT	480

// �߉��N���b�s���O�̋����i�ʏ킱�̂܂܂ł悢�j
#define NEAR_LEN	1.0
#define FAR_LEN		10000.0

using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	//// OpenCV��
	cv::VideoCapture cap(0);
    if(!cap.isOpened())
        return -1;

	//cv::Mat frame, img;//20140807 while���ֈړ�
	namedWindow("camera");


	//// ARToolKitPlus��
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
	float m34[ 3 ][ 4 ];
	// �}�[�J���S�̃I�t�Z�b�g�i�ʏ킱�̂܂܂ł悢�j
	float c[ 2 ] = { 0.0f, 0.0f };
	// �}�[�J�̈�ӂ̃T�C�Y [mm] �i�����l�j
	float w = 59.0f;

	int temp=0;
	int error=0;

	while(1)
	{
		Mat frame, img;
		cap >> frame;
		frame.copyTo(img);

		// �J�����摜��10��擾�ł��Ȃ�������I���
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

		 /////�摜������}�[�J�����o
		tracker->calc((unsigned char *)(img.data));
		
		// ���o���ꂽ�}�[�J�̐�
		int num = tracker->getNumDetectedMarkers();

		// ���o���ꂽ�e�}�[�J�������Ă���
		for(int i = 0; i < num; i++)
		{
			// i �Ԗڂ̃}�[�J�����擾�iid �~���ɕ��ׂ��Ă���l�q�j
			ARToolKitPlus::ARMarkerInfo a = tracker->getDetectedMarker(i);

			//�擾�������W���I�[�o�[���C
			cv::Point2i center; // �L�[�|�C���g�̒��S���W
			int radius;   // �L�[�|�C���g�̔��a
			center.x = a.vertex[0][0];
			center.y = a.vertex[0][1];
			radius = 20.0;
			cv::circle(img, center, radius, cv::Scalar(255,0,0), 10, 8, 0);

			// id, �摜���̃s�N�Z���ʒu��\���iARMarkerInfo �\���̂̃����o�ɂ͂����Ă�j
			printf("id = %d, pos = (%f, %f)\n", a.id, a.vertex[0][0], a.vertex[0][1]);
		
			// �}�[�J���W�n����J�������W�n�ւ̕ϊ��s������߂�
			// 3*4 �s��ł��邱�Ƃɒ��ӁA4 ��ڂ͕��i�x�N�g�� [mm]
//			tracker->rppGetTransMat( &a, c, w, m34 );

			// ���߂��ϊ��s���\��
/*			for(int row = 0; row < 3; row++){
			for(int col = 0; col < 4; col++){
				printf("%f, ", m34[row][col]);
			}
			printf("\n");
			}
			printf("\n");
*/		}

		cv::imshow("camera", img);//�o��
		if (waitKey(2) > 0) 
		{
            break;
        }
		//temp++;
	}
	return 0;
}