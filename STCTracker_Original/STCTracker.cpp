#include "STCTracker.h"


STCTracker::STCTracker()
{
	out.open("STC_Result.txt", ios::out);

	mFrameIndex = 0;
	mIntervalNumber = 5;
	alpha = 2.25;
	beta = 1;
	mLearnRate = 0.075;
	mScale = 1.0;
	mScaleLearnRate = 0.25;

	mEachFrameConfd = NULL;
	mCxtX = NULL;
	mCxtY = NULL;

	mMinConfiPosi = 1;

}


STCTracker::~STCTracker()
{
	out.flush();
	out.close();
	if (mEachFrameConfd != NULL)
	{
		delete[] mEachFrameConfd;
	}
	if (mCxtX != NULL)
	{
		delete[] mCxtX;
	}
	if (mCxtY != NULL)
	{
		delete[] mCxtY;
	}
}


void STCTracker::createHammingWindow()
{
	int mCols = mHammingWinMat.cols;
	int mRows = mHammingWinMat.rows;

	double *mHammingValue = new double[mRows];
	double *mHanningValue = new double[mCols];

	for (int i = 0; i < mRows; i++)
	{
		mHammingValue[i] = 0.54 - 0.46*cos(2 * CV_PI*i / (mRows - 1));
	}

	for (int i = 0; i < mCols; i++)
	{
		mHanningValue[i] = 0.5 - 0.5*cos(2 * CV_PI*i / (mCols - 1));
	}
	
	double *mData = NULL;

	for (int i = 0; i < mRows; i++)
	{
		mData = (double *)(mHammingWinMat.data + i * mHammingWinMat.step);
		for (int j = 0; j < mCols; j++)
		{
			mData[j] = mHanningValue[j] * mHammingValue[i];
		}

	}

	delete[] mHammingValue;
	delete[] mHanningValue;
}

void STCTracker::createEucliDistMatAndCxtPostPro()
{
	double *mXArray, *mYArray, *mXTempPtr, *mYTempPtr;

	mXArray = new double[mCxtRegionSize.width];
	mYArray = new double[mCxtRegionSize.height];

	double mXMiddle = floor((mCxtRegionSize.width - 1 + 0.000001) / 2);
	double mYMiddle = floor((mCxtRegionSize.height - 1 + 0.000001) / 2);

	mXTempPtr = mXArray;
	for (int i = 0; i < mCxtRegionSize.width; i++)
	{
		*mXTempPtr = (i - mXMiddle);
		mXTempPtr++;
	}

	mYTempPtr = mYArray;
	for (int i = 0; i < mCxtRegionSize.height; i++)
	{
		*mYTempPtr = (i - mYMiddle);
		mYTempPtr++;
	}
	double *mEucData = NULL;
	double *mCxtPostProData = NULL;
	double mPostSum = 0;
	for (int i = 0; i < mCxtRegionSize.height; i++)
	{
		mEucData = (double *)(mEucliDistMat.data + mEucliDistMat.step * i);
		mCxtPostProData = (double *)(mCxtPostProMat.data + mCxtPostProMat.step * i);

		mXTempPtr = mXArray;
		mYTempPtr = mYArray + i;
		for (int j = 0; j < mCxtRegionSize.width; j++)
		{
			mEucData[j] = *mYTempPtr * *mYTempPtr + *mXTempPtr * *mXTempPtr;
			mXTempPtr++;

			mCxtPostProData[j] = exp(-pow(0.5 * sqrt(mEucData[j]) / alpha, beta));
			mPostSum += mCxtPostProData[j];
		}
	}
	mCxtPostProMat.convertTo(mCxtPostProMat, -1, 1.0 / mPostSum);
	
	//预先计算出后验概率，即置信度
	Mat planes1[] = { mCxtPostProMat, Mat::zeros(mCxtPostProMat.size(), CV_64FC1) };
	merge(planes1, 2, mCxtPostProFFTMat);
	dft(mCxtPostProFFTMat, mCxtPostProFFTMat);

	delete[] mXArray;
	delete[] mYArray;
}

