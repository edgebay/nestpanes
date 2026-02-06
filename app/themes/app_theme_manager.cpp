#include "app_theme_manager.h"

#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"

#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h"
#include "scene/resources/texture.h"

#include "scene/scene_string_names.h"

#include "servers/display/display_server.h"

#include "app/app_modules/settings/app_settings.h"
#include "app/app_string_names.h"
#include "app/themes/app_fonts.h"
#include "app/themes/app_icons.h"
#include "app/themes/app_scale.h"
#include "app/themes/theme_classic.h"
#include "app/themes/theme_modern.h"

// Theme configuration.

uint32_t AppThemeManager::ThemeConfiguration::hash() {
	uint32_t hash = hash_murmur3_one_float(APP_SCALE);

	// Basic properties.

	hash = hash_murmur3_one_32(style.hash(), hash);
	hash = hash_murmur3_one_32(preset.hash(), hash);
	hash = hash_murmur3_one_32(spacing_preset.hash(), hash);

	hash = hash_murmur3_one_32(base_color.to_rgba32(), hash);
	hash = hash_murmur3_one_32(accent_color.to_rgba32(), hash);
	hash = hash_murmur3_one_float(contrast, hash);
	hash = hash_murmur3_one_float(icon_saturation, hash);

	// Extra properties.

	hash = hash_murmur3_one_32(base_spacing, hash);
	hash = hash_murmur3_one_32(extra_spacing, hash);
	hash = hash_murmur3_one_32(border_width, hash);
	hash = hash_murmur3_one_32(corner_radius, hash);

	hash = hash_murmur3_one_32((int)draw_extra_borders, hash);
	hash = hash_murmur3_one_float(relationship_line_opacity, hash);
	hash = hash_murmur3_one_32(thumb_size, hash);
	hash = hash_murmur3_one_32(class_icon_size, hash);
	hash = hash_murmur3_one_32((int)enable_touch_optimizations, hash);
	hash = hash_murmur3_one_float(gizmo_handle_scale, hash);
	hash = hash_murmur3_one_32(inspector_property_height, hash);
	hash = hash_murmur3_one_float(subresource_hue_tint, hash);

	hash = hash_murmur3_one_float(default_contrast, hash);

	// Generated properties.

	hash = hash_murmur3_one_32((int)dark_theme, hash);
	hash = hash_murmur3_one_32((int)dark_icon_and_font, hash);

	return hash;
}

uint32_t AppThemeManager::ThemeConfiguration::hash_fonts() {
	uint32_t hash = hash_murmur3_one_float(APP_SCALE);

	// TODO: Implement the hash based on what app_register_fonts() uses.

	return hash;
}

uint32_t AppThemeManager::ThemeConfiguration::hash_icons() {
	uint32_t hash = hash_murmur3_one_float(APP_SCALE);

	hash = hash_murmur3_one_32(accent_color.to_rgba32(), hash);
	hash = hash_murmur3_one_float(icon_saturation, hash);

	hash = hash_murmur3_one_32(thumb_size, hash);
	hash = hash_murmur3_one_float(gizmo_handle_scale, hash);

	hash = hash_murmur3_one_32((int)dark_icon_and_font, hash);

	return hash;
}

// Benchmarks.

int AppThemeManager::benchmark_run = 0;

String AppThemeManager::get_benchmark_key() {
	if (benchmark_run == 0) {
		return "AppTheme (Startup)";
	}

	return vformat("AppTheme (Run %d)", benchmark_run);
}

// Generation helper methods.

Ref<StyleBoxTexture> AppThemeManager::make_stylebox(Ref<Texture2D> p_texture, float p_left, float p_top, float p_right, float p_bottom, float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom, bool p_draw_center) {
	Ref<StyleBoxTexture> style(memnew(StyleBoxTexture));
	style->set_texture(p_texture);
	style->set_texture_margin_individual(p_left * APP_SCALE, p_top * APP_SCALE, p_right * APP_SCALE, p_bottom * APP_SCALE);
	style->set_content_margin_individual((p_left + p_margin_left) * APP_SCALE, (p_top + p_margin_top) * APP_SCALE, (p_right + p_margin_right) * APP_SCALE, (p_bottom + p_margin_bottom) * APP_SCALE);
	style->set_draw_center(p_draw_center);
	return style;
}

