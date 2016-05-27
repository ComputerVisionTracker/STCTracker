#include "STCTracker.h"
#include "VideoBean.h"
#include <windows.h>
#include <vector>

using namespace cv;
using namespace std;

vector<VideoBean> mVideoData;


//当前打开的视频序号
int mVideoNumber = 5;

//跟踪的矩形框
Rect mBox;
//是否正在绘制矩形框
bool mDrawingBox = false;
//是否得到矩形框
bool isGotBB = false;

void initVideoData();

ofstream out;
void openStream(int mVideoIndex);
void closeStream();


void selectOneToCalculateRect();
void calculateFPS();
double calculateSingleFPS(int mIndex);

//得到鼠标圈中的跟踪框的鼠标事件回调
void mouseHandler(int event, int x, int y, int flags, void *param){
	switch (event){
	case CV_EVENT_MOUSEMOVE:
		if (mDrawingBox){
			mBox.width = x - mBox.x;
			mBox.height = y - mBox.y;
		}
		break;
	case CV_EVENT_LBUTTONDOWN:
		mDrawingBox = true;
		mBox.x = x;
		mBox.y = y;
		mBox.width = 0;
		mBox.height = 0;
		break;
	case CV_EVENT_LBUTTONUP:
		mDrawingBox = false;
		if (mBox.width < 0){
			mBox.x += mBox.width;
			mBox.width *= -1;
		}
		if (mBox.height < 0){
			mBox.y += mBox.height;
			mBox.height *= -1;
		}

		mBox.x = mVideoData[mVideoNumber].x;
		mBox.y = mVideoData[mVideoNumber].y;
		mBox.width = mVideoData[mVideoNumber].width;
		mBox.height = mVideoData[mVideoNumber].height;

		isGotBB = true;
		break;
	}
}

void readOptions(int argc, char ** argv, VideoCapture &mVideo)
{
	if (argc == 2)
	{
		mVideoNumber = atoi(argv[1]);
	}

	mVideo.open(mVideoData[mVideoNumber].mVideoPath);

}

void openVideo(VideoCapture& mVideo)
{
	cout << "Please input a video index:" << endl;
	cin >> mVideoNumber;
	while (mVideoNumber < 0 || mVideoNumber > mVideoData.size())
	{
		cout << "Please input a legal video index:" << endl;
		cin >> mVideoNumber;
	}

	mVideo.open(mVideoData[mVideoNumber].mVideoPath);

}

void main(int argc, char * argv[])
{
	selectOneToCalculateRect();

	//calculateFPS();

	system("pause");
}

