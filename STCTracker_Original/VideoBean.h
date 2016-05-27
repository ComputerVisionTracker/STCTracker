#pragma once

#include "string"

using namespace std;

class VideoBean
{
public:
	string mVideoPath;
	int x;
	int y;
	int width;
	int height;
	string name;


public:
	VideoBean();
	VideoBean(string mVideoPath, int x, int y, int width, int height, string name);
	VideoBean(const VideoBean& r);
	~VideoBean();

public:
	VideoBean& operator =(const VideoBean &r);

public:
	void set(string mVideoPath, int x, int y, int width, int height, string name);
};