Ref<StyleBoxEmpty> AppThemeManager::make_empty_stylebox(float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom) {
	Ref<StyleBoxEmpty> style(memnew(StyleBoxEmpty));
	style->set_content_margin_individual(p_margin_left * APP_SCALE, p_margin_top * APP_SCALE, p_margin_right * APP_SCALE, p_margin_bottom * APP_SCALE);
	return style;
}

Ref<StyleBoxFlat> AppThemeManager::make_flat_stylebox(Color p_color, float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom, int p_corner_width) {
	Ref<StyleBoxFlat> style(memnew(StyleBoxFlat));
	style->set_bg_color(p_color);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(Math::ceil(0.8 * p_corner_width * APP_SCALE));
	style->set_corner_radius_all(p_corner_width * APP_SCALE);
	style->set_content_margin_individual(p_margin_left * APP_SCALE, p_margin_top * APP_SCALE, p_margin_right * APP_SCALE, p_margin_bottom * APP_SCALE);
	return style;
}

Ref<StyleBoxLine> AppThemeManager::make_line_stylebox(Color p_color, int p_thickness, float p_grow_begin, float p_grow_end, bool p_vertical) {
	Ref<StyleBoxLine> style(memnew(StyleBoxLine));
	style->set_color(p_color);
	style->set_grow_begin(p_grow_begin);
	style->set_grow_end(p_grow_end);
	style->set_thickness(p_thickness);
	style->set_vertical(p_vertical);
	return style;
}

// Theme generation and population routines.

Ref<AppTheme> AppThemeManager::_create_base_theme(const Ref<AppTheme> &p_old_theme) {
	OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Create Base Theme");

	Ref<AppTheme> theme = memnew(AppTheme);
	ThemeConfiguration config = _create_theme_config();
	theme->set_generated_hash(config.hash());
	theme->set_generated_fonts_hash(config.hash_fonts());
	theme->set_generated_icons_hash(config.hash_icons());

	print_verbose(vformat("AppTheme: Generating new theme for the config '%d'.", theme->get_generated_hash()));

	bool is_default_style = config.style == "Modern";
	if (is_default_style) {
		ThemeModern::populate_shared_styles(theme, config);
	} else {
		ThemeClassic::populate_shared_styles(theme, config);
	}

	// Register icons.
	{
		OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Register Icons");

		AppIcons::app_configure_icons(config.dark_icon_and_font);

		// If settings are comparable to the old theme, then just copy existing icons over.
		// Otherwise, regenerate them.
		bool keep_old_icons = (p_old_theme.is_valid() && theme->get_generated_icons_hash() == p_old_theme->get_generated_icons_hash());
		if (keep_old_icons) {
			print_verbose("AppTheme: Can keep old icons, copying.");
			AppIcons::app_copy_icons(theme, p_old_theme);
		} else {
			print_verbose("AppTheme: Generating new icons.");
			AppIcons::app_register_icons(theme, config.dark_icon_and_font, config.icon_saturation, config.thumb_size, config.gizmo_handle_scale);
		}

		OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Register Icons");
	}

	// Register fonts.
	{
		OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Register Fonts");

		// TODO: Check if existing font definitions from the old theme are usable and copy them.

		// External function, see app_fonts.cpp.
		print_verbose("AppTheme: Generating new fonts.");
		app_register_fonts(theme);

		OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Register Fonts");
	}

	print_verbose("AppTheme: Generating new styles.");

	if (is_default_style) {
		ThemeModern::populate_standard_styles(theme, config);
		ThemeModern::populate_app_styles(theme, config);
	} else {
		ThemeClassic::populate_standard_styles(theme, config);
		ThemeClassic::populate_app_styles(theme, config);
	}

	// _populate_text_editor_styles(theme, config);
	// _populate_visual_shader_styles(theme, config);

	OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Create Base Theme");
	return theme;
}

