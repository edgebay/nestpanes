#pragma once

#include "scene/gui/control.h"

class AppControl : public Control {
	GDCLASS(AppControl, Control);

	// public:
	// 	Ref<Texture2D> get_app_theme_icon(const StringName &p_name) const;
	// 	// bool has_app_theme_icon(const StringName &p_name) const;
};

// Ref<Texture2D> get_app_theme_icon(const StringName &p_name) const;
#define get_app_theme_icon(_p_name) get_theme_icon(_p_name, SNAME("AppIcons"))

// Ref<Texture2D> get_app_theme_native_menu_icon(const StringName &p_name, bool p_global_menu, bool p_dark_mode) const;
#define get_app_theme_native_menu_icon(_p_name, _p_global_menu, _p_dark_mode)                       \
	do {                                                                                            \
		if (!_p_global_menu) {                                                                      \
			return get_theme_icon(_p_name, SNAME("AppIcons"));                                      \
		}                                                                                           \
		if (_p_dark_mode && has_theme_icon(String(_p_name) + "Dark", SNAME("AppIcons"))) {          \
			return get_theme_icon(String(_p_name) + "Dark", SNAME("AppIcons"));                     \
		} else if (!_p_dark_mode && has_theme_icon(String(_p_name) + "Light", SNAME("AppIcons"))) { \
			return get_theme_icon(String(_p_name) + "Light", SNAME("AppIcons"));                    \
		}                                                                                           \
		return get_theme_icon(_p_name, SNAME("AppIcons"));                                          \
	} while (0)

// TODO: Remove
#define get_editor_theme_icon get_app_theme_icon
