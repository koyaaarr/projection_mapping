



////// Fullscreen.cppで使う
#define _WIN32_WINNT 0x0500 // must be before windows.h (only needed you use a transparent window)

////// windowsのインクルード


#include <vector>
#include <iostream>
#include <windows.h>
#include <stdio.h>


/////// OpenCVのインクルード

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


/////// ARToolKitPlusのインクルード


#include    <ARToolKitPlus/TrackerMultiMarkerImpl.h>
//#pragma comment(lib, "ARToolKitPlusd.lib")

// tracker に渡す画像のサイズ
#define WIDTH	640
#define HEIGHT	480

// 近遠クリッピングの距離（通常このままでよい）
#define NEAR_LEN	1.0
#define FAR_LEN		10000.0


////////  自前関数のインクルード
#include "stattimer.hpp"


///////// ESM
#include "NewTrackerESM01.hpp"
#pragma comment(lib,"user32.lib")