AppThemeManager::ThemeConfiguration AppThemeManager::_create_theme_config() {
	ThemeConfiguration config;

	// Basic properties.

	config.style = EDITOR_GET("interface/theme/style");
	config.preset = EDITOR_GET("interface/theme/color_preset");
	config.spacing_preset = APP_GET("interface/theme/spacing_preset");

	config.base_color = APP_GET("interface/theme/base_color");
	config.accent_color = APP_GET("interface/theme/accent_color");
	config.contrast = APP_GET("interface/theme/contrast");
	config.icon_saturation = APP_GET("interface/theme/icon_saturation");
	config.corner_radius = EDITOR_GET("interface/theme/corner_radius");

	// Extra properties.

	config.base_spacing = APP_GET("interface/theme/base_spacing");
	config.extra_spacing = APP_GET("interface/theme/additional_spacing");
	// Ensure borders are visible when using an app scale below 100%.
	config.border_width = CLAMP((int)APP_GET("interface/theme/border_size"), 0, 2) * MAX(1, APP_SCALE);

	config.draw_extra_borders = EDITOR_GET("interface/theme/draw_extra_borders");
	config.draw_relationship_lines = EDITOR_GET("interface/theme/draw_relationship_lines");
	config.relationship_line_opacity = APP_GET("interface/theme/relationship_line_opacity");
	config.thumb_size = APP_GET("filesystem/file_dialog/thumbnail_size");
	config.class_icon_size = 16 * APP_SCALE;
	// config.enable_touch_optimizations = APP_GET("interface/touchscreen/enable_touch_optimizations");
	// config.gizmo_handle_scale = APP_GET("interface/touchscreen/scale_gizmo_handles");
	// config.subresource_hue_tint = APP_GET("docks/property_editor/subresource_hue_tint");
	config.dragging_hover_wait_msec = (float)EDITOR_GET("interface/app/dragging_hover_wait_seconds") * 1000;

	// Handle theme style.
	if (config.preset != "Custom") {
		if (config.style == "Classic") {
			config.draw_relationship_lines = RELATIONSHIP_ALL;
			config.corner_radius = 3;
		} else { // Default
			config.draw_relationship_lines = config.default_relationship_lines;
			config.corner_radius = config.default_corner_radius;
		}

		EditorSettings::get_singleton()->set_initial_value("interface/theme/draw_relationship_lines", config.draw_relationship_lines);
		EditorSettings::get_singleton()->set_initial_value("interface/theme/corner_radius", config.corner_radius);

		// Enforce values in case they were adjusted or overridden.
		EditorSettings::get_singleton()->set_manually("interface/theme/draw_relationship_lines", config.draw_relationship_lines);
		EditorSettings::get_singleton()->set_manually("interface/theme/corner_radius", config.corner_radius);
	}

	// Handle color preset.
	{
		const bool follow_system_theme = APP_GET("interface/theme/follow_system_theme");
		const bool use_system_accent_color = APP_GET("interface/theme/use_system_accent_color");
		DisplayServer *display_server = DisplayServer::get_singleton();
		Color system_base_color = display_server->get_base_color();
		Color system_accent_color = display_server->get_accent_color();

		if (follow_system_theme) {
			String dark_theme = "Default";
			String light_theme = "Light";

			config.preset = light_theme; // Assume light theme if we can't detect system theme attributes.

			if (system_base_color == Color(0, 0, 0, 0)) {
				if (display_server->is_dark_mode_supported() && display_server->is_dark_mode()) {
					config.preset = dark_theme;
				}
			} else {
				if (system_base_color.get_luminance() < 0.5) {
					config.preset = dark_theme;
				}
			}
		}

		if (config.preset != "Custom") {
			Color preset_accent_color;
			Color preset_base_color;
			float preset_contrast = config.default_contrast;
			bool preset_draw_extra_borders = false;
			float preset_icon_saturation = config.default_icon_saturation;

			// A negative contrast rate looks better for light themes, since it better follows the natural order of UI "elevation".
			const float light_contrast = -0.06;

			// Please use alphabetical order if you're adding a new color preset here.
			if (config.preset == "Black (OLED)") {
				preset_accent_color = Color(0.45, 0.75, 1.0);
				preset_base_color = Color(0, 0, 0);
				// The contrast rate value is irrelevant on a fully black theme.
				preset_contrast = 0.0;
				preset_draw_extra_borders = true;
			} else if (config.preset == "Breeze Dark") {
				preset_accent_color = Color(0.239, 0.682, 0.914);
				preset_base_color = Color(0.1255, 0.1373, 0.149);
			} else if (config.preset == "Godot 2") {
				preset_accent_color = Color(0.53, 0.67, 0.89);
				preset_base_color = Color(0.24, 0.23, 0.27);
				preset_icon_saturation = 1;
			} else if (config.preset == "Godot 3") {
				preset_accent_color = Color(0.44, 0.73, 0.98);
				preset_base_color = Color(0.21, 0.24, 0.29);
				preset_icon_saturation = 1;
			} else if (config.preset == "Gray") {
				preset_accent_color = Color(0.44, 0.73, 0.98);
				preset_base_color = Color(0.24, 0.24, 0.24);
			} else if (config.preset == "Light") {
				preset_accent_color = Color(0.18, 0.50, 1.00);
				preset_base_color = Color(0.9, 0.9, 0.9);
				preset_contrast = light_contrast;
				preset_icon_saturation = 1;
			} else if (config.preset == "Solarized (Dark)") {
				preset_accent_color = Color(0.15, 0.55, 0.82);
				preset_base_color = Color(0.03, 0.21, 0.26);
				preset_contrast = 0.23;
			} else if (config.preset == "Solarized (Light)") {
				preset_accent_color = Color(0.15, 0.55, 0.82);
				preset_base_color = Color(0.89, 0.86, 0.79);
				preset_contrast = light_contrast;
			} else { // Default
				preset_accent_color = Color(0.337, 0.62, 1.0);
				preset_base_color = Color(0.161, 0.161, 0.161);
			}

			config.accent_color = preset_accent_color;
			config.base_color = preset_base_color;
			config.contrast = preset_contrast;
			config.draw_extra_borders = preset_draw_extra_borders;
			config.icon_saturation = preset_icon_saturation;

			AppSettings::get_singleton()->set_initial_value("interface/theme/accent_color", config.accent_color);
			AppSettings::get_singleton()->set_initial_value("interface/theme/base_color", config.base_color);
			AppSettings::get_singleton()->set_initial_value("interface/theme/contrast", config.contrast);
			AppSettings::get_singleton()->set_initial_value("interface/theme/draw_extra_borders", config.draw_extra_borders);
			EditorSettings::get_singleton()->set_initial_value("interface/theme/icon_saturation", config.icon_saturation);
		}

		if (follow_system_theme && system_base_color != Color(0, 0, 0, 0)) {
			config.base_color = system_base_color;
			config.preset = "Custom";
		}

		if (use_system_accent_color && system_accent_color != Color(0, 0, 0, 0)) {
			config.accent_color = system_accent_color;
			config.preset = "Custom";
		}

		// Enforce values in case they were adjusted or overridden.
		AppSettings::get_singleton()->set_manually("interface/theme/preset", config.preset);
		AppSettings::get_singleton()->set_manually("interface/theme/accent_color", config.accent_color);
		AppSettings::get_singleton()->set_manually("interface/theme/base_color", config.base_color);
		AppSettings::get_singleton()->set_manually("interface/theme/contrast", config.contrast);
		AppSettings::get_singleton()->set_manually("interface/theme/draw_extra_borders", config.draw_extra_borders);
		EditorSettings::get_singleton()->set_manually("interface/theme/icon_saturation", config.icon_saturation);
	}

	// Handle theme spacing preset.
	{
		if (config.spacing_preset != "Custom") {
			int preset_base_spacing = 0;
			int preset_extra_spacing = 0;
			Size2 preset_dialogs_buttons_min_size;

			if (config.spacing_preset == "Compact") {
				preset_base_spacing = 2;
				preset_extra_spacing = 2;
				preset_dialogs_buttons_min_size = Size2(90, 26);
			} else if (config.spacing_preset == "Spacious") {
				preset_base_spacing = 6;
				preset_extra_spacing = 2;
				preset_dialogs_buttons_min_size = Size2(112, 36);
			} else { // Default
				preset_base_spacing = 4;
				preset_extra_spacing = 0;
				preset_dialogs_buttons_min_size = Size2(105, 34);
			}

			config.base_spacing = preset_base_spacing;
			config.extra_spacing = preset_extra_spacing;
			config.dialogs_buttons_min_size = preset_dialogs_buttons_min_size;

			AppSettings::get_singleton()->set_initial_value("interface/theme/base_spacing", config.base_spacing);
			AppSettings::get_singleton()->set_initial_value("interface/theme/additional_spacing", config.extra_spacing);
		}

		// Enforce values in case they were adjusted or overridden.
		AppSettings::get_singleton()->set_manually("interface/theme/spacing_preset", config.spacing_preset);
		AppSettings::get_singleton()->set_manually("interface/theme/base_spacing", config.base_spacing);
		AppSettings::get_singleton()->set_manually("interface/theme/additional_spacing", config.extra_spacing);
	}

	// Generated properties.

	config.dark_theme = is_dark_theme();
	config.dark_icon_and_font = is_dark_icon_and_font();

	config.base_margin = config.base_spacing;
	config.increased_margin = config.base_spacing + config.extra_spacing * 0.75;
	config.separation_margin = (config.base_spacing + config.extra_spacing / 2) * APP_SCALE;
	config.popup_margin = config.base_margin * 2.4 * APP_SCALE;
	// Make sure content doesn't stick to window decorations; this can be fixed in future with layout changes.
	config.window_border_margin = MAX(1, config.base_margin * APP_SCALE);
	config.top_bar_separation = MAX(1, config.base_margin * APP_SCALE);

	// Force the v_separation to be even so that the spacing on top and bottom is even.
	// If the vsep is odd and cannot be split into 2 even groups (of pixels), then it will be lopsided.
	// We add 2 to the vsep to give it some extra spacing which looks a bit more modern (see Windows, for example).
	const int separation_base = config.increased_margin + 6;
	config.forced_even_separation = separation_base + (separation_base % 2);

	return config;
}

