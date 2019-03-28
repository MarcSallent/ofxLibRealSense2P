#pragma once

#include <librealsense2/rs.hpp> 
#include <librealsense2/rsutil.h>
#include "ofMain.h"
#include "ofxGui.h"
#include "ofxLRS2/Filter.h"

class ofxLibRealSense2P : public ofThread
{

public:
	enum COLOR_SCHEMA
	{
		COLOR_SCHEMA_Jet,
		COLOR_SCHEMA_Classic, 
		COLOR_SCHEMA_WhiteToBlack, 
		COLOR_SCHEMA_BlackToWhite, 
		COLOR_SCHEMA_Bio, 
		COLOR_SCHEMA_Cold, 
		COLOR_SCHEMA_Warm, 
		COLOR_SCHEMA_Quantized, 
		COLOR_SCHEMA_Pattern
	};

	ofxLibRealSense2P()
	{
	}
	~ofxLibRealSense2P()
	{
		stop();
		waitForThread(false);
	}

	void setupDevice(int deviceID = 0, bool listAvailableStream = true);
	void load(string path);

	void setupFilter();

	void enableColor(int width, int height, int fps = 60, bool useArbTex = true);
	void enableIr(int width, int height, int fps = 60, bool useArbTex = true);
	void enableDepth(int width, int height, int fps = 60, bool useArbTex = true);
	void startRecord(string path);
	void stopRecord(bool bplayback = true);
	void playbackRecorded();
	bool isRecording();

	void startStream();

	void threadedFunction();
	void update();

	float getDistanceAt(int x, int y);
	glm::vec3 getWorldCoordinateAt(float x, float y);

	ofxGuiGroup* setupGUI();
	void onD400BoolParamChanged(bool &value);
	void onD400IntParamChanged(int &value);
	void onD400ColorizerParamChanged(float &value);
	ofxGuiGroup* getGui();


	void setAligned(bool aligned)
	{
		if (depth_enabled && color_enabled)
		{
			this->bAligned = aligned;
		}
		else
		{
			ofLogWarning() << "Align FAILED";
		}
	}

	void drawDepthRaw(int x, int y)
	{
		if (depth_enabled)
			raw_depth_texture->draw(x, y);
	}

	void drawDepth(int x, int y)
	{
		if(depth_enabled)
			depth_texture->draw(x, y);
	}

	void drawColor(int x, int y)
	{
		if (color_enabled)
			color_texture->draw(x, y);
	}

	void drawIR(int x, int y)
	{
		if (ir_enabled)
			ir_tex->draw(x, y);
	}

	float getDepthWidth() const
	{
		return depth_width;
	}

	float getDepthHeight() const
	{
		return depth_height;
	}

	float getColorWidth() const
	{
		if (bAligned)
		{
			return _color.as<rs2::video_frame>().get_width();
		}
		return color_width;
	}

	float getColorHeight() const
	{
		if (bAligned)
		{
			return _color.as<rs2::video_frame>().get_height();
		}
		return color_height;
	}

	bool isFrameNew()
	{
		return bFrameNew;
	}

	shared_ptr<ofTexture> getDepthTexture() const
	{
		return depth_texture;
	}

	shared_ptr<ofTexture> getDepthRawTexture() const
	{
		return raw_depth_texture;
	}

	shared_ptr<ofTexture> getColorTexture()
	{
		return color_texture;
	}

	float getDepthScale()
	{
		return depthScale;
	}

	void setDepthColorSchema(COLOR_SCHEMA schema)
	{
		rs2colorizer.set_option(RS2_OPTION_COLOR_SCHEME, schema);
	}

	rs2_intrinsics getCameraIntrinsics() const
	{
		return intr;
	}

private:
	ofThreadChannel<rs2::frameset> frameChannel;
	rs2::device		rs2device;
	rs2::config		rs2config; 
	rs2::colorizer  rs2colorizer;
	shared_ptr<rs2::pipeline>	rs2_pipeline;

	rs2::frame_queue rs2depth_queue;
	rs2::frame_queue rs2video_queue;
	rs2::frame_queue rs2ir_queue;


	rs2::frame _depth;
	rs2::frame _color;
	rs2_intrinsics intr;

	//attribute
	ofxGuiGroup     _D400Params;
	ofxToggle       _autoExposure;
	ofxToggle       _enableEmitter;
	ofxIntSlider    _irExposure;
	ofxFloatSlider  _depthMin;
	ofxFloatSlider  _depthMax;

	ofPixels         _colBuff, _irBuff, _depthBuff;
	ofShortPixels    _rawDepthBuff;

	bool _isRecording;

	int _color_fps, _ir_fps, _depth_fps;

	vector<shared_ptr<ofxlibrealsense2p::Filter>> filters;

	float depthScale;

	int deviceId;
	string device_serial;
	bool color_enabled = false, ir_enabled = false, depth_enabled = false;
	atomic_bool _isFrameNew;
	bool bFrameNew = false;

	int color_width, color_height;
	int ir_width, ir_height;
	int depth_width, depth_height;
	int original_depth_width, original_depth_height;

	bool bReadFile = false;
	bool bAligned = false;
	bool bUseArbTexDepth = true;
	string _recordFilePath;

	ofPtr<ofTexture> depth_texture, raw_depth_texture, color_texture, ir_tex;

	rs2::disparity_transform* disparity_to_depth;

	void start()
	{
		startThread();
	}

	void stop()
	{
		stopThread();
	}

	float calcDepthScale(rs2::device dev)
	{
		// Go over the device's sensors
		for (rs2::sensor& sensor : dev.query_sensors())
		{
			// Check if the sensor if a depth sensor
			if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
			{
				return dpt.get_depth_scale();
			}
		}
		throw std::runtime_error("Device does not have a depth sensor");
	}

	void allocateDepthBuffer(float width, float height)
	{
		_depthBuff.clear();
		_depthBuff.allocate(width, height, 3);
		if (!depth_texture->isAllocated() || (depth_texture->getWidth() != width || depth_texture->getHeight() != height))
		{
			if(depth_texture->isAllocated())depth_texture->clear();
			depth_texture->allocate(width, height, GL_RGB, bUseArbTexDepth);
		}
		if (!raw_depth_texture->isAllocated() || (raw_depth_texture->getWidth() != width || raw_depth_texture->getHeight() != height))
		{
			raw_depth_texture->clear();
			raw_depth_texture->allocate(width, height, GL_R16, bUseArbTexDepth);
		}
		depth_width = width;
		depth_height = height;
		raw_depth_texture->setRGToRGBASwizzles(true);
	}
	void listSensorProfile();
	void listStreamingProfile(const rs2::sensor& sensor);
	void process();
	bool _useThread = true;
};
