#pragma once

class AppScale {
	static float _scale;

public:
	static void set_scale(float p_scale);
	static float get_scale();
};

#define APP_SCALE (AppScale::get_scale())

// TODO: Remove
#define EDSCALE APP_SCALE
