#include "headers.h"

using namespace std;
using namespace cv;

int main( int argc, char** argv )
{
    //cv::Mat src;

	cv::VideoCapture cap(0);
    if(!cap.isOpened())
        return -1;
    cv::namedWindow("window",1);

	while(1)
	{
		Mat src;
		cap >> src;

		if(!src.empty())
		{
			cv::imshow( "window", src );
		}
		else
		{
			cout << "error" << endl;
		}
		if (waitKey(2) > 0) 
		{
            break;
        }
	}

    return 0;
}