void STCTracker::complexMultiply(const Mat& src1, const Mat& src2, Mat& dst)
{
	Mat *mAReal, *mAImag, *mBReal, *mBImag, *mRReal, *mRImag;

	vector<Mat> mAPlanes;
	split(src1, mAPlanes);
	mAReal = &mAPlanes[0];
	mAImag = &mAPlanes[1];

	vector<Mat> mBPlanes;
	split(src2, mBPlanes);
	mBReal = &mBPlanes[0];
	mBImag = &mBPlanes[1];

	dst.create(src1.rows, src1.cols, CV_64FC2);
	vector<Mat> mRPlanes;
	split(dst, mRPlanes);
	mRReal = &mRPlanes[0];
	mRImag = &mRPlanes[1];

	double *mARealData, *mAImagData, *mBRealData, *mBImagData, *mRRealData, *mRImagData;

	for (int i = 0; i < src1.rows; i++)
	{
		mARealData = (double *)(mAReal->data + mAReal->step * i);
		mAImagData = (double *)(mAImag->data + mAImag->step * i);
		mBRealData = (double *)(mBReal->data + mBReal->step * i);
		mBImagData = (double *)(mBImag->data + mBImag->step * i);
		mRRealData = (double *)(mRReal->data + mRReal->step * i);
		mRImagData = (double *)(mRImag->data + mRImag->step * i);

		for (int j = 0; j < src1.cols; j++)
		{
			mRRealData[j] = mARealData[j] * mBRealData[j] - mAImagData[j] * mBImagData[j];
			mRImagData[j] = mARealData[j] * mBImagData[j] + mAImagData[j] * mBRealData[j];
		}
	
	}
	merge(mRPlanes, dst);
}


void STCTracker::complexDivide(const Mat& src1, const Mat& src2, Mat& dst)
{
	Mat *mAReal, *mAImag, *mBReal, *mBImag, *mRReal, *mRImag;

	vector<Mat> mAPlanes;
	split(src1, mAPlanes);
	mAReal = &mAPlanes[0];
	mAImag = &mAPlanes[1];

	vector<Mat> mBPlanes;
	split(src2, mBPlanes);
	mBReal = &mBPlanes[0];
	mBImag = &mBPlanes[1];

	dst.create(src1.rows, src1.cols, CV_64FC2);
	vector<Mat> mRPlanes;
	split(dst, mRPlanes);
	mRReal = &mRPlanes[0];
	mRImag = &mRPlanes[1];

	double *mARealData, *mAImagData, *mBRealData, *mBImagData, *mRRealData, *mRImagData;

	for (int i = 0; i < src1.rows; i++)
	{
		mARealData = (double *)(mAReal->data + mAReal->step * i);
		mAImagData = (double *)(mAImag->data + mAImag->step * i);
		mBRealData = (double *)(mBReal->data + mBReal->step * i);
		mBImagData = (double *)(mBImag->data + mBImag->step * i);
		mRRealData = (double *)(mRReal->data + mRReal->step * i);
		mRImagData = (double *)(mRImag->data + mRImag->step * i);

		for (int j = 0; j < src1.cols; j++)
		{
			mRRealData[j] = (mARealData[j] * mBRealData[j] + mAImagData[j] * mBImagData[j]) / (mBRealData[j] * mBRealData[j] + mBImagData[j] * mBImagData[j]);
			mRImagData[j] = (mAImagData[j] * mBRealData[j] - mARealData[j] * mBImagData[j]) / (mBRealData[j] * mBRealData[j] + mBImagData[j] * mBImagData[j]);
		}

	}
	merge(mRPlanes, dst);
}

/*
 * 因为sigma每帧都在变化，所以滤波窗每一帧都需计算
 */
void STCTracker::getFilterWindow()
{
	double *mFilterData = NULL, *mEucliData = NULL, *mHammData = NULL, mSum = 0;

	for (int i = 0; i < mCxtRegionSize.height; i++)
	{
		mFilterData = (double *)(mFilterWindowMat.data + mFilterWindowMat.step * i);
		mEucliData = (double *)(mEucliDistMat.data + mEucliDistMat.step * i);
		mHammData = (double *)(mHammingWinMat.data + mHammingWinMat.step * i);
		for (int j = 0; j < mCxtRegionSize.width; j++)
		{
			mFilterData[j] = mHammData[j] * exp(-mEucliData[j] / (2 * sigma * sigma));
			mSum += mFilterData[j];
		}

	}
	mFilterWindowMat.convertTo(mFilterWindowMat, -1, 1 / mSum);

}

