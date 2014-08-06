#include "headers.h"

using namespace std;
using namespace cv;

int main( int argc, char** argv )
{
    cv::Mat src;

	cv::VideoCapture cap(0); // �f�t�H���g�J�������I�[�v��
    if(!cap.isOpened())  // �����������ǂ������`�F�b�N
        return -1;
    cv::namedWindow("window",1);

	while(1)
	{
		cap >> src;

		cv::imshow( "window", src );                   // Show our image inside it.
		if (waitKey(2) > 0) 
		{
            break;
        }
	}

    return 0;
}