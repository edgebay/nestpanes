#pragma once

#include "scene/gui/control.h"

class AppControl : public Control {
	GDCLASS(AppControl, Control);

public:
	int get_app_theme_constant(const StringName &p_name) const;
	Ref<Texture2D> get_app_theme_icon(const StringName &p_name) const;
	bool has_app_theme_icon(const StringName &p_name) const;
};
