#include "app_theme.h"

#include "app/app_string_names.h"
#include "scene/theme/theme_db.h"

Vector<StringName> AppTheme::app_theme_types;

// TODO: Refactor these and corresponding Theme methods to use the bool get_xxx(r_value) pattern internally.

// Keep in sync with Theme::get_color.
Color AppTheme::get_color(const StringName &p_name, const StringName &p_theme_type) const {
	if (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name)) {
		return color_map[p_theme_type][p_name];
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme color '%s' in '%s'.", p_name, p_theme_type));
		}
		return Color();
	}
}

// Keep in sync with Theme::get_constant.
int AppTheme::get_constant(const StringName &p_name, const StringName &p_theme_type) const {
	if (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name)) {
		return constant_map[p_theme_type][p_name];
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme constant '%s' in '%s'.", p_name, p_theme_type));
		}
		return 0;
	}
}

// Keep in sync with Theme::get_font.
Ref<Font> AppTheme::get_font(const StringName &p_name, const StringName &p_theme_type) const {
	if (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) && font_map[p_theme_type][p_name].is_valid()) {
		return font_map[p_theme_type][p_name];
	} else if (has_default_font()) {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme font '%s' in '%s'.", p_name, p_theme_type));
		}
		return default_font;
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme font '%s' in '%s'.", p_name, p_theme_type));
		}
		return ThemeDB::get_singleton()->get_fallback_font();
	}
}

// Keep in sync with Theme::get_font_size.
int AppTheme::get_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	if (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) && (font_size_map[p_theme_type][p_name] > 0)) {
		return font_size_map[p_theme_type][p_name];
	} else if (has_default_font_size()) {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme font size '%s' in '%s'.", p_name, p_theme_type));
		}
		return default_font_size;
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme font size '%s' in '%s'.", p_name, p_theme_type));
		}
		return ThemeDB::get_singleton()->get_fallback_font_size();
	}
}

// Keep in sync with Theme::get_icon.
Ref<Texture2D> AppTheme::get_icon(const StringName &p_name, const StringName &p_theme_type) const {
	if (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) && icon_map[p_theme_type][p_name].is_valid()) {
		return icon_map[p_theme_type][p_name];
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme icon '%s' in '%s'.", p_name, p_theme_type));
		}
		return ThemeDB::get_singleton()->get_fallback_icon();
	}
}

// Keep in sync with Theme::get_stylebox.
Ref<StyleBox> AppTheme::get_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	if (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) && style_map[p_theme_type][p_name].is_valid()) {
		return style_map[p_theme_type][p_name];
	} else {
		if (app_theme_types.has(p_theme_type)) {
			WARN_PRINT(vformat("Trying to access a non-existing app theme stylebox '%s' in '%s'.", p_name, p_theme_type));
		}
		return ThemeDB::get_singleton()->get_fallback_stylebox();
	}
}

void AppTheme::initialize() {
	app_theme_types.append(AppStringName(App));
	app_theme_types.append(AppStringName(AppFonts));
	app_theme_types.append(AppStringName(AppIcons));
	app_theme_types.append(AppStringName(AppStyles));
}

void AppTheme::finalize() {
	app_theme_types.clear();
}