void selectOneToCalculateRect()
{

	mDrawingBox = false;
	isGotBB = false;

	initVideoData();

	VideoCapture mVideo;

	openVideo(mVideo);

	if (!mVideo.isOpened()){
		cout << "capture device failed to open!" << endl;
		system("pause");
		return ;
	}

	openStream(mVideoNumber);
	cvNamedWindow("Tracker", CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback("Tracker", mouseHandler, NULL);

	//分别为得到的当前帧，当前帧的副本，当前帧对应的灰度图像帧
	Mat mFrame, mTempFrame, mGrayFrame;

	mVideo >> mFrame;
	cvtColor(mFrame, mGrayFrame, CV_RGB2GRAY);
	mFrame.copyTo(mTempFrame);

	while (!isGotBB)
	{
		mTempFrame.copyTo(mFrame);

		rectangle(mFrame, mBox, Scalar(0, 0, 255));
		imshow("Tracker", mFrame);
		if (cvWaitKey(20) == 27){
			closeStream();
			return ;
		}
	}

	cvSetMouseCallback("Tracker", NULL, NULL);

	STCTracker mTracker;
	mTracker.init(mGrayFrame, mBox, (int)mVideo.get(CV_CAP_PROP_FRAME_COUNT));
	out << mBox.x << "," << mBox.y << "," << mBox.width << "," << mBox.height << endl;

	//分别记录处理的帧号和开始处理的毫秒数
	int mFrameNum = 1;
	double mTick;
	stringstream mBuf;

	mTick = (double)getTickCount();

	while (true)
	{
		mVideo >> mFrame;
		if (mFrame.empty())
		{
			break;
		}
		cvtColor(mFrame, mGrayFrame, CV_RGB2GRAY);
		mFrameNum++;

		mTracker.track(mGrayFrame, mBox);

		mBuf.str("");
		mBuf << mFrameNum;

		putText(mFrame, mBuf.str(), Point(20, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
		rectangle(mFrame, mBox, Scalar(0, 0, 255), 2);
		imshow("Tracker", mFrame);
		out << mBox.x << "," << mBox.y << "," << mBox.width << "," << mBox.height << endl;

		if (cvWaitKey(1) == 27)
			break;

	}

	double time = (getTickCount() - mTick) / getTickFrequency();
	double fps = (mFrameNum - 1) / time;
	cout << fps << endl;

	closeStream();
	system("pause");
	return ;
}

void calculateFPS()
{
	initVideoData();

	out.open("fps.txt", ios::out);

	for (int i = 0; i <= 50; i++)
	{
		double fps = 0;
		for (int j = 0; j < 3; j++)
		{
			fps += calculateSingleFPS(i);
		}
		fps = fps / 3;
		cout << i << "--->" << fps << endl;
		out << i << "--->" << fps << endl;
	}

	out.flush();
	out.close();

	system("pause");
	return ;
}

double calculateSingleFPS(int mIndex)
{
	VideoCapture mVideo;

	mVideo.open(mVideoData[mIndex].mVideoPath);

	cvNamedWindow("Tracker", CV_WINDOW_AUTOSIZE);

	Mat mFrame, mTempFrame, mGrayFrame;
	mVideo >> mFrame;
	cvtColor(mFrame, mGrayFrame, CV_RGB2GRAY);
	imshow("Tracker", mFrame);

	mBox.x = mVideoData[mIndex].x;
	mBox.y = mVideoData[mIndex].y;
	mBox.width = mVideoData[mIndex].width;
	mBox.height = mVideoData[mIndex].height;

	STCTracker mTracker;
	mTracker.init(mGrayFrame, mBox, (int)mVideo.get(CV_CAP_PROP_FRAME_COUNT));

	int mFrameCount = 0;
	double mTick = getTickCount();
	stringstream mBuf;

	while (true)
	{
		mVideo >> mFrame;
		if (mFrame.empty()){
			break;
		}
		cvtColor(mFrame, mGrayFrame, CV_RGB2GRAY);
		mFrameCount++;

		mTracker.track(mGrayFrame, mBox);

		mBuf.str("");
		mBuf << mFrameCount;

		putText(mFrame, mBuf.str(), Point(20, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
		rectangle(mFrame, mBox, Scalar(0, 0, 255), 2);
		imshow("Tracker", mFrame);

		if (cvWaitKey(1) == 27)
			break;

	}

	double time = (getTickCount() - mTick) / getTickFrequency();
	double fps = mFrameCount / time;

	return fps;
}

void initVideoData()
{
	VideoBean mBean;
	mBean.set("D:\\OpenCVData\\Basketball.avi", 198, 214, 34, 81, "Basketball");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Bolt.avi", 336, 165, 26, 61, "Bolt");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Boy.avi", 288, 143, 35, 42, "Boy");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Car4.avi", 70, 51, 107, 87, "Car4");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\CarDark.avi", 73, 126, 29, 23, "CarDark");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\CarScale.avi", 6, 166, 42, 26, "CarScale");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Coke.avi", 298, 160, 48, 80, "Coke");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Couple.avi", 51, 47, 25, 62, "Couple");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Crossing.avi", 205, 151, 17, 50, "Crossing");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\David.avi", 161, 65, 75, 95, "David");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\David2.avi", 141, 73, 27, 34, "David2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\David3.avi", 83, 200, 35, 131, "David3");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Deer.avi", 306, 5, 95, 65, "Deer");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Dog1.avi", 139, 112, 51, 36, "Dog1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Doll.avi", 146, 150, 32, 73, "Doll");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Dudek.avi", 123, 87, 132, 176, "Dudek");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\FaceOcc1.avi", 118, 69, 114, 162, "FaceOcc1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\FaceOcc2.avi", 118, 57, 82, 98, "FaceOcc2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Fish.avi", 134, 55, 60, 88, "Fish");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\FleetFace.avi", 405, 256, 122, 148, "FleetFace");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Football.avi", 310, 102, 39, 50, "Football");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Football1.avi", 153, 105, 26, 43, "Football1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Freeman1.avi", 253, 66, 23, 28, "Freeman1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Freeman3.avi", 245, 64, 12, 13, "Freeman3");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Freeman4.avi", 125, 86, 15, 16, "Freeman4");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Girl.avi", 57, 21, 31, 45, "Girl");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Ironman.avi", 206, 85, 49, 57, "Ironman");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Jogging.avi", 111, 98, 25, 101, "Jogging1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Jogging.avi", 180, 79, 37, 114, "Jogging2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Jumping.avi", 147, 110, 34, 33, "Jumping");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Lemming.avi", 40, 199, 61, 103, "Lemming");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Liquor.avi", 256, 152, 73, 210, "Liquor");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Matrix.avi", 331, 39, 38, 42, "Matrix");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Mhyang.avi", 84, 53, 62, 70, "Mhyang");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\MotorRolling.avi", 117, 68, 122, 125, "MotorRolling");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\MountainBike.avi", 319, 185, 67, 56, "MountainBike");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Shaking.avi", 225, 135, 61, 71, "Shaking");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Singer1.avi", 51, 53, 87, 290, "Singer1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Singer2.avi", 298, 149, 67, 122, "Singer2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Skating1.avi", 162, 188, 34, 84, "Skating1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Skiing.avi", 446, 181, 29, 26, "Skiing");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Soccer.avi", 302, 135, 67, 81, "Soccer");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Subway.avi", 16, 88, 19, 51, "Subway");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Suv.avi", 142, 125, 91, 40, "Suv");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Sylvester.avi", 122, 51, 51, 61, "Sylvester");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Tiger1.avi", 232, 88, 76, 84, "Tiger1");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Tiger2.avi", 32, 60, 68, 78, "Tiger2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Trellis.avi", 146, 54, 68, 101, "Trellis");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Walking.avi", 692, 439, 24, 79, "Walking");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Walking2.avi", 130, 132, 31, 115, "Walking2");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\Woman.avi", 213, 121, 21, 95, "Woman");
	mVideoData.push_back(mBean);
	mBean.set("D:\\OpenCVData\\soccer(2).avi", 213, 121, 21, 95, "soccer");
	mVideoData.push_back(mBean);
}

void openStream(int mVideoIndex)
{
	out.open(mVideoData[mVideoIndex].name + ".txt", ios::out);
}

void closeStream()
{
	if (out.is_open())
	{
		out.flush();
		out.close();
	}
}