// Public interface for theme generation.

Ref<AppTheme> AppThemeManager::generate_theme(const Ref<AppTheme> &p_old_theme) {
	OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Generate Theme");

	Ref<AppTheme> theme = _create_base_theme(p_old_theme);

	OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Merge Custom Theme");

	const String custom_theme_path = APP_GET("interface/theme/custom_theme");
	if (!custom_theme_path.is_empty()) {
		Ref<Theme> custom_theme = ResourceLoader::load(custom_theme_path);
		if (custom_theme.is_valid()) {
			theme->merge_with(custom_theme);
		}
	}

	OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Merge Custom Theme");

	OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Generate Theme");
	benchmark_run++;

	return theme;
}

bool AppThemeManager::is_dark_theme() {
	Color base_color = APP_GET("interface/theme/base_color");
	return base_color.get_luminance() < 0.5;
}

bool AppThemeManager::is_dark_icon_and_font() {
	int icon_font_color_setting = EDITOR_GET("interface/theme/icon_and_font_color");
	if (icon_font_color_setting == ColorMode::AUTO_COLOR) {
		return is_dark_theme();
	}

	return icon_font_color_setting == ColorMode::LIGHT_COLOR;
}

void AppThemeManager::initialize() {
	AppIcons::AppColorMap::create();
	AppTheme::initialize();
}

void AppThemeManager::finalize() {
	AppIcons::AppColorMap::finish();
	AppTheme::finalize();
}
