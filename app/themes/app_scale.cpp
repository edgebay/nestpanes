#include "app_scale.h"

float AppScale::_scale = 1.0f;

void AppScale::set_scale(float p_scale) {
	_scale = p_scale;
}

float AppScale::get_scale() {
	return _scale;
}
