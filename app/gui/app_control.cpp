#include "app_control.h"

int AppControl::get_app_theme_constant(const StringName &p_name) const {
	return get_theme_constant(p_name, SNAME("AppIcons"));
}

Ref<Texture2D> AppControl::get_app_theme_icon(const StringName &p_name) const {
	return get_theme_icon(p_name, SNAME("AppIcons"));
}

bool AppControl::has_app_theme_icon(const StringName &p_name) const {
	return has_theme_icon(p_name, SNAME("AppIcons"));
}