/************************************************************************/
/*	首先需要考虑边界问题，计算出需要裁减的局部空间上下文对应横纵坐标值，然后裁减并规范化
/************************************************************************/
void STCTracker::getContext(const Mat& mFrame)
{
	int *temp = NULL;
	temp = mCxtX;
	for (int i = 0; i < mCxtRegionSize.width; i++)
	{
		*temp = floor(mCenter.x + (i - mCxtRegionSize.width / 2.0));
		if (*temp < 0)
		{
			*temp = 0;
		}
		if (*temp >= mFrame.cols)
		{
			*temp = mFrame.cols - 1;
		}
		temp++;
	}

	temp = mCxtY;
	for (int i = 0; i < mCxtRegionSize.height; i++)
	{
		*temp = floor(mCenter.y + (i - mCxtRegionSize.height / 2.0));
		if (*temp < 0)
		{
			*temp = 0;
		}
		if (*temp >= mFrame.rows)
		{
			*temp = mFrame.rows - 1;
		}
		temp++;
	}

	double *mData = NULL;
	uchar *mFrameData = NULL;
	double mPxSum = 0, mPxAverage = 0;

	for (int i = 0; i < mCxtRegionSize.height; i++)
	{

		mData = (double *)(mCxtMat.data + mCxtMat.step * i);
		mFrameData = (uchar *)(mFrame.data + mFrame.step * mCxtY[i]);
		for (int j = 0; j < mCxtRegionSize.width; j++)
		{
			mData[j] = mFrameData[mCxtX[j]];
			mPxSum += mData[j];
		}

	}
	mPxAverage = mPxSum / (mCxtRegionSize.width * mCxtRegionSize.height);

	mCxtMat.convertTo(mCxtMat, -1, 1, -mPxAverage);

	mCxtPriProMat = mFilterWindowMat.mul(mCxtMat);

}


void STCTracker::learnSCModol()
{
	//对先验概率进行FFT转换
	Mat priorFourier;
	Mat planes1[] = { mCxtPriProMat, Mat::zeros(mCxtPriProMat.size(), CV_64FC1) };
	merge(planes1, 2, priorFourier);
	dft(priorFourier, priorFourier);

	complexDivide(mCxtPostProFFTMat, priorFourier, mSCModelFFT);

	//反FFT转换得到当前帧的空间上下文模型
	//dft(conditionalFourier, mSCModelFFT, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);

}

void STCTracker::updateSTCModol()
{
	addWeighted(mSTCModelFFT, 1.0 - mLearnRate, mSCModelFFT, mLearnRate, 0.0, mSTCModelFFT);
}



/************************************************************************/
/* 首先初始化一个够长的数组用于存储每一帧对应的置信度，用于处理尺度缩放
/************************************************************************/
void STCTracker::init(const Mat& mFrame, const Rect& mBox, int mFrameCount)
{
	out << "初始跟踪框：" << mBox << endl;
	mFrameIndex = 0;

	if ( mFrameCount <= 0 )
	{
		this->mFrameCount = 1000;
	}
	else 
	{
		this->mFrameCount = mFrameCount + 5;
	}

	mEachFrameConfd = new double[this->mFrameCount];
	memset(mEachFrameConfd, 0, this->mFrameCount * sizeof(double));

	sigma = 0.5 * (mBox.width + mBox.height);
	mCenter.x = mBox.x + mBox.width / 2;
	mCenter.y = mBox.y + mBox.height / 2;
	out << "初始跟踪中心：" << mCenter << endl;

	mRectSize.width = mBox.width;
	mRectSize.height = mBox.height;

	mCxtRegionSize.width = mBox.width * 2;
	mCxtRegionSize.height = mBox.height * 2;

	if (mCxtRegionSize.width > mFrame.cols)
	{
		mCxtRegionSize.width = mFrame.cols;
	}

	if (mCxtRegionSize.width < mBox.width)
	{
		mCxtRegionSize.width = mBox.width;
	}

	if (mCxtRegionSize.height >= mFrame.rows)
	{
		mCxtRegionSize.height = mFrame.rows;
	}

	if (mCxtRegionSize.height < mBox.height)
	{
		mCxtRegionSize.height = mBox.height;
	}

	mCxtX = new int[mCxtRegionSize.width];
	mCxtY = new int[mCxtRegionSize.height];

	mCxtPriProMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);
	mCxtPostProMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);
	mSCModelFFT = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC2);
	mSTCModelFFT = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC2);
	mFilterWindowMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);
	mEucliDistMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);
	createEucliDistMatAndCxtPostPro();

	mHammingWinMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);
	createHammingWindow();

	mCxtMat = Mat::zeros(mCxtRegionSize.height, mCxtRegionSize.width, CV_64FC1);

	getFilterWindow();
	getContext(mFrame);
	learnSCModol();
	mSCModelFFT.copyTo(mSTCModelFFT);

	out << endl;
}


