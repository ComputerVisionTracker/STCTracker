#include "VideoBean.h"

VideoBean::VideoBean()
{
}

VideoBean::VideoBean(string mVideoPath, int x, int y, int width, int height, string name)
{
	this->mVideoPath = mVideoPath;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->name = name;
}

VideoBean::VideoBean(const VideoBean& r)
{
	mVideoPath = r.mVideoPath;
	x = r.x;
	y = r.y;
	width = r.width;
	height = r.height;
	name = r.name;
}


VideoBean::~VideoBean()
{
}

VideoBean& VideoBean::operator=(const VideoBean& r)
{
	if (this == &r)
	{
		return *this;
	}

	(VideoBean&)*this = r;
	return *this;
}

void VideoBean::set(string mVideoPath, int x, int y, int width, int height, string name)
{
	this->mVideoPath = mVideoPath;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->name = name;
}