void STCTracker::track(const Mat& mFrame, Rect& mBox)
{
	mFrameIndex++;
	out << "第" << mFrameIndex << "帧" << endl;
	out << "初始跟踪框:" << mBox;

	sigma = sigma * mScale;

	getFilterWindow();
	getContext(mFrame);

	Mat priorFourier;
	Mat planes1[] = { mCxtPriProMat, Mat::zeros(mCxtPriProMat.size(), CV_64FC1) };
	merge(planes1, 2, priorFourier);
	dft(priorFourier, priorFourier);

	Mat conditionalFourier;
	complexMultiply(mSTCModelFFT, priorFourier, conditionalFourier);
	dft(conditionalFourier, mNextCxtPostProMat, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);

	Point point;
	minMaxLoc(mNextCxtPostProMat, 0, 0, 0, &point);
	out << "先验最大置信度坐标:" << point << endl;
	if (point.x == -1 || point.y == -1)
	{
		out << "找不到最大置信度的点，程序已经无法继续执行，不在进行预测" << endl;
		out << endl;
		return;
	}
	out << "先验置信度:" << *(((double *)(mNextCxtPostProMat.data + point.y * mNextCxtPostProMat.step)) + point.x) << endl;

	//这里要加一个像素的偏移量，是因为point得到的是矩阵中元素的坐标，尺度范围为0到(size-1)，而mCxtRegionSize的宽高除二得到的中心坐标的尺度范围为1到size
	//故这里加上一个像素的偏移量，使两种坐标落到一个尺度范围内，便可得到中心坐标移动的偏移量
	mCenter.x = mCenter.x + point.x + 1 - mCxtRegionSize.width / 2;
	mCenter.y = mCenter.y + point.y + 1 - mCxtRegionSize.height / 2;
	out << "预测后跟踪框中心：X-" << mCenter.x << ",Y-" << mCenter.y << endl;

	getFilterWindow();
	getContext(mFrame);

	Mat planes2[] = { mCxtPriProMat, Mat::zeros(mCxtPriProMat.size(), CV_64FC1) };
	merge(planes2, 2, priorFourier);
	dft(priorFourier, priorFourier);

	Mat conditionalFourier2;
	complexMultiply(mSTCModelFFT, priorFourier, conditionalFourier2);
	dft(conditionalFourier2, mNextCxtPostProMat, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);

	minMaxLoc(mNextCxtPostProMat, 0, 0, 0, &point);
	mEachFrameConfd[mFrameIndex] = *(((double *)(mNextCxtPostProMat.data + point.y * mNextCxtPostProMat.step)) + point.x);
	out << "后验最大置信度坐标:" << point << endl;

	if (mEachFrameConfd[mFrameIndex] > 0.001)
	{
		if (mEachFrameConfd[mFrameIndex] < mMinConfiPosi)
		{
			mMinConfiPosi = mEachFrameConfd[mFrameIndex];
		}
	}
	else
	{
		mEachFrameConfd[mFrameIndex] = mMinConfiPosi;
	}
	out << "后验置信度:" << mEachFrameConfd[mFrameIndex] << endl;

	if (mFrameIndex % (mIntervalNumber + 1) == 0){
		double mSum = 0;
		for (int i = 0; i < mIntervalNumber; i++)
		{
			mSum = mSum + sqrt(mEachFrameConfd[mFrameIndex - i] / mEachFrameConfd[mFrameIndex - i - 1]);
		}
		mScale = (1 - mScaleLearnRate)*mScale + mScaleLearnRate*(mSum / mIntervalNumber);
	}
	
	mRectSize.width = mRectSize.width * mScale;
	mRectSize.height = mRectSize.height * mScale;
	mBox.width = (int)mRectSize.width;
	mBox.height = (int)mRectSize.height;
	mBox.x = mCenter.x - mBox.width / 2;
	mBox.y = mCenter.y - mBox.height / 2;

	out << "Scale-->" << mScale << endl;
	out << "跟踪结果:" << mBox << endl;

	learnSCModol();
	updateSTCModol();

	out << endl;
}
