#include "app_theme_manager.h"

#include "core/io/resource_loader.h"

#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h"
#include "scene/resources/texture.h"

#include "scene/scene_string_names.h"

#include "servers/display_server.h"

#include "app/app_string_names.h"
#include "app/settings/app_settings.h"
#include "app/themes/app_fonts.h"
#include "app/themes/app_icons.h"

// Theme configuration.

uint32_t AppThemeManager::ThemeConfiguration::hash() {
	uint32_t hash = hash_murmur3_one_float(APP_SCALE);

	// Basic properties.

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
	hash = hash_murmur3_one_32(color_picker_button_height, hash);
	hash = hash_murmur3_one_float(subresource_hue_tint, hash);

	hash = hash_murmur3_one_float(default_contrast, hash);

	// Generated properties.

	hash = hash_murmur3_one_32((int)dark_theme, hash);

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

	hash = hash_murmur3_one_32((int)dark_theme, hash);

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

Ref<StyleBoxTexture> make_stylebox(Ref<Texture2D> p_texture, float p_left, float p_top, float p_right, float p_bottom, float p_margin_left = -1, float p_margin_top = -1, float p_margin_right = -1, float p_margin_bottom = -1, bool p_draw_center = true) {
	Ref<StyleBoxTexture> style(memnew(StyleBoxTexture));
	style->set_texture(p_texture);
	style->set_texture_margin_individual(p_left * APP_SCALE, p_top * APP_SCALE, p_right * APP_SCALE, p_bottom * APP_SCALE);
	style->set_content_margin_individual((p_left + p_margin_left) * APP_SCALE, (p_top + p_margin_top) * APP_SCALE, (p_right + p_margin_right) * APP_SCALE, (p_bottom + p_margin_bottom) * APP_SCALE);
	style->set_draw_center(p_draw_center);
	return style;
}

Ref<StyleBoxEmpty> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1, float p_margin_right = -1, float p_margin_bottom = -1) {
	Ref<StyleBoxEmpty> style(memnew(StyleBoxEmpty));
	style->set_content_margin_individual(p_margin_left * APP_SCALE, p_margin_top * APP_SCALE, p_margin_right * APP_SCALE, p_margin_bottom * APP_SCALE);
	return style;
}

Ref<StyleBoxFlat> make_flat_stylebox(Color p_color, float p_margin_left = -1, float p_margin_top = -1, float p_margin_right = -1, float p_margin_bottom = -1, int p_corner_width = 0) {
	Ref<StyleBoxFlat> style(memnew(StyleBoxFlat));
	style->set_bg_color(p_color);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(Math::ceil(0.8 * p_corner_width * APP_SCALE));
	style->set_corner_radius_all(p_corner_width * APP_SCALE);
	style->set_content_margin_individual(p_margin_left * APP_SCALE, p_margin_top * APP_SCALE, p_margin_right * APP_SCALE, p_margin_bottom * APP_SCALE);
	// Work around issue about antialiased edges being blurrier (GH-35279).
	style->set_anti_aliased(false);
	return style;
}

Ref<StyleBoxLine> make_line_stylebox(Color p_color, int p_thickness = 1, float p_grow_begin = 1, float p_grow_end = 1, bool p_vertical = false) {
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
	ThemeConfiguration config = _create_theme_config(theme);
	theme->set_generated_hash(config.hash());
	theme->set_generated_fonts_hash(config.hash_fonts());
	theme->set_generated_icons_hash(config.hash_icons());

	print_verbose(vformat("AppTheme: Generating new theme for the config '%d'.", theme->get_generated_hash()));

	_create_shared_styles(theme, config);

	// Register icons.
	{
		OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Register Icons");

		AppIcons::app_configure_icons(config.dark_theme);

		// If settings are comparable to the old theme, then just copy existing icons over.
		// Otherwise, regenerate them.
		bool keep_old_icons = (p_old_theme.is_valid() && theme->get_generated_icons_hash() == p_old_theme->get_generated_icons_hash());
		if (keep_old_icons) {
			print_verbose("AppTheme: Can keep old icons, copying.");
			AppIcons::app_copy_icons(theme, p_old_theme);
		} else {
			print_verbose("AppTheme: Generating new icons.");
			AppIcons::app_register_icons(theme, config.dark_theme, config.icon_saturation, config.thumb_size, config.gizmo_handle_scale);
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
	_populate_standard_styles(theme, config);
	_populate_app_styles(theme, config);
	// _populate_text_editor_styles(theme, config);
	// _populate_visual_shader_styles(theme, config);

	OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Create Base Theme");
	return theme;
}

AppThemeManager::ThemeConfiguration AppThemeManager::_create_theme_config(const Ref<AppTheme> &p_theme) {
	ThemeConfiguration config;

	// Basic properties.

	config.preset = APP_GET("interface/theme/preset");
	config.spacing_preset = APP_GET("interface/theme/spacing_preset");

	config.base_color = APP_GET("interface/theme/base_color");
	config.accent_color = APP_GET("interface/theme/accent_color");
	config.contrast = APP_GET("interface/theme/contrast");
	config.icon_saturation = APP_GET("interface/theme/icon_saturation");

	// Extra properties.

	config.base_spacing = APP_GET("interface/theme/base_spacing");
	config.extra_spacing = APP_GET("interface/theme/additional_spacing");
	// Ensure borders are visible when using an app scale below 100%.
	config.border_width = CLAMP((int)APP_GET("interface/theme/border_size"), 0, 2) * MAX(1, APP_SCALE);
	config.corner_radius = CLAMP((int)APP_GET("interface/theme/corner_radius"), 0, 6);

	config.draw_extra_borders = APP_GET("interface/theme/draw_extra_borders");
	config.relationship_line_opacity = APP_GET("interface/theme/relationship_line_opacity");
	config.thumb_size = APP_GET("filesystem/file_dialog/thumbnail_size");
	config.class_icon_size = 16 * APP_SCALE;
	// config.enable_touch_optimizations = APP_GET("interface/touchscreen/enable_touch_optimizations");
	// config.gizmo_handle_scale = APP_GET("interface/touchscreen/scale_gizmo_handles");
	config.color_picker_button_height = 28 * APP_SCALE;
	// config.subresource_hue_tint = APP_GET("docks/property_editor/subresource_hue_tint");

	config.default_contrast = 0.3; // Make sure to keep this in sync with the app settings definition.

	// Handle main theme preset.
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
			float preset_contrast = 0;
			bool preset_draw_extra_borders = false;

			// Please use alphabetical order if you're adding a new theme here.
			if (config.preset == "Breeze Dark") {
				preset_accent_color = Color(0.26, 0.76, 1.00);
				preset_base_color = Color(0.24, 0.26, 0.28);
				preset_contrast = config.default_contrast;
			} else if (config.preset == "Godot 2") {
				preset_accent_color = Color(0.53, 0.67, 0.89);
				preset_base_color = Color(0.24, 0.23, 0.27);
				preset_contrast = config.default_contrast;
			} else if (config.preset == "Gray") {
				preset_accent_color = Color(0.44, 0.73, 0.98);
				preset_base_color = Color(0.24, 0.24, 0.24);
				preset_contrast = config.default_contrast;
			} else if (config.preset == "Light") {
				preset_accent_color = Color(0.18, 0.50, 1.00);
				preset_base_color = Color(0.9, 0.9, 0.9);
				// A negative contrast rate looks better for light themes, since it better follows the natural order of UI "elevation".
				preset_contrast = -0.06;
			} else if (config.preset == "Solarized (Dark)") {
				preset_accent_color = Color(0.15, 0.55, 0.82);
				preset_base_color = Color(0.03, 0.21, 0.26);
				preset_contrast = 0.23;
			} else if (config.preset == "Solarized (Light)") {
				preset_accent_color = Color(0.15, 0.55, 0.82);
				preset_base_color = Color(0.89, 0.86, 0.79);
				// A negative contrast rate looks better for light themes, since it better follows the natural order of UI "elevation".
				preset_contrast = -0.06;
			} else if (config.preset == "Black (OLED)") {
				preset_accent_color = Color(0.45, 0.75, 1.0);
				preset_base_color = Color(0, 0, 0);
				// The contrast rate value is irrelevant on a fully black theme.
				preset_contrast = 0.0;
				preset_draw_extra_borders = true;
			} else { // Default
				preset_accent_color = Color(0.44, 0.73, 0.98);
				preset_base_color = Color(0.21, 0.24, 0.29);
				preset_contrast = config.default_contrast;
			}

			config.accent_color = preset_accent_color;
			config.base_color = preset_base_color;
			config.contrast = preset_contrast;
			config.draw_extra_borders = preset_draw_extra_borders;

			AppSettings::get_singleton()->set_initial_value("interface/theme/accent_color", config.accent_color);
			AppSettings::get_singleton()->set_initial_value("interface/theme/base_color", config.base_color);
			AppSettings::get_singleton()->set_initial_value("interface/theme/contrast", config.contrast);
			AppSettings::get_singleton()->set_initial_value("interface/theme/draw_extra_borders", config.draw_extra_borders);
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
	}

	// Handle theme spacing preset.
	{
		if (config.spacing_preset != "Custom") {
			int preset_base_spacing = 0;
			int preset_extra_spacing = 0;
			Size2 preset_dialogs_buttons_min_size;

			if (config.spacing_preset == "Compact") {
				preset_base_spacing = 0;
				preset_extra_spacing = 4;
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

	config.base_margin = config.base_spacing;
	config.increased_margin = config.base_spacing + config.extra_spacing;
	config.separation_margin = (config.base_spacing + config.extra_spacing / 2) * APP_SCALE;
	config.popup_margin = config.base_margin * 2.4 * APP_SCALE;
	// Make sure content doesn't stick to window decorations; this can be fixed in future with layout changes.
	config.window_border_margin = MAX(1, config.base_margin * 2);
	config.top_bar_separation = config.base_margin * 2 * APP_SCALE;

	// Force the v_separation to be even so that the spacing on top and bottom is even.
	// If the vsep is odd and cannot be split into 2 even groups (of pixels), then it will be lopsided.
	// We add 2 to the vsep to give it some extra spacing which looks a bit more modern (see Windows, for example).
	const int separation_base = config.increased_margin + 6;
	config.forced_even_separation = separation_base + (separation_base % 2);

	return config;
}

void AppThemeManager::_create_shared_styles(const Ref<AppTheme> &p_theme, ThemeConfiguration &p_config) {
	// Colors.
	{
		// Base colors.

		p_theme->set_color("base_color", AppStringName(App), p_config.base_color);
		p_theme->set_color("accent_color", AppStringName(App), p_config.accent_color);

		// White (dark theme) or black (light theme), will be used to generate the rest of the colors
		p_config.mono_color = p_config.dark_theme ? Color(1, 1, 1) : Color(0, 0, 0);

		// Ensure base colors are in the 0..1 luminance range to avoid 8-bit integer overflow or text rendering issues.
		// Some places in the editor use 8-bit integer colors.
		p_config.dark_color_1 = p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast).clamp();
		p_config.dark_color_2 = p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast * 1.5).clamp();
		p_config.dark_color_3 = p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast * 2).clamp();

		p_config.contrast_color_1 = p_config.base_color.lerp(p_config.mono_color, MAX(p_config.contrast, p_config.default_contrast));
		p_config.contrast_color_2 = p_config.base_color.lerp(p_config.mono_color, MAX(p_config.contrast * 1.5, p_config.default_contrast * 1.5));

		p_config.highlight_color = Color(p_config.accent_color.r, p_config.accent_color.g, p_config.accent_color.b, 0.275);
		p_config.highlight_disabled_color = p_config.highlight_color.lerp(p_config.dark_theme ? Color(0, 0, 0) : Color(1, 1, 1), 0.5);

		p_config.success_color = Color(0.45, 0.95, 0.5);
		p_config.warning_color = Color(1, 0.87, 0.4);
		p_config.error_color = Color(1, 0.47, 0.42);
		if (!p_config.dark_theme) {
			// Darken some colors to be readable on a light background.
			p_config.success_color = p_config.success_color.lerp(p_config.mono_color, 0.35);
			p_config.warning_color = Color(0.82, 0.56, 0.1);
			p_config.error_color = Color(0.8, 0.22, 0.22);
		}

		p_theme->set_color("mono_color", AppStringName(App), p_config.mono_color);
		p_theme->set_color("dark_color_1", AppStringName(App), p_config.dark_color_1);
		p_theme->set_color("dark_color_2", AppStringName(App), p_config.dark_color_2);
		p_theme->set_color("dark_color_3", AppStringName(App), p_config.dark_color_3);
		p_theme->set_color("contrast_color_1", AppStringName(App), p_config.contrast_color_1);
		p_theme->set_color("contrast_color_2", AppStringName(App), p_config.contrast_color_2);
		p_theme->set_color("highlight_color", AppStringName(App), p_config.highlight_color);
		p_theme->set_color("highlight_disabled_color", AppStringName(App), p_config.highlight_disabled_color);
		p_theme->set_color("success_color", AppStringName(App), p_config.success_color);
		p_theme->set_color("warning_color", AppStringName(App), p_config.warning_color);
		p_theme->set_color("error_color", AppStringName(App), p_config.error_color);

		// Only used when the Draw Extra Borders editor setting is enabled.
		p_config.extra_border_color_1 = Color(0.5, 0.5, 0.5);
		p_config.extra_border_color_2 = p_config.dark_theme ? Color(0.3, 0.3, 0.3) : Color(0.7, 0.7, 0.7);

		p_theme->set_color("extra_border_color_1", AppStringName(App), p_config.extra_border_color_1);
		p_theme->set_color("extra_border_color_2", AppStringName(App), p_config.extra_border_color_2);

		// Font colors.

		p_config.font_color = p_config.mono_color.lerp(p_config.base_color, 0.25);
		p_config.font_focus_color = p_config.mono_color.lerp(p_config.base_color, 0.125);
		p_config.font_hover_color = p_config.mono_color.lerp(p_config.base_color, 0.125);
		p_config.font_pressed_color = p_config.accent_color;
		p_config.font_hover_pressed_color = p_config.font_hover_color.lerp(p_config.accent_color, 0.74);
		p_config.font_disabled_color = Color(p_config.mono_color.r, p_config.mono_color.g, p_config.mono_color.b, 0.35);
		p_config.font_readonly_color = Color(p_config.mono_color.r, p_config.mono_color.g, p_config.mono_color.b, 0.65);
		p_config.font_placeholder_color = Color(p_config.mono_color.r, p_config.mono_color.g, p_config.mono_color.b, 0.5);
		p_config.font_outline_color = Color(0, 0, 0, 0);

		p_theme->set_color(SceneStringName(font_color), AppStringName(App), p_config.font_color);
		p_theme->set_color("font_focus_color", AppStringName(App), p_config.font_focus_color);
		p_theme->set_color("font_hover_color", AppStringName(App), p_config.font_hover_color);
		p_theme->set_color("font_pressed_color", AppStringName(App), p_config.font_pressed_color);
		p_theme->set_color("font_hover_pressed_color", AppStringName(App), p_config.font_hover_pressed_color);
		p_theme->set_color("font_disabled_color", AppStringName(App), p_config.font_disabled_color);
		p_theme->set_color("font_readonly_color", AppStringName(App), p_config.font_readonly_color);
		p_theme->set_color("font_placeholder_color", AppStringName(App), p_config.font_placeholder_color);
		p_theme->set_color("font_outline_color", AppStringName(App), p_config.font_outline_color);

		// Icon colors.

		p_config.icon_normal_color = Color(1, 1, 1);
		p_config.icon_focus_color = p_config.icon_normal_color * (p_config.dark_theme ? 1.15 : 1.45);
		p_config.icon_focus_color.a = 1.0;
		p_config.icon_hover_color = p_config.icon_focus_color;
		// Make the pressed icon color overbright because icons are not completely white on a dark theme.
		// On a light theme, icons are dark, so we need to modulate them with an even brighter color.
		p_config.icon_pressed_color = p_config.accent_color * (p_config.dark_theme ? 1.15 : 3.5);
		p_config.icon_pressed_color.a = 1.0;
		p_config.icon_disabled_color = Color(p_config.icon_normal_color, 0.4);

		p_theme->set_color("icon_normal_color", AppStringName(App), p_config.icon_normal_color);
		p_theme->set_color("icon_focus_color", AppStringName(App), p_config.icon_focus_color);
		p_theme->set_color("icon_hover_color", AppStringName(App), p_config.icon_hover_color);
		p_theme->set_color("icon_pressed_color", AppStringName(App), p_config.icon_pressed_color);
		p_theme->set_color("icon_disabled_color", AppStringName(App), p_config.icon_disabled_color);

		// Additional GUI colors.

		p_config.shadow_color = Color(0, 0, 0, p_config.dark_theme ? 0.3 : 0.1);
		p_config.selection_color = p_config.accent_color * Color(1, 1, 1, 0.4);
		p_config.disabled_border_color = p_config.mono_color.inverted().lerp(p_config.base_color, 0.7);
		p_config.disabled_bg_color = p_config.mono_color.inverted().lerp(p_config.base_color, 0.9);
		p_config.separator_color = Color(p_config.mono_color.r, p_config.mono_color.g, p_config.mono_color.b, 0.1);

		p_theme->set_color("selection_color", AppStringName(App), p_config.selection_color);
		p_theme->set_color("disabled_border_color", AppStringName(App), p_config.disabled_border_color);
		p_theme->set_color("disabled_bg_color", AppStringName(App), p_config.disabled_bg_color);
		p_theme->set_color("separator_color", AppStringName(App), p_config.separator_color);

		// Additional editor colors.

		p_theme->set_color("box_selection_fill_color", AppStringName(App), p_config.accent_color * Color(1, 1, 1, 0.3));
		p_theme->set_color("box_selection_stroke_color", AppStringName(App), p_config.accent_color * Color(1, 1, 1, 0.8));

		p_theme->set_color("axis_x_color", AppStringName(App), Color(0.96, 0.20, 0.32));
		p_theme->set_color("axis_y_color", AppStringName(App), Color(0.53, 0.84, 0.01));
		p_theme->set_color("axis_z_color", AppStringName(App), Color(0.16, 0.55, 0.96));
		p_theme->set_color("axis_w_color", AppStringName(App), Color(0.55, 0.55, 0.55));

		const float prop_color_saturation = p_config.accent_color.get_s() * 0.75;
		const float prop_color_value = p_config.accent_color.get_v();

		p_theme->set_color("property_color_x", AppStringName(App), Color().from_hsv(0.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_y", AppStringName(App), Color().from_hsv(1.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_z", AppStringName(App), Color().from_hsv(2.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_w", AppStringName(App), Color().from_hsv(1.5 / 3.0 + 0.05, prop_color_saturation, prop_color_value));

		// Special colors for rendering methods.

		p_theme->set_color("forward_plus_color", AppStringName(App), Color::hex(0x5d8c3fff));
		p_theme->set_color("mobile_color", AppStringName(App), Color::hex(0xa5557dff));
		p_theme->set_color("gl_compatibility_color", AppStringName(App), Color::hex(0x5586a4ff));

		if (p_config.dark_theme) {
			p_theme->set_color("highend_color", AppStringName(App), Color(1.0, 0.0, 0.0));
		} else {
			p_theme->set_color("highend_color", AppStringName(App), Color::hex(0xad1128ff));
		}
	}

	// Constants.
	{
		// Can't save single float in theme, so using Color.
		p_theme->set_color("icon_saturation", AppStringName(App), Color(p_config.icon_saturation, p_config.icon_saturation, p_config.icon_saturation));

		// Controls may rely on the scale for their internal drawing logic.
		p_theme->set_default_base_scale(APP_SCALE);
		p_theme->set_constant("scale", AppStringName(App), APP_SCALE);

		p_theme->set_constant("thumb_size", AppStringName(App), p_config.thumb_size);
		p_theme->set_constant("class_icon_size", AppStringName(App), p_config.class_icon_size);
		p_theme->set_constant("color_picker_button_height", AppStringName(App), p_config.color_picker_button_height);
		p_theme->set_constant("gizmo_handle_scale", AppStringName(App), p_config.gizmo_handle_scale);

		p_theme->set_constant("base_margin", AppStringName(App), p_config.base_margin);
		p_theme->set_constant("increased_margin", AppStringName(App), p_config.increased_margin);
		p_theme->set_constant("window_border_margin", AppStringName(App), p_config.window_border_margin);
		p_theme->set_constant("top_bar_separation", AppStringName(App), p_config.top_bar_separation);

		p_theme->set_constant("dark_theme", AppStringName(App), p_config.dark_theme);
	}

	// Styleboxes.
	{
		// This is the basic stylebox, used as a base for most other styleboxes (through `duplicate()`).
		p_config.base_style = make_flat_stylebox(p_config.base_color, p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.corner_radius);
		p_config.base_style->set_border_width_all(p_config.border_width);
		p_config.base_style->set_border_color(p_config.base_color);

		p_config.base_empty_style = make_empty_stylebox(p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin);

		// Button styles.
		{
			p_config.widget_margin = Vector2(p_config.increased_margin + 2, p_config.increased_margin + 1) * APP_SCALE;

			p_config.button_style = p_config.base_style->duplicate();
			p_config.button_style->set_content_margin_individual(p_config.widget_margin.x, p_config.widget_margin.y, p_config.widget_margin.x, p_config.widget_margin.y);
			p_config.button_style->set_bg_color(p_config.dark_color_1);
			if (p_config.draw_extra_borders) {
				p_config.button_style->set_border_width_all(Math::round(APP_SCALE));
				p_config.button_style->set_border_color(p_config.extra_border_color_1);
			} else {
				p_config.button_style->set_border_color(p_config.dark_color_2);
			}

			p_config.button_style_disabled = p_config.button_style->duplicate();
			p_config.button_style_disabled->set_bg_color(p_config.disabled_bg_color);
			if (p_config.draw_extra_borders) {
				p_config.button_style_disabled->set_border_color(p_config.extra_border_color_2);
			} else {
				p_config.button_style_disabled->set_border_color(p_config.disabled_border_color);
			}

			p_config.button_style_focus = p_config.button_style->duplicate();
			p_config.button_style_focus->set_draw_center(false);
			p_config.button_style_focus->set_border_width_all(Math::round(2 * MAX(1, APP_SCALE)));
			p_config.button_style_focus->set_border_color(p_config.accent_color);

			p_config.button_style_pressed = p_config.button_style->duplicate();
			p_config.button_style_pressed->set_bg_color(p_config.dark_color_1.darkened(0.125));

			p_config.button_style_hover = p_config.button_style->duplicate();
			p_config.button_style_hover->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.11));
			if (p_config.draw_extra_borders) {
				p_config.button_style_hover->set_border_color(p_config.extra_border_color_1);
			} else {
				p_config.button_style_hover->set_border_color(p_config.mono_color * Color(1, 1, 1, 0.05));
			}
		}

		// Windows and popups.
		{
			p_config.popup_style = p_config.base_style->duplicate();
			p_config.popup_style->set_content_margin_all(p_config.popup_margin);
			p_config.popup_style->set_border_color(p_config.contrast_color_1);
			p_config.popup_style->set_shadow_color(p_config.shadow_color);
			p_config.popup_style->set_shadow_size(4 * APP_SCALE);
			// Popups are separate windows by default in the editor. Windows currently don't support per-pixel transparency
			// in 4.0, and even if it was, it may not always work in practice (e.g. running with compositing disabled).
			p_config.popup_style->set_corner_radius_all(0);

			p_config.popup_border_style = p_config.popup_style->duplicate();
			p_config.popup_border_style->set_content_margin_all(MAX(Math::round(APP_SCALE), p_config.border_width) + 2 + (p_config.base_margin * 1.5) * APP_SCALE);
			// Always display a border for popups like PopupMenus so they can be distinguished from their background.
			p_config.popup_border_style->set_border_width_all(MAX(Math::round(APP_SCALE), p_config.border_width));
			if (p_config.draw_extra_borders) {
				p_config.popup_border_style->set_border_color(p_config.extra_border_color_2);
			} else {
				p_config.popup_border_style->set_border_color(p_config.dark_color_2);
			}

			p_config.window_style = p_config.popup_style->duplicate();
			p_config.window_style->set_border_color(p_config.base_color);
			p_config.window_style->set_border_width(SIDE_TOP, 24 * APP_SCALE);
			p_config.window_style->set_expand_margin(SIDE_TOP, 24 * APP_SCALE);

			// Prevent corner artifacts between window title and body.
			p_config.dialog_style = p_config.base_style->duplicate();
			p_config.dialog_style->set_corner_radius(CORNER_TOP_LEFT, 0);
			p_config.dialog_style->set_corner_radius(CORNER_TOP_RIGHT, 0);
			p_config.dialog_style->set_content_margin_all(p_config.popup_margin);
			// Prevent visible line between window title and body.
			p_config.dialog_style->set_expand_margin(SIDE_BOTTOM, 2 * APP_SCALE);
		}

		// Panels.
		{
			p_config.panel_container_style = p_config.button_style->duplicate();
			p_config.panel_container_style->set_draw_center(false);
			p_config.panel_container_style->set_border_width_all(0);

			// Content panel for tabs and similar containers.

			// Compensate for the border.
			const int content_panel_margin = p_config.base_margin * APP_SCALE + p_config.border_width;

			p_config.content_panel_style = p_config.base_style->duplicate();
			p_config.content_panel_style->set_border_color(p_config.dark_color_3);
			p_config.content_panel_style->set_border_width_all(p_config.border_width);
			p_config.content_panel_style->set_border_width(Side::SIDE_TOP, 0);
			p_config.content_panel_style->set_corner_radius(CORNER_TOP_LEFT, 0);
			p_config.content_panel_style->set_corner_radius(CORNER_TOP_RIGHT, 0);
			p_config.content_panel_style->set_content_margin_individual(content_panel_margin, 2 * APP_SCALE + content_panel_margin, content_panel_margin, content_panel_margin);

			// Trees and similarly inset panels.

			p_config.tree_panel_style = p_config.base_style->duplicate();
			// Make Trees easier to distinguish from other controls by using a darker background color.
			p_config.tree_panel_style->set_bg_color(p_config.dark_color_1.lerp(p_config.dark_color_2, 0.5));
			if (p_config.draw_extra_borders) {
				p_config.tree_panel_style->set_border_width_all(Math::round(APP_SCALE));
				p_config.tree_panel_style->set_border_color(p_config.extra_border_color_2);
			} else {
				p_config.tree_panel_style->set_border_color(p_config.dark_color_3);
			}
		}
	}
}

void AppThemeManager::_populate_standard_styles(const Ref<AppTheme> &p_theme, ThemeConfiguration &p_config) {
	// Panels.
	{
		// Panel.
		p_theme->set_stylebox(SceneStringName(panel), "Panel", make_flat_stylebox(p_config.dark_color_1, 6, 4, 6, 4, p_config.corner_radius));

		// PanelContainer.
		p_theme->set_stylebox(SceneStringName(panel), "PanelContainer", p_config.panel_container_style);

		// TooltipPanel & TooltipLabel.
		{
			// TooltipPanel is also used for custom tooltips, while TooltipLabel
			// is only relevant for default tooltips.

			p_theme->set_color(SceneStringName(font_color), "TooltipLabel", p_config.font_hover_color);
			p_theme->set_color("font_shadow_color", "TooltipLabel", Color(0, 0, 0, 0));

			Ref<StyleBoxFlat> style_tooltip = p_config.popup_style->duplicate();
			style_tooltip->set_shadow_size(0);
			style_tooltip->set_content_margin_all(p_config.base_margin * APP_SCALE * 0.5);
			style_tooltip->set_bg_color(p_config.dark_color_3 * Color(0.8, 0.8, 0.8, 0.9));
			style_tooltip->set_border_width_all(0);
			p_theme->set_stylebox(SceneStringName(panel), "TooltipPanel", style_tooltip);
		}

		// PopupPanel
		p_theme->set_stylebox(SceneStringName(panel), "PopupPanel", p_config.popup_border_style);
	}

	// Buttons.
	{
		// Button.

		p_theme->set_stylebox(CoreStringName(normal), "Button", p_config.button_style);
		p_theme->set_stylebox(SceneStringName(hover), "Button", p_config.button_style_hover);
		p_theme->set_stylebox(SceneStringName(pressed), "Button", p_config.button_style_pressed);
		p_theme->set_stylebox("focus", "Button", p_config.button_style_focus);
		p_theme->set_stylebox("disabled", "Button", p_config.button_style_disabled);

		p_theme->set_color(SceneStringName(font_color), "Button", p_config.font_color);
		p_theme->set_color("font_hover_color", "Button", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "Button", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "Button", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "Button", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "Button", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "Button", p_config.font_outline_color);

		p_theme->set_color("icon_normal_color", "Button", p_config.icon_normal_color);
		p_theme->set_color("icon_hover_color", "Button", p_config.icon_hover_color);
		p_theme->set_color("icon_focus_color", "Button", p_config.icon_focus_color);
		p_theme->set_color("icon_hover_pressed_color", "Button", p_config.icon_pressed_color);
		p_theme->set_color("icon_pressed_color", "Button", p_config.icon_pressed_color);
		p_theme->set_color("icon_disabled_color", "Button", p_config.icon_disabled_color);

		p_theme->set_constant("h_separation", "Button", 4 * APP_SCALE);
		p_theme->set_constant("outline_size", "Button", 0);

		p_theme->set_constant("align_to_largest_stylebox", "Button", 1); // Enabled.

		// MenuButton.

		p_theme->set_stylebox(CoreStringName(normal), "MenuButton", p_config.panel_container_style);
		p_theme->set_stylebox(SceneStringName(hover), "MenuButton", p_config.button_style_hover);
		p_theme->set_stylebox(SceneStringName(pressed), "MenuButton", p_config.panel_container_style);
		p_theme->set_stylebox("focus", "MenuButton", p_config.panel_container_style);
		p_theme->set_stylebox("disabled", "MenuButton", p_config.panel_container_style);

		p_theme->set_color(SceneStringName(font_color), "MenuButton", p_config.font_color);
		p_theme->set_color("font_hover_color", "MenuButton", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "MenuButton", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "MenuButton", p_config.font_focus_color);
		p_theme->set_color("font_outline_color", "MenuButton", p_config.font_outline_color);

		p_theme->set_constant("outline_size", "MenuButton", 0);

		// MenuBar.

		p_theme->set_stylebox(CoreStringName(normal), "MenuBar", p_config.button_style);
		p_theme->set_stylebox(SceneStringName(hover), "MenuBar", p_config.button_style_hover);
		p_theme->set_stylebox(SceneStringName(pressed), "MenuBar", p_config.button_style_pressed);
		p_theme->set_stylebox("disabled", "MenuBar", p_config.button_style_disabled);

		p_theme->set_color(SceneStringName(font_color), "MenuBar", p_config.font_color);
		p_theme->set_color("font_hover_color", "MenuBar", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "MenuBar", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "MenuBar", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "MenuBar", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "MenuBar", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "MenuBar", p_config.font_outline_color);

		p_theme->set_color("icon_normal_color", "MenuBar", p_config.icon_normal_color);
		p_theme->set_color("icon_hover_color", "MenuBar", p_config.icon_hover_color);
		p_theme->set_color("icon_focus_color", "MenuBar", p_config.icon_focus_color);
		p_theme->set_color("icon_pressed_color", "MenuBar", p_config.icon_pressed_color);
		p_theme->set_color("icon_disabled_color", "MenuBar", p_config.icon_disabled_color);

		p_theme->set_constant("h_separation", "MenuBar", 4 * APP_SCALE);
		p_theme->set_constant("outline_size", "MenuBar", 0);

		// OptionButton.
		{
			Ref<StyleBoxFlat> option_button_focus_style = p_config.button_style_focus->duplicate();
			Ref<StyleBoxFlat> option_button_normal_style = p_config.button_style->duplicate();
			Ref<StyleBoxFlat> option_button_hover_style = p_config.button_style_hover->duplicate();
			Ref<StyleBoxFlat> option_button_pressed_style = p_config.button_style_pressed->duplicate();
			Ref<StyleBoxFlat> option_button_disabled_style = p_config.button_style_disabled->duplicate();

			option_button_focus_style->set_content_margin(SIDE_RIGHT, 4 * APP_SCALE);
			option_button_normal_style->set_content_margin(SIDE_RIGHT, 4 * APP_SCALE);
			option_button_hover_style->set_content_margin(SIDE_RIGHT, 4 * APP_SCALE);
			option_button_pressed_style->set_content_margin(SIDE_RIGHT, 4 * APP_SCALE);
			option_button_disabled_style->set_content_margin(SIDE_RIGHT, 4 * APP_SCALE);

			p_theme->set_stylebox("focus", "OptionButton", option_button_focus_style);
			p_theme->set_stylebox(CoreStringName(normal), "OptionButton", p_config.button_style);
			p_theme->set_stylebox(SceneStringName(hover), "OptionButton", p_config.button_style_hover);
			p_theme->set_stylebox(SceneStringName(pressed), "OptionButton", p_config.button_style_pressed);
			p_theme->set_stylebox("disabled", "OptionButton", p_config.button_style_disabled);

			p_theme->set_stylebox("normal_mirrored", "OptionButton", option_button_normal_style);
			p_theme->set_stylebox("hover_mirrored", "OptionButton", option_button_hover_style);
			p_theme->set_stylebox("pressed_mirrored", "OptionButton", option_button_pressed_style);
			p_theme->set_stylebox("disabled_mirrored", "OptionButton", option_button_disabled_style);

			p_theme->set_color(SceneStringName(font_color), "OptionButton", p_config.font_color);
			p_theme->set_color("font_hover_color", "OptionButton", p_config.font_hover_color);
			p_theme->set_color("font_hover_pressed_color", "OptionButton", p_config.font_hover_pressed_color);
			p_theme->set_color("font_focus_color", "OptionButton", p_config.font_focus_color);
			p_theme->set_color("font_pressed_color", "OptionButton", p_config.font_pressed_color);
			p_theme->set_color("font_disabled_color", "OptionButton", p_config.font_disabled_color);
			p_theme->set_color("font_outline_color", "OptionButton", p_config.font_outline_color);

			p_theme->set_color("icon_normal_color", "OptionButton", p_config.icon_normal_color);
			p_theme->set_color("icon_hover_color", "OptionButton", p_config.icon_hover_color);
			p_theme->set_color("icon_focus_color", "OptionButton", p_config.icon_focus_color);
			p_theme->set_color("icon_pressed_color", "OptionButton", p_config.icon_pressed_color);
			p_theme->set_color("icon_disabled_color", "OptionButton", p_config.icon_disabled_color);

			p_theme->set_icon("arrow", "OptionButton", p_theme->get_icon(SNAME("GuiOptionArrow"), AppStringName(AppIcons)));
			p_theme->set_constant("arrow_margin", "OptionButton", p_config.widget_margin.x - 2 * APP_SCALE);
			p_theme->set_constant("modulate_arrow", "OptionButton", true);
			p_theme->set_constant("h_separation", "OptionButton", 4 * APP_SCALE);
			p_theme->set_constant("outline_size", "OptionButton", 0);
		}

		// CheckButton.

		p_theme->set_stylebox(CoreStringName(normal), "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox(SceneStringName(pressed), "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("disabled", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox(SceneStringName(hover), "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("hover_pressed", "CheckButton", p_config.panel_container_style);

		p_theme->set_icon("checked", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOn"), AppStringName(AppIcons)));
		p_theme->set_icon("checked_disabled", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOnDisabled"), AppStringName(AppIcons)));
		p_theme->set_icon("unchecked", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOff"), AppStringName(AppIcons)));
		p_theme->set_icon("unchecked_disabled", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOffDisabled"), AppStringName(AppIcons)));

		p_theme->set_icon("checked_mirrored", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOnMirrored"), AppStringName(AppIcons)));
		p_theme->set_icon("checked_disabled_mirrored", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOnDisabledMirrored"), AppStringName(AppIcons)));
		p_theme->set_icon("unchecked_mirrored", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOffMirrored"), AppStringName(AppIcons)));
		p_theme->set_icon("unchecked_disabled_mirrored", "CheckButton", p_theme->get_icon(SNAME("GuiToggleOffDisabledMirrored"), AppStringName(AppIcons)));

		p_theme->set_color(SceneStringName(font_color), "CheckButton", p_config.font_color);
		p_theme->set_color("font_hover_color", "CheckButton", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "CheckButton", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "CheckButton", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "CheckButton", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "CheckButton", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "CheckButton", p_config.font_outline_color);

		p_theme->set_color("icon_normal_color", "CheckButton", p_config.icon_normal_color);
		p_theme->set_color("icon_hover_color", "CheckButton", p_config.icon_hover_color);
		p_theme->set_color("icon_focus_color", "CheckButton", p_config.icon_focus_color);
		p_theme->set_color("icon_pressed_color", "CheckButton", p_config.icon_pressed_color);
		p_theme->set_color("icon_disabled_color", "CheckButton", p_config.icon_disabled_color);

		p_theme->set_constant("h_separation", "CheckButton", 8 * APP_SCALE);
		p_theme->set_constant("check_v_offset", "CheckButton", 0);
		p_theme->set_constant("outline_size", "CheckButton", 0);

		// CheckBox.
		{
			Ref<StyleBoxFlat> checkbox_style = p_config.panel_container_style->duplicate();

			p_theme->set_stylebox(CoreStringName(normal), "CheckBox", checkbox_style);
			p_theme->set_stylebox(SceneStringName(pressed), "CheckBox", checkbox_style);
			p_theme->set_stylebox("disabled", "CheckBox", checkbox_style);
			p_theme->set_stylebox(SceneStringName(hover), "CheckBox", checkbox_style);
			p_theme->set_stylebox("hover_pressed", "CheckBox", checkbox_style);
			p_theme->set_icon("checked", "CheckBox", p_theme->get_icon(SNAME("GuiChecked"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked", "CheckBox", p_theme->get_icon(SNAME("GuiUnchecked"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_checked", "CheckBox", p_theme->get_icon(SNAME("GuiRadioChecked"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_unchecked", "CheckBox", p_theme->get_icon(SNAME("GuiRadioUnchecked"), AppStringName(AppIcons)));
			p_theme->set_icon("checked_disabled", "CheckBox", p_theme->get_icon(SNAME("GuiCheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked_disabled", "CheckBox", p_theme->get_icon(SNAME("GuiUncheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_checked_disabled", "CheckBox", p_theme->get_icon(SNAME("GuiRadioCheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_unchecked_disabled", "CheckBox", p_theme->get_icon(SNAME("GuiRadioUncheckedDisabled"), AppStringName(AppIcons)));

			p_theme->set_color(SceneStringName(font_color), "CheckBox", p_config.font_color);
			p_theme->set_color("font_hover_color", "CheckBox", p_config.font_hover_color);
			p_theme->set_color("font_hover_pressed_color", "CheckBox", p_config.font_hover_pressed_color);
			p_theme->set_color("font_focus_color", "CheckBox", p_config.font_focus_color);
			p_theme->set_color("font_pressed_color", "CheckBox", p_config.font_pressed_color);
			p_theme->set_color("font_disabled_color", "CheckBox", p_config.font_disabled_color);
			p_theme->set_color("font_outline_color", "CheckBox", p_config.font_outline_color);

			p_theme->set_color("icon_normal_color", "CheckBox", p_config.icon_normal_color);
			p_theme->set_color("icon_hover_color", "CheckBox", p_config.icon_hover_color);
			p_theme->set_color("icon_focus_color", "CheckBox", p_config.icon_focus_color);
			p_theme->set_color("icon_pressed_color", "CheckBox", p_config.icon_pressed_color);
			p_theme->set_color("icon_disabled_color", "CheckBox", p_config.icon_disabled_color);

			p_theme->set_constant("h_separation", "CheckBox", 8 * APP_SCALE);
			p_theme->set_constant("check_v_offset", "CheckBox", 0);
			p_theme->set_constant("outline_size", "CheckBox", 0);
		}

		// LinkButton.

		p_theme->set_stylebox("focus", "LinkButton", p_config.base_empty_style);
		p_theme->set_color(SceneStringName(font_color), "LinkButton", p_config.font_color);
		p_theme->set_color("font_hover_color", "LinkButton", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "LinkButton", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "LinkButton", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "LinkButton", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "LinkButton", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "LinkButton", p_config.font_outline_color);

		p_theme->set_constant("outline_size", "LinkButton", 0);
	}

	// Tree & ItemList.
	{
		Ref<StyleBoxFlat> style_tree_focus = p_config.base_style->duplicate();
		style_tree_focus->set_bg_color(p_config.highlight_color);
		style_tree_focus->set_border_width_all(0);

		Ref<StyleBoxFlat> style_tree_selected = style_tree_focus->duplicate();

		const Color guide_color = p_config.mono_color * Color(1, 1, 1, 0.05);

		// Tree.
		{
			p_theme->set_icon("checked", "Tree", p_theme->get_icon(SNAME("GuiChecked"), AppStringName(AppIcons)));
			p_theme->set_icon("checked_disabled", "Tree", p_theme->get_icon(SNAME("GuiCheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("indeterminate", "Tree", p_theme->get_icon(SNAME("GuiIndeterminate"), AppStringName(AppIcons)));
			p_theme->set_icon("indeterminate_disabled", "Tree", p_theme->get_icon(SNAME("GuiIndeterminateDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked", "Tree", p_theme->get_icon(SNAME("GuiUnchecked"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked_disabled", "Tree", p_theme->get_icon(SNAME("GuiUncheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("arrow", "Tree", p_theme->get_icon(SNAME("GuiTreeArrowDown"), AppStringName(AppIcons)));
			p_theme->set_icon("arrow_collapsed", "Tree", p_theme->get_icon(SNAME("GuiTreeArrowRight"), AppStringName(AppIcons)));
			p_theme->set_icon("arrow_collapsed_mirrored", "Tree", p_theme->get_icon(SNAME("GuiTreeArrowLeft"), AppStringName(AppIcons)));
			p_theme->set_icon("updown", "Tree", p_theme->get_icon(SNAME("GuiTreeUpdown"), AppStringName(AppIcons)));
			p_theme->set_icon("select_arrow", "Tree", p_theme->get_icon(SNAME("GuiDropdown"), AppStringName(AppIcons)));

			p_theme->set_stylebox(SceneStringName(panel), "Tree", p_config.tree_panel_style);
			p_theme->set_stylebox("focus", "Tree", p_config.button_style_focus);
			p_theme->set_stylebox("custom_button", "Tree", make_empty_stylebox());
			p_theme->set_stylebox("custom_button_pressed", "Tree", make_empty_stylebox());
			p_theme->set_stylebox("custom_button_hover", "Tree", p_config.button_style);

			p_theme->set_color("custom_button_font_highlight", "Tree", p_config.font_hover_color);
			p_theme->set_color(SceneStringName(font_color), "Tree", p_config.font_color);
			p_theme->set_color("font_hovered_color", "Tree", p_config.mono_color);
			p_theme->set_color("font_hovered_dimmed_color", "Tree", p_config.font_color);
			p_theme->set_color("font_hovered_selected_color", "Tree", p_config.mono_color);
			p_theme->set_color("font_selected_color", "Tree", p_config.mono_color);
			p_theme->set_color("font_disabled_color", "Tree", p_config.font_disabled_color);
			p_theme->set_color("font_outline_color", "Tree", p_config.font_outline_color);
			p_theme->set_color("title_button_color", "Tree", p_config.font_color);
			p_theme->set_color("drop_position_color", "Tree", p_config.accent_color);

			p_theme->set_constant("v_separation", "Tree", p_config.separation_margin);
			p_theme->set_constant("h_separation", "Tree", (p_config.increased_margin + 2) * APP_SCALE);
			p_theme->set_constant("guide_width", "Tree", p_config.border_width);
			p_theme->set_constant("item_margin", "Tree", MAX(3 * p_config.increased_margin * APP_SCALE, 12 * APP_SCALE));
			p_theme->set_constant("inner_item_margin_top", "Tree", p_config.separation_margin);
			p_theme->set_constant("inner_item_margin_bottom", "Tree", p_config.separation_margin);
			p_theme->set_constant("inner_item_margin_left", "Tree", p_config.increased_margin * APP_SCALE);
			p_theme->set_constant("inner_item_margin_right", "Tree", p_config.increased_margin * APP_SCALE);
			p_theme->set_constant("button_margin", "Tree", p_config.base_margin * APP_SCALE);
			p_theme->set_constant("scroll_border", "Tree", 40 * APP_SCALE);
			p_theme->set_constant("scroll_speed", "Tree", 12);
			p_theme->set_constant("outline_size", "Tree", 0);
			p_theme->set_constant("scrollbar_margin_left", "Tree", 0);
			p_theme->set_constant("scrollbar_margin_top", "Tree", 0);
			p_theme->set_constant("scrollbar_margin_right", "Tree", 0);
			p_theme->set_constant("scrollbar_margin_bottom", "Tree", 0);
			p_theme->set_constant("scrollbar_h_separation", "Tree", 1 * APP_SCALE);
			p_theme->set_constant("scrollbar_v_separation", "Tree", 1 * APP_SCALE);

			Color relationship_line_color = p_config.mono_color * Color(1, 1, 1, p_config.relationship_line_opacity);

			p_theme->set_constant("draw_guides", "Tree", p_config.relationship_line_opacity < 0.01);
			p_theme->set_color("guide_color", "Tree", guide_color);

			int relationship_line_width = 1;
			Color parent_line_color = p_config.mono_color * Color(1, 1, 1, CLAMP(p_config.relationship_line_opacity + 0.45, 0.0, 1.0));
			Color children_line_color = p_config.mono_color * Color(1, 1, 1, CLAMP(p_config.relationship_line_opacity + 0.25, 0.0, 1.0));

			p_theme->set_constant("draw_relationship_lines", "Tree", p_config.relationship_line_opacity >= 0.01);
			p_theme->set_constant("relationship_line_width", "Tree", relationship_line_width);
			p_theme->set_constant("parent_hl_line_width", "Tree", relationship_line_width * 2);
			p_theme->set_constant("children_hl_line_width", "Tree", relationship_line_width);
			p_theme->set_constant("parent_hl_line_margin", "Tree", relationship_line_width * 3);
			p_theme->set_color("relationship_line_color", "Tree", relationship_line_color);
			p_theme->set_color("parent_hl_line_color", "Tree", parent_line_color);
			p_theme->set_color("children_hl_line_color", "Tree", children_line_color);
			p_theme->set_color("drop_position_color", "Tree", p_config.accent_color);

			Ref<StyleBoxFlat> style_tree_btn = p_config.base_style->duplicate();
			style_tree_btn->set_bg_color(p_config.highlight_color);
			style_tree_btn->set_border_width_all(0);
			p_theme->set_stylebox("button_pressed", "Tree", style_tree_btn);

			Ref<StyleBoxFlat> style_tree_hover = p_config.base_style->duplicate();
			style_tree_hover->set_bg_color(p_config.highlight_color * Color(1, 1, 1, 0.4));
			style_tree_hover->set_border_width_all(0);
			p_theme->set_stylebox("hovered", "Tree", style_tree_hover);
			p_theme->set_stylebox("button_hover", "Tree", style_tree_hover);

			Ref<StyleBoxFlat> style_tree_hover_dimmed = p_config.base_style->duplicate();
			style_tree_hover_dimmed->set_bg_color(p_config.highlight_color * Color(1, 1, 1, 0.2));
			style_tree_hover_dimmed->set_border_width_all(0);
			p_theme->set_stylebox("hovered_dimmed", "Tree", style_tree_hover_dimmed);

			Ref<StyleBoxFlat> style_tree_hover_selected = style_tree_selected->duplicate();
			style_tree_hover_selected->set_bg_color(p_config.highlight_color * Color(1, 1, 1, 1.2));
			style_tree_hover_selected->set_border_width_all(0);

			p_theme->set_stylebox("hovered_selected", "Tree", style_tree_hover_selected);
			p_theme->set_stylebox("hovered_selected_focus", "Tree", style_tree_hover_selected);

			p_theme->set_stylebox("selected_focus", "Tree", style_tree_focus);
			p_theme->set_stylebox("selected", "Tree", style_tree_selected);

			Ref<StyleBoxFlat> style_tree_cursor = p_config.base_style->duplicate();
			style_tree_cursor->set_draw_center(false);
			style_tree_cursor->set_border_width_all(MAX(1, p_config.border_width));
			style_tree_cursor->set_border_color(p_config.contrast_color_1);

			Ref<StyleBoxFlat> style_tree_title = p_config.base_style->duplicate();
			style_tree_title->set_bg_color(p_config.dark_color_3);
			style_tree_title->set_border_width_all(0);
			p_theme->set_stylebox("cursor", "Tree", style_tree_cursor);
			p_theme->set_stylebox("cursor_unfocused", "Tree", style_tree_cursor);
			p_theme->set_stylebox("title_button_normal", "Tree", style_tree_title);
			p_theme->set_stylebox("title_button_hover", "Tree", style_tree_title);
			p_theme->set_stylebox("title_button_pressed", "Tree", style_tree_title);
		}

		// ItemList.
		{
			Ref<StyleBoxFlat> style_itemlist_bg = p_config.base_style->duplicate();
			style_itemlist_bg->set_content_margin_all(p_config.separation_margin);
			style_itemlist_bg->set_bg_color(p_config.dark_color_1);

			if (p_config.draw_extra_borders) {
				style_itemlist_bg->set_border_width_all(Math::round(APP_SCALE));
				style_itemlist_bg->set_border_color(p_config.extra_border_color_2);
			} else {
				style_itemlist_bg->set_border_width_all(p_config.border_width);
				style_itemlist_bg->set_border_color(p_config.dark_color_3);
			}

			Ref<StyleBoxFlat> style_itemlist_cursor = p_config.base_style->duplicate();
			style_itemlist_cursor->set_draw_center(false);
			style_itemlist_cursor->set_border_width_all(MAX(1 * APP_SCALE, p_config.border_width));
			style_itemlist_cursor->set_border_color(p_config.highlight_color);

			Ref<StyleBoxFlat> style_itemlist_hover = style_tree_selected->duplicate();
			style_itemlist_hover->set_bg_color(p_config.highlight_color * Color(1, 1, 1, 0.3));
			style_itemlist_hover->set_border_width_all(0);

			Ref<StyleBoxFlat> style_itemlist_hover_selected = style_tree_selected->duplicate();
			style_itemlist_hover_selected->set_bg_color(p_config.highlight_color * Color(1, 1, 1, 1.2));
			style_itemlist_hover_selected->set_border_width_all(0);

			p_theme->set_stylebox(SceneStringName(panel), "ItemList", style_itemlist_bg);
			p_theme->set_stylebox("focus", "ItemList", p_config.button_style_focus);
			p_theme->set_stylebox("cursor", "ItemList", style_itemlist_cursor);
			p_theme->set_stylebox("cursor_unfocused", "ItemList", style_itemlist_cursor);
			p_theme->set_stylebox("selected_focus", "ItemList", style_tree_focus);
			p_theme->set_stylebox("selected", "ItemList", style_tree_selected);
			p_theme->set_stylebox("hovered", "ItemList", style_itemlist_hover);
			p_theme->set_stylebox("hovered_selected", "ItemList", style_itemlist_hover_selected);
			p_theme->set_stylebox("hovered_selected_focus", "ItemList", style_itemlist_hover_selected);
			p_theme->set_color(SceneStringName(font_color), "ItemList", p_config.font_color);
			p_theme->set_color("font_hovered_color", "ItemList", p_config.mono_color);
			p_theme->set_color("font_hovered_selected_color", "ItemList", p_config.mono_color);
			p_theme->set_color("font_selected_color", "ItemList", p_config.mono_color);
			p_theme->set_color("font_outline_color", "ItemList", p_config.font_outline_color);
			p_theme->set_color("guide_color", "ItemList", Color(1, 1, 1, 0));
			p_theme->set_constant("v_separation", "ItemList", p_config.forced_even_separation * APP_SCALE);
			p_theme->set_constant("h_separation", "ItemList", (p_config.increased_margin + 2) * APP_SCALE);
			p_theme->set_constant("icon_margin", "ItemList", (p_config.increased_margin + 2) * APP_SCALE);
			p_theme->set_constant(SceneStringName(line_separation), "ItemList", p_config.separation_margin);
			p_theme->set_constant("outline_size", "ItemList", 0);
		}
	}

	// TabBar & TabContainer.
	{
		Ref<StyleBoxFlat> style_tab_base = p_config.button_style->duplicate();

		style_tab_base->set_border_width_all(0);
		// Don't round the top corners to avoid creating a small blank space between the tabs and the main panel.
		// This also makes the top highlight look better.
		style_tab_base->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
		style_tab_base->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);

		// When using a border width greater than 0, visually line up the left of the selected tab with the underlying panel.
		style_tab_base->set_expand_margin(SIDE_LEFT, -p_config.border_width);

		style_tab_base->set_content_margin(SIDE_LEFT, p_config.widget_margin.x + 5 * APP_SCALE);
		style_tab_base->set_content_margin(SIDE_RIGHT, p_config.widget_margin.x + 5 * APP_SCALE);
		style_tab_base->set_content_margin(SIDE_BOTTOM, p_config.widget_margin.y);
		style_tab_base->set_content_margin(SIDE_TOP, p_config.widget_margin.y);

		Ref<StyleBoxFlat> style_tab_selected = style_tab_base->duplicate();

		style_tab_selected->set_bg_color(p_config.base_color);
		// Add a highlight line at the top of the selected tab.
		style_tab_selected->set_border_width(SIDE_TOP, Math::round(2 * APP_SCALE));
		// Make the highlight line prominent, but not too prominent as to not be distracting.
		Color tab_highlight = p_config.dark_color_2.lerp(p_config.accent_color, 0.75);
		style_tab_selected->set_border_color(tab_highlight);
		style_tab_selected->set_corner_radius_all(0);

		Ref<StyleBoxFlat> style_tab_hovered = style_tab_base->duplicate();

		style_tab_hovered->set_bg_color(p_config.dark_color_1.lerp(p_config.base_color, 0.4));
		// Hovered tab has a subtle highlight between normal and selected states.
		style_tab_hovered->set_corner_radius_all(0);

		Ref<StyleBoxFlat> style_tab_unselected = style_tab_base->duplicate();
		style_tab_unselected->set_expand_margin(SIDE_BOTTOM, 0);
		style_tab_unselected->set_bg_color(p_config.dark_color_1);
		// Add some spacing between unselected tabs to make them easier to distinguish from each other
		style_tab_unselected->set_border_color(Color(0, 0, 0, 0));

		Ref<StyleBoxFlat> style_tab_disabled = style_tab_base->duplicate();
		style_tab_disabled->set_expand_margin(SIDE_BOTTOM, 0);
		style_tab_disabled->set_bg_color(p_config.disabled_bg_color);
		style_tab_disabled->set_border_color(p_config.disabled_bg_color);

		Ref<StyleBoxFlat> style_tab_focus = p_config.button_style_focus->duplicate();

		Ref<StyleBoxFlat> style_tabbar_background = make_flat_stylebox(p_config.dark_color_1, 0, 0, 0, 0, p_config.corner_radius * APP_SCALE);
		style_tabbar_background->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
		style_tabbar_background->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
		p_theme->set_stylebox("tabbar_background", "TabContainer", style_tabbar_background);
		p_theme->set_stylebox(SceneStringName(panel), "TabContainer", p_config.content_panel_style);

		p_theme->set_stylebox("tab_selected", "TabContainer", style_tab_selected);
		p_theme->set_stylebox("tab_hovered", "TabContainer", style_tab_hovered);
		p_theme->set_stylebox("tab_unselected", "TabContainer", style_tab_unselected);
		p_theme->set_stylebox("tab_disabled", "TabContainer", style_tab_disabled);
		p_theme->set_stylebox("tab_focus", "TabContainer", style_tab_focus);
		p_theme->set_stylebox("tab_selected", "TabBar", style_tab_selected);
		p_theme->set_stylebox("tab_hovered", "TabBar", style_tab_hovered);
		p_theme->set_stylebox("tab_unselected", "TabBar", style_tab_unselected);
		p_theme->set_stylebox("tab_disabled", "TabBar", style_tab_disabled);
		p_theme->set_stylebox("tab_focus", "TabBar", style_tab_focus);
		p_theme->set_stylebox("button_pressed", "TabBar", p_config.panel_container_style);
		p_theme->set_stylebox("button_highlight", "TabBar", p_config.panel_container_style);

		p_theme->set_color("font_selected_color", "TabContainer", p_config.font_color);
		p_theme->set_color("font_hovered_color", "TabContainer", p_config.font_color);
		p_theme->set_color("font_unselected_color", "TabContainer", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "TabContainer", p_config.font_outline_color);
		p_theme->set_color("font_selected_color", "TabBar", p_config.font_color);
		p_theme->set_color("font_hovered_color", "TabBar", p_config.font_color);
		p_theme->set_color("font_unselected_color", "TabBar", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "TabBar", p_config.font_outline_color);
		p_theme->set_color("drop_mark_color", "TabContainer", tab_highlight);
		p_theme->set_color("drop_mark_color", "TabBar", tab_highlight);

		p_theme->set_icon("menu", "TabContainer", p_theme->get_icon(SNAME("GuiTabMenu"), AppStringName(AppIcons)));
		p_theme->set_icon("menu_highlight", "TabContainer", p_theme->get_icon(SNAME("GuiTabMenuHl"), AppStringName(AppIcons)));
		p_theme->set_icon("close", "TabBar", p_theme->get_icon(SNAME("GuiClose"), AppStringName(AppIcons)));
		p_theme->set_icon("increment", "TabContainer", p_theme->get_icon(SNAME("GuiScrollArrowRight"), AppStringName(AppIcons)));
		p_theme->set_icon("decrement", "TabContainer", p_theme->get_icon(SNAME("GuiScrollArrowLeft"), AppStringName(AppIcons)));
		p_theme->set_icon("increment", "TabBar", p_theme->get_icon(SNAME("GuiScrollArrowRight"), AppStringName(AppIcons)));
		p_theme->set_icon("decrement", "TabBar", p_theme->get_icon(SNAME("GuiScrollArrowLeft"), AppStringName(AppIcons)));
		p_theme->set_icon("increment_highlight", "TabBar", p_theme->get_icon(SNAME("GuiScrollArrowRightHl"), AppStringName(AppIcons)));
		p_theme->set_icon("decrement_highlight", "TabBar", p_theme->get_icon(SNAME("GuiScrollArrowLeftHl"), AppStringName(AppIcons)));
		p_theme->set_icon("increment_highlight", "TabContainer", p_theme->get_icon(SNAME("GuiScrollArrowRightHl"), AppStringName(AppIcons)));
		p_theme->set_icon("decrement_highlight", "TabContainer", p_theme->get_icon(SNAME("GuiScrollArrowLeftHl"), AppStringName(AppIcons)));
		p_theme->set_icon("drop_mark", "TabContainer", p_theme->get_icon(SNAME("GuiTabDropMark"), AppStringName(AppIcons)));
		p_theme->set_icon("drop_mark", "TabBar", p_theme->get_icon(SNAME("GuiTabDropMark"), AppStringName(AppIcons)));

		p_theme->set_constant("side_margin", "TabContainer", 0);
		p_theme->set_constant("outline_size", "TabContainer", 0);
		p_theme->set_constant("h_separation", "TabBar", 4 * APP_SCALE);
		p_theme->set_constant("outline_size", "TabBar", 0);
	}

	// Separators.
	p_theme->set_stylebox("separator", "HSeparator", make_line_stylebox(p_config.separator_color, MAX(Math::round(APP_SCALE), p_config.border_width)));
	p_theme->set_stylebox("separator", "VSeparator", make_line_stylebox(p_config.separator_color, MAX(Math::round(APP_SCALE), p_config.border_width), 0, 0, true));

	// LineEdit & TextEdit.
	{
		Ref<StyleBoxFlat> text_editor_style = p_config.button_style->duplicate();

		// Don't round the bottom corners to make the line look sharper.
		text_editor_style->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
		text_editor_style->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);

		if (p_config.draw_extra_borders) {
			text_editor_style->set_border_width_all(Math::round(APP_SCALE));
			text_editor_style->set_border_color(p_config.extra_border_color_1);
		} else {
			// Add a bottom line to make LineEdits more visible, especially in sectioned inspectors
			// such as the Project Settings.
			text_editor_style->set_border_width(SIDE_BOTTOM, Math::round(2 * APP_SCALE));
			text_editor_style->set_border_color(p_config.dark_color_2);
		}

		Ref<StyleBoxFlat> text_editor_disabled_style = text_editor_style->duplicate();
		text_editor_disabled_style->set_border_color(p_config.disabled_border_color);
		text_editor_disabled_style->set_bg_color(p_config.disabled_bg_color);

		// LineEdit.

		p_theme->set_stylebox(CoreStringName(normal), "LineEdit", text_editor_style);
		p_theme->set_stylebox("focus", "LineEdit", p_config.button_style_focus);
		p_theme->set_stylebox("read_only", "LineEdit", text_editor_disabled_style);

		p_theme->set_icon("clear", "LineEdit", p_theme->get_icon(SNAME("GuiClose"), AppStringName(AppIcons)));

		p_theme->set_color(SceneStringName(font_color), "LineEdit", p_config.font_color);
		p_theme->set_color("font_selected_color", "LineEdit", p_config.mono_color);
		p_theme->set_color("font_uneditable_color", "LineEdit", p_config.font_readonly_color);
		p_theme->set_color("font_placeholder_color", "LineEdit", p_config.font_placeholder_color);
		p_theme->set_color("font_outline_color", "LineEdit", p_config.font_outline_color);
		p_theme->set_color("caret_color", "LineEdit", p_config.font_color);
		p_theme->set_color("selection_color", "LineEdit", p_config.selection_color);
		p_theme->set_color("clear_button_color", "LineEdit", p_config.font_color);
		p_theme->set_color("clear_button_color_pressed", "LineEdit", p_config.accent_color);

		p_theme->set_constant("minimum_character_width", "LineEdit", 4);
		p_theme->set_constant("outline_size", "LineEdit", 0);
		p_theme->set_constant("caret_width", "LineEdit", 1);

		// TextEdit.

		p_theme->set_stylebox(CoreStringName(normal), "TextEdit", text_editor_style);
		p_theme->set_stylebox("focus", "TextEdit", p_config.button_style_focus);
		p_theme->set_stylebox("read_only", "TextEdit", text_editor_disabled_style);

		p_theme->set_icon("tab", "TextEdit", p_theme->get_icon(SNAME("GuiTab"), AppStringName(AppIcons)));
		p_theme->set_icon("space", "TextEdit", p_theme->get_icon(SNAME("GuiSpace"), AppStringName(AppIcons)));

		p_theme->set_color(SceneStringName(font_color), "TextEdit", p_config.font_color);
		p_theme->set_color("font_readonly_color", "TextEdit", p_config.font_readonly_color);
		p_theme->set_color("font_placeholder_color", "TextEdit", p_config.font_placeholder_color);
		p_theme->set_color("font_outline_color", "TextEdit", p_config.font_outline_color);
		p_theme->set_color("caret_color", "TextEdit", p_config.font_color);
		p_theme->set_color("selection_color", "TextEdit", p_config.selection_color);
		p_theme->set_color("background_color", "TextEdit", Color(0, 0, 0, 0));

		p_theme->set_constant("line_spacing", "TextEdit", 4 * APP_SCALE);
		p_theme->set_constant("outline_size", "TextEdit", 0);
		p_theme->set_constant("caret_width", "TextEdit", 1);
	}

	// Containers.
	{
		p_theme->set_constant("separation", "BoxContainer", p_config.separation_margin);
		p_theme->set_constant("separation", "HBoxContainer", p_config.separation_margin);
		p_theme->set_constant("separation", "VBoxContainer", p_config.separation_margin);
		p_theme->set_constant("margin_left", "MarginContainer", 0);
		p_theme->set_constant("margin_top", "MarginContainer", 0);
		p_theme->set_constant("margin_right", "MarginContainer", 0);
		p_theme->set_constant("margin_bottom", "MarginContainer", 0);
		p_theme->set_constant("h_separation", "GridContainer", p_config.separation_margin);
		p_theme->set_constant("v_separation", "GridContainer", p_config.separation_margin);
		p_theme->set_constant("h_separation", "FlowContainer", p_config.separation_margin);
		p_theme->set_constant("v_separation", "FlowContainer", p_config.separation_margin);
		p_theme->set_constant("h_separation", "HFlowContainer", p_config.separation_margin);
		p_theme->set_constant("v_separation", "HFlowContainer", p_config.separation_margin);
		p_theme->set_constant("h_separation", "VFlowContainer", p_config.separation_margin);
		p_theme->set_constant("v_separation", "VFlowContainer", p_config.separation_margin);

		// SplitContainer.

		p_theme->set_icon("h_grabber", "SplitContainer", p_theme->get_icon(SNAME("GuiHsplitter"), AppStringName(AppIcons)));
		p_theme->set_icon("v_grabber", "SplitContainer", p_theme->get_icon(SNAME("GuiVsplitter"), AppStringName(AppIcons)));
		p_theme->set_icon("grabber", "VSplitContainer", p_theme->get_icon(SNAME("GuiVsplitter"), AppStringName(AppIcons)));
		p_theme->set_icon("grabber", "HSplitContainer", p_theme->get_icon(SNAME("GuiHsplitter"), AppStringName(AppIcons)));

		p_theme->set_constant("separation", "SplitContainer", p_config.separation_margin);
		p_theme->set_constant("separation", "HSplitContainer", p_config.separation_margin);
		p_theme->set_constant("separation", "VSplitContainer", p_config.separation_margin);

		p_theme->set_constant("minimum_grab_thickness", "SplitContainer", p_config.increased_margin * APP_SCALE);
		p_theme->set_constant("minimum_grab_thickness", "HSplitContainer", p_config.increased_margin * APP_SCALE);
		p_theme->set_constant("minimum_grab_thickness", "VSplitContainer", p_config.increased_margin * APP_SCALE);

		// GridContainer.
		p_theme->set_constant("v_separation", "GridContainer", Math::round(p_config.widget_margin.y - 2 * APP_SCALE));
	}

	// Window and dialogs.
	{
		// Window.

		p_theme->set_stylebox("embedded_border", "Window", p_config.window_style);
		p_theme->set_stylebox("embedded_unfocused_border", "Window", p_config.window_style);

		p_theme->set_color("title_color", "Window", p_config.font_color);
		p_theme->set_icon("close", "Window", p_theme->get_icon(SNAME("GuiClose"), AppStringName(AppIcons)));
		p_theme->set_icon("close_pressed", "Window", p_theme->get_icon(SNAME("GuiClose"), AppStringName(AppIcons)));
		p_theme->set_constant("close_h_offset", "Window", 22 * APP_SCALE);
		p_theme->set_constant("close_v_offset", "Window", 20 * APP_SCALE);
		p_theme->set_constant("title_height", "Window", 24 * APP_SCALE);
		p_theme->set_constant("resize_margin", "Window", 4 * APP_SCALE);
		p_theme->set_font("title_font", "Window", p_theme->get_font(SNAME("title"), AppStringName(AppFonts)));
		p_theme->set_font_size("title_font_size", "Window", p_theme->get_font_size(SNAME("title_size"), AppStringName(AppFonts)));

		// AcceptDialog.
		p_theme->set_stylebox(SceneStringName(panel), "AcceptDialog", p_config.dialog_style);
		p_theme->set_constant("buttons_separation", "AcceptDialog", 8 * APP_SCALE);
		// Make buttons with short texts such as "OK" easier to click/tap.
		p_theme->set_constant("buttons_min_width", "AcceptDialog", p_config.dialogs_buttons_min_size.x * APP_SCALE);
		p_theme->set_constant("buttons_min_height", "AcceptDialog", p_config.dialogs_buttons_min_size.y * APP_SCALE);

		// FileDialog.
		p_theme->set_icon("folder", "FileDialog", p_theme->get_icon(SNAME("Folder"), AppStringName(AppIcons)));
		p_theme->set_icon("parent_folder", "FileDialog", p_theme->get_icon(SNAME("ArrowUp"), AppStringName(AppIcons)));
		p_theme->set_icon("back_folder", "FileDialog", p_theme->get_icon(SNAME("Back"), AppStringName(AppIcons)));
		p_theme->set_icon("forward_folder", "FileDialog", p_theme->get_icon(SNAME("Forward"), AppStringName(AppIcons)));
		p_theme->set_icon("reload", "FileDialog", p_theme->get_icon(SNAME("Reload"), AppStringName(AppIcons)));
		p_theme->set_icon("toggle_hidden", "FileDialog", p_theme->get_icon(SNAME("GuiVisibilityVisible"), AppStringName(AppIcons)));
		p_theme->set_icon("create_folder", "FileDialog", p_theme->get_icon(SNAME("FolderCreate"), AppStringName(AppIcons)));
		// Use a different color for folder icons to make them easier to distinguish from files.
		// On a light theme, the icon will be dark, so we need to lighten it before blending it with the accent color.
		p_theme->set_color("folder_icon_color", "FileDialog", (p_config.dark_theme ? Color(1, 1, 1) : Color(4.25, 4.25, 4.25)).lerp(p_config.accent_color, 0.7));
		p_theme->set_color("file_disabled_color", "FileDialog", p_config.font_disabled_color);

		// PopupDialog.
		p_theme->set_stylebox(SceneStringName(panel), "PopupDialog", p_config.popup_style);

		// PopupMenu.
		{
			Ref<StyleBoxFlat> style_popup_menu = p_config.popup_border_style->duplicate();
			// Use 1 pixel for the sides, since if 0 is used, the highlight of hovered items is drawn
			// on top of the popup border. This causes a 'gap' in the panel border when an item is highlighted,
			// and it looks weird. 1px solves this.
			style_popup_menu->set_content_margin_individual(Math::round(APP_SCALE), 2 * APP_SCALE, Math::round(APP_SCALE), 2 * APP_SCALE);
			p_theme->set_stylebox(SceneStringName(panel), "PopupMenu", style_popup_menu);

			Ref<StyleBoxFlat> style_menu_hover = p_config.button_style_hover->duplicate();
			// Don't use rounded corners for hover highlights since the StyleBox touches the PopupMenu's edges.
			style_menu_hover->set_corner_radius_all(0);
			p_theme->set_stylebox(SceneStringName(hover), "PopupMenu", style_menu_hover);

			Ref<StyleBoxLine> style_popup_separator(memnew(StyleBoxLine));
			style_popup_separator->set_color(p_config.separator_color);
			style_popup_separator->set_grow_begin(Math::round(APP_SCALE) - MAX(Math::round(APP_SCALE), p_config.border_width));
			style_popup_separator->set_grow_end(Math::round(APP_SCALE) - MAX(Math::round(APP_SCALE), p_config.border_width));
			style_popup_separator->set_thickness(MAX(Math::round(APP_SCALE), p_config.border_width));

			Ref<StyleBoxLine> style_popup_labeled_separator_left(memnew(StyleBoxLine));
			style_popup_labeled_separator_left->set_grow_begin(Math::round(APP_SCALE) - MAX(Math::round(APP_SCALE), p_config.border_width));
			style_popup_labeled_separator_left->set_color(p_config.separator_color);
			style_popup_labeled_separator_left->set_thickness(MAX(Math::round(APP_SCALE), p_config.border_width));

			Ref<StyleBoxLine> style_popup_labeled_separator_right(memnew(StyleBoxLine));
			style_popup_labeled_separator_right->set_grow_end(Math::round(APP_SCALE) - MAX(Math::round(APP_SCALE), p_config.border_width));
			style_popup_labeled_separator_right->set_color(p_config.separator_color);
			style_popup_labeled_separator_right->set_thickness(MAX(Math::round(APP_SCALE), p_config.border_width));

			p_theme->set_stylebox("separator", "PopupMenu", style_popup_separator);
			p_theme->set_stylebox("labeled_separator_left", "PopupMenu", style_popup_labeled_separator_left);
			p_theme->set_stylebox("labeled_separator_right", "PopupMenu", style_popup_labeled_separator_right);

			p_theme->set_color(SceneStringName(font_color), "PopupMenu", p_config.font_color);
			p_theme->set_color("font_hover_color", "PopupMenu", p_config.font_hover_color);
			p_theme->set_color("font_accelerator_color", "PopupMenu", p_config.font_disabled_color);
			p_theme->set_color("font_disabled_color", "PopupMenu", p_config.font_disabled_color);
			p_theme->set_color("font_separator_color", "PopupMenu", p_config.font_disabled_color);
			p_theme->set_color("font_outline_color", "PopupMenu", p_config.font_outline_color);

			p_theme->set_icon("checked", "PopupMenu", p_theme->get_icon(SNAME("GuiChecked"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked", "PopupMenu", p_theme->get_icon(SNAME("GuiUnchecked"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_checked", "PopupMenu", p_theme->get_icon(SNAME("GuiRadioChecked"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_unchecked", "PopupMenu", p_theme->get_icon(SNAME("GuiRadioUnchecked"), AppStringName(AppIcons)));
			p_theme->set_icon("checked_disabled", "PopupMenu", p_theme->get_icon(SNAME("GuiCheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("unchecked_disabled", "PopupMenu", p_theme->get_icon(SNAME("GuiUncheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_checked_disabled", "PopupMenu", p_theme->get_icon(SNAME("GuiRadioCheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("radio_unchecked_disabled", "PopupMenu", p_theme->get_icon(SNAME("GuiRadioUncheckedDisabled"), AppStringName(AppIcons)));
			p_theme->set_icon("submenu", "PopupMenu", p_theme->get_icon(SNAME("ArrowRight"), AppStringName(AppIcons)));
			p_theme->set_icon("submenu_mirrored", "PopupMenu", p_theme->get_icon(SNAME("ArrowLeft"), AppStringName(AppIcons)));

			p_theme->set_constant("v_separation", "PopupMenu", p_config.forced_even_separation * APP_SCALE);
			p_theme->set_constant("outline_size", "PopupMenu", 0);
			p_theme->set_constant("item_start_padding", "PopupMenu", p_config.separation_margin);
			p_theme->set_constant("item_end_padding", "PopupMenu", p_config.separation_margin);
		}
	}

	// Sliders and scrollbars.
	{
		Ref<Texture2D> empty_icon = memnew(ImageTexture);

		// HScrollBar.

		if (p_config.enable_touch_optimizations) {
			p_theme->set_stylebox("scroll", "HScrollBar", make_line_stylebox(p_config.separator_color, 50));
		} else {
			p_theme->set_stylebox("scroll", "HScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollBg"), AppStringName(AppIcons)), 5, 5, 5, 5, -5, 1, -5, 1));
		}
		p_theme->set_stylebox("scroll_focus", "HScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollBg"), AppStringName(AppIcons)), 5, 5, 5, 5, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber", "HScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabber"), AppStringName(AppIcons)), 6, 6, 6, 6, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber_highlight", "HScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabberHl"), AppStringName(AppIcons)), 5, 5, 5, 5, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber_pressed", "HScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabberPressed"), AppStringName(AppIcons)), 6, 6, 6, 6, 1, 1, 1, 1));

		p_theme->set_icon("increment", "HScrollBar", empty_icon);
		p_theme->set_icon("increment_highlight", "HScrollBar", empty_icon);
		p_theme->set_icon("increment_pressed", "HScrollBar", empty_icon);
		p_theme->set_icon("decrement", "HScrollBar", empty_icon);
		p_theme->set_icon("decrement_highlight", "HScrollBar", empty_icon);
		p_theme->set_icon("decrement_pressed", "HScrollBar", empty_icon);

		// VScrollBar.

		if (p_config.enable_touch_optimizations) {
			p_theme->set_stylebox("scroll", "VScrollBar", make_line_stylebox(p_config.separator_color, 50, 1, 1, true));
		} else {
			p_theme->set_stylebox("scroll", "VScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollBg"), AppStringName(AppIcons)), 5, 5, 5, 5, 1, -5, 1, -5));
		}
		p_theme->set_stylebox("scroll_focus", "VScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollBg"), AppStringName(AppIcons)), 5, 5, 5, 5, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber", "VScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabber"), AppStringName(AppIcons)), 6, 6, 6, 6, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber_highlight", "VScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabberHl"), AppStringName(AppIcons)), 5, 5, 5, 5, 1, 1, 1, 1));
		p_theme->set_stylebox("grabber_pressed", "VScrollBar", make_stylebox(p_theme->get_icon(SNAME("GuiScrollGrabberPressed"), AppStringName(AppIcons)), 6, 6, 6, 6, 1, 1, 1, 1));

		p_theme->set_icon("increment", "VScrollBar", empty_icon);
		p_theme->set_icon("increment_highlight", "VScrollBar", empty_icon);
		p_theme->set_icon("increment_pressed", "VScrollBar", empty_icon);
		p_theme->set_icon("decrement", "VScrollBar", empty_icon);
		p_theme->set_icon("decrement_highlight", "VScrollBar", empty_icon);
		p_theme->set_icon("decrement_pressed", "VScrollBar", empty_icon);

		// Slider
		const int background_margin = MAX(2, p_config.base_margin / 2);

		// HSlider.
		p_theme->set_icon("grabber_highlight", "HSlider", p_theme->get_icon(SNAME("GuiSliderGrabberHl"), AppStringName(AppIcons)));
		p_theme->set_icon("grabber", "HSlider", p_theme->get_icon(SNAME("GuiSliderGrabber"), AppStringName(AppIcons)));
		p_theme->set_stylebox("slider", "HSlider", make_flat_stylebox(p_config.dark_color_3, 0, background_margin, 0, background_margin, p_config.corner_radius));
		p_theme->set_stylebox("grabber_area", "HSlider", make_flat_stylebox(p_config.contrast_color_1, 0, background_margin, 0, background_margin, p_config.corner_radius));
		p_theme->set_stylebox("grabber_area_highlight", "HSlider", make_flat_stylebox(p_config.contrast_color_1, 0, background_margin, 0, background_margin));
		p_theme->set_constant("center_grabber", "HSlider", 0);
		p_theme->set_constant("grabber_offset", "HSlider", 0);

		// VSlider.
		p_theme->set_icon("grabber", "VSlider", p_theme->get_icon(SNAME("GuiSliderGrabber"), AppStringName(AppIcons)));
		p_theme->set_icon("grabber_highlight", "VSlider", p_theme->get_icon(SNAME("GuiSliderGrabberHl"), AppStringName(AppIcons)));
		p_theme->set_stylebox("slider", "VSlider", make_flat_stylebox(p_config.dark_color_3, background_margin, 0, background_margin, 0, p_config.corner_radius));
		p_theme->set_stylebox("grabber_area", "VSlider", make_flat_stylebox(p_config.contrast_color_1, background_margin, 0, background_margin, 0, p_config.corner_radius));
		p_theme->set_stylebox("grabber_area_highlight", "VSlider", make_flat_stylebox(p_config.contrast_color_1, background_margin, 0, background_margin, 0));
		p_theme->set_constant("center_grabber", "VSlider", 0);
		p_theme->set_constant("grabber_offset", "VSlider", 0);
	}

	// Labels.
	{
		// RichTextLabel.

		p_theme->set_stylebox(CoreStringName(normal), "RichTextLabel", p_config.tree_panel_style);
		p_theme->set_stylebox("focus", "RichTextLabel", make_empty_stylebox());

		p_theme->set_color("default_color", "RichTextLabel", p_config.font_color);
		p_theme->set_color("font_shadow_color", "RichTextLabel", Color(0, 0, 0, 0));
		p_theme->set_color("font_outline_color", "RichTextLabel", p_config.font_outline_color);
		p_theme->set_color("selection_color", "RichTextLabel", p_config.selection_color);

		p_theme->set_constant("shadow_offset_x", "RichTextLabel", 1 * APP_SCALE);
		p_theme->set_constant("shadow_offset_y", "RichTextLabel", 1 * APP_SCALE);
		p_theme->set_constant("shadow_outline_size", "RichTextLabel", 1 * APP_SCALE);
		p_theme->set_constant("outline_size", "RichTextLabel", 0);

		// Label.

		p_theme->set_stylebox(CoreStringName(normal), "Label", p_config.base_empty_style);
		p_theme->set_stylebox("focus", "Label", p_config.button_style_focus);

		p_theme->set_color(SceneStringName(font_color), "Label", p_config.font_color);
		p_theme->set_color("font_shadow_color", "Label", Color(0, 0, 0, 0));
		p_theme->set_color("font_outline_color", "Label", p_config.font_outline_color);

		p_theme->set_constant("shadow_offset_x", "Label", 1 * APP_SCALE);
		p_theme->set_constant("shadow_offset_y", "Label", 1 * APP_SCALE);
		p_theme->set_constant("shadow_outline_size", "Label", 1 * APP_SCALE);
		p_theme->set_constant("line_spacing", "Label", 3 * APP_SCALE);
		p_theme->set_constant("outline_size", "Label", 0);
	}

	// SpinBox.
	{
		Ref<Texture2D> empty_icon = memnew(ImageTexture);
		p_theme->set_icon("updown", "SpinBox", empty_icon);
		p_theme->set_icon("up", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxUp"), AppStringName(AppIcons)));
		p_theme->set_icon("up_hover", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxUp"), AppStringName(AppIcons)));
		p_theme->set_icon("up_pressed", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxUp"), AppStringName(AppIcons)));
		p_theme->set_icon("up_disabled", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxUp"), AppStringName(AppIcons)));
		p_theme->set_icon("down", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxDown"), AppStringName(AppIcons)));
		p_theme->set_icon("down_hover", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxDown"), AppStringName(AppIcons)));
		p_theme->set_icon("down_pressed", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxDown"), AppStringName(AppIcons)));
		p_theme->set_icon("down_disabled", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxDown"), AppStringName(AppIcons)));

		p_theme->set_stylebox("up_background", "SpinBox", make_empty_stylebox());
		p_theme->set_stylebox("up_background_hovered", "SpinBox", p_config.button_style_hover);
		p_theme->set_stylebox("up_background_pressed", "SpinBox", p_config.button_style_pressed);
		p_theme->set_stylebox("up_background_disabled", "SpinBox", make_empty_stylebox());
		p_theme->set_stylebox("down_background", "SpinBox", make_empty_stylebox());
		p_theme->set_stylebox("down_background_hovered", "SpinBox", p_config.button_style_hover);
		p_theme->set_stylebox("down_background_pressed", "SpinBox", p_config.button_style_pressed);
		p_theme->set_stylebox("down_background_disabled", "SpinBox", make_empty_stylebox());

		p_theme->set_color("up_icon_modulate", "SpinBox", p_config.font_color);
		p_theme->set_color("up_hover_icon_modulate", "SpinBox", p_config.font_hover_color);
		p_theme->set_color("up_pressed_icon_modulate", "SpinBox", p_config.font_pressed_color);
		p_theme->set_color("up_disabled_icon_modulate", "SpinBox", p_config.font_disabled_color);
		p_theme->set_color("down_icon_modulate", "SpinBox", p_config.font_color);
		p_theme->set_color("down_hover_icon_modulate", "SpinBox", p_config.font_hover_color);
		p_theme->set_color("down_pressed_icon_modulate", "SpinBox", p_config.font_pressed_color);
		p_theme->set_color("down_disabled_icon_modulate", "SpinBox", p_config.font_disabled_color);

		p_theme->set_stylebox("field_and_buttons_separator", "SpinBox", make_empty_stylebox());
		p_theme->set_stylebox("up_down_buttons_separator", "SpinBox", make_empty_stylebox());

		p_theme->set_constant("buttons_vertical_separation", "SpinBox", 0);
		p_theme->set_constant("field_and_buttons_separation", "SpinBox", 2);
		p_theme->set_constant("buttons_width", "SpinBox", 16);
	}

	// ProgressBar.
	p_theme->set_stylebox("background", "ProgressBar", make_stylebox(p_theme->get_icon(SNAME("GuiProgressBar"), AppStringName(AppIcons)), 4, 4, 4, 4, 0, 0, 0, 0));
	p_theme->set_stylebox("fill", "ProgressBar", make_stylebox(p_theme->get_icon(SNAME("GuiProgressFill"), AppStringName(AppIcons)), 6, 6, 6, 6, 2, 1, 2, 1));
	p_theme->set_color(SceneStringName(font_color), "ProgressBar", p_config.font_color);
	p_theme->set_color("font_outline_color", "ProgressBar", p_config.font_outline_color);
	p_theme->set_constant("outline_size", "ProgressBar", 0);

// TODO
#if 0
	// GraphEdit and related nodes.
	{
		// GraphEdit.

		p_theme->set_stylebox(SceneStringName(panel), "GraphEdit", p_config.tree_panel_style);
		p_theme->set_stylebox("panel_focus", "GraphEdit", p_config.button_style_focus);
		p_theme->set_stylebox("menu_panel", "GraphEdit", make_flat_stylebox(p_config.dark_color_1 * Color(1, 1, 1, 0.6), 4, 2, 4, 2, 3));

		float grid_base_brightness = p_config.dark_theme ? 1.0 : 0.0;
		GraphEdit::GridPattern grid_pattern = (GraphEdit::GridPattern) int(APP_GET("editors/visual_editors/grid_pattern"));
		switch (grid_pattern) {
			case GraphEdit::GRID_PATTERN_LINES:
				p_theme->set_color("grid_major", "GraphEdit", Color(grid_base_brightness, grid_base_brightness, grid_base_brightness, 0.10));
				p_theme->set_color("grid_minor", "GraphEdit", Color(grid_base_brightness, grid_base_brightness, grid_base_brightness, 0.05));
				break;
			case GraphEdit::GRID_PATTERN_DOTS:
				p_theme->set_color("grid_major", "GraphEdit", Color(grid_base_brightness, grid_base_brightness, grid_base_brightness, 0.07));
				p_theme->set_color("grid_minor", "GraphEdit", Color(grid_base_brightness, grid_base_brightness, grid_base_brightness, 0.07));
				break;
			default:
				WARN_PRINT("Unknown grid pattern.");
				break;
		}

		p_theme->set_color("selection_fill", "GraphEdit", p_theme->get_color(SNAME("box_selection_fill_color"), AppStringName(App)));
		p_theme->set_color("selection_stroke", "GraphEdit", p_theme->get_color(SNAME("box_selection_stroke_color"), AppStringName(App)));
		p_theme->set_color("activity", "GraphEdit", p_config.dark_theme ? Color(1, 1, 1) : Color(0, 0, 0));

		p_theme->set_color("connection_hover_tint_color", "GraphEdit", p_config.dark_theme ? Color(0, 0, 0, 0.3) : Color(1, 1, 1, 0.3));
		p_theme->set_constant("connection_hover_thickness", "GraphEdit", 0);
		p_theme->set_color("connection_valid_target_tint_color", "GraphEdit", p_config.dark_theme ? Color(1, 1, 1, 0.4) : Color(0, 0, 0, 0.4));
		p_theme->set_color("connection_rim_color", "GraphEdit", p_config.tree_panel_style->get_bg_color());

		p_theme->set_icon("zoom_out", "GraphEdit", p_theme->get_icon(SNAME("ZoomLess"), AppStringName(AppIcons)));
		p_theme->set_icon("zoom_in", "GraphEdit", p_theme->get_icon(SNAME("ZoomMore"), AppStringName(AppIcons)));
		p_theme->set_icon("zoom_reset", "GraphEdit", p_theme->get_icon(SNAME("ZoomReset"), AppStringName(AppIcons)));
		p_theme->set_icon("grid_toggle", "GraphEdit", p_theme->get_icon(SNAME("GridToggle"), AppStringName(AppIcons)));
		p_theme->set_icon("minimap_toggle", "GraphEdit", p_theme->get_icon(SNAME("GridMinimap"), AppStringName(AppIcons)));
		p_theme->set_icon("snapping_toggle", "GraphEdit", p_theme->get_icon(SNAME("SnapGrid"), AppStringName(AppIcons)));
		p_theme->set_icon("layout", "GraphEdit", p_theme->get_icon(SNAME("GridLayout"), AppStringName(AppIcons)));

		// GraphEditMinimap.
		{
			Ref<StyleBoxFlat> style_minimap_bg = make_flat_stylebox(p_config.dark_color_1, 0, 0, 0, 0);
			style_minimap_bg->set_border_color(p_config.dark_color_3);
			style_minimap_bg->set_border_width_all(1);
			p_theme->set_stylebox(SceneStringName(panel), "GraphEditMinimap", style_minimap_bg);

			Ref<StyleBoxFlat> style_minimap_camera;
			Ref<StyleBoxFlat> style_minimap_node;
			if (p_config.dark_theme) {
				style_minimap_camera = make_flat_stylebox(Color(0.65, 0.65, 0.65, 0.2), 0, 0, 0, 0);
				style_minimap_camera->set_border_color(Color(0.65, 0.65, 0.65, 0.45));
				style_minimap_node = make_flat_stylebox(Color(1, 1, 1), 0, 0, 0, 0);
			} else {
				style_minimap_camera = make_flat_stylebox(Color(0.38, 0.38, 0.38, 0.2), 0, 0, 0, 0);
				style_minimap_camera->set_border_color(Color(0.38, 0.38, 0.38, 0.45));
				style_minimap_node = make_flat_stylebox(Color(0, 0, 0), 0, 0, 0, 0);
			}
			style_minimap_camera->set_border_width_all(1);
			style_minimap_node->set_anti_aliased(false);
			p_theme->set_stylebox("camera", "GraphEditMinimap", style_minimap_camera);
			p_theme->set_stylebox("node", "GraphEditMinimap", style_minimap_node);

			const Color minimap_resizer_color = p_config.dark_theme ? Color(1, 1, 1, 0.65) : Color(0, 0, 0, 0.65);
			p_theme->set_icon("resizer", "GraphEditMinimap", p_theme->get_icon(SNAME("GuiResizerTopLeft"), AppStringName(AppIcons)));
			p_theme->set_color("resizer_color", "GraphEditMinimap", minimap_resizer_color);
		}

		// GraphElement, GraphNode & GraphFrame.
		{
			const int gn_margin_top = 2;
			const int gn_margin_side = 2;
			const int gn_margin_bottom = 2;

			const int gn_corner_radius = 3;

			const Color gn_bg_color = p_config.dark_theme ? p_config.dark_color_3 : p_config.dark_color_1.lerp(p_config.mono_color, 0.09);
			const Color gn_selected_border_color = p_config.dark_theme ? Color(1, 1, 1) : Color(0, 0, 0);
			const Color gn_frame_bg = gn_bg_color.lerp(p_config.tree_panel_style->get_bg_color(), 0.3);

			const bool high_contrast_borders = p_config.draw_extra_borders && p_config.dark_theme;

			Ref<StyleBoxFlat> gn_panel_style = make_flat_stylebox(gn_frame_bg, gn_margin_side, gn_margin_top, gn_margin_side, gn_margin_bottom, p_config.corner_radius);
			gn_panel_style->set_border_width(SIDE_BOTTOM, 2 * APP_SCALE);
			gn_panel_style->set_border_width(SIDE_LEFT, 2 * APP_SCALE);
			gn_panel_style->set_border_width(SIDE_RIGHT, 2 * APP_SCALE);
			gn_panel_style->set_border_color(high_contrast_borders ? gn_bg_color.lightened(0.2) : gn_bg_color.darkened(0.3));
			gn_panel_style->set_corner_radius_individual(0, 0, gn_corner_radius * APP_SCALE, gn_corner_radius * APP_SCALE);
			gn_panel_style->set_anti_aliased(true);

			Ref<StyleBoxFlat> gn_panel_selected_style = gn_panel_style->duplicate();
			gn_panel_selected_style->set_bg_color(p_config.dark_theme ? gn_bg_color.lightened(0.15) : gn_bg_color.darkened(0.15));
			gn_panel_selected_style->set_border_width(SIDE_TOP, 0);
			gn_panel_selected_style->set_border_width(SIDE_BOTTOM, 2 * APP_SCALE);
			gn_panel_selected_style->set_border_width(SIDE_LEFT, 2 * APP_SCALE);
			gn_panel_selected_style->set_border_width(SIDE_RIGHT, 2 * APP_SCALE);
			gn_panel_selected_style->set_border_color(gn_selected_border_color);

			const int gn_titlebar_margin_top = 8;
			const int gn_titlebar_margin_side = 12;
			const int gn_titlebar_margin_bottom = 8;

			Ref<StyleBoxFlat> gn_titlebar_style = make_flat_stylebox(gn_bg_color, gn_titlebar_margin_side, gn_titlebar_margin_top, gn_titlebar_margin_side, gn_titlebar_margin_bottom, p_config.corner_radius);
			gn_titlebar_style->set_border_width(SIDE_TOP, 2 * APP_SCALE);
			gn_titlebar_style->set_border_width(SIDE_LEFT, 2 * APP_SCALE);
			gn_titlebar_style->set_border_width(SIDE_RIGHT, 2 * APP_SCALE);
			gn_titlebar_style->set_border_color(high_contrast_borders ? gn_bg_color.lightened(0.2) : gn_bg_color.darkened(0.3));
			gn_titlebar_style->set_expand_margin(SIDE_TOP, 2 * APP_SCALE);
			gn_titlebar_style->set_corner_radius_individual(gn_corner_radius * APP_SCALE, gn_corner_radius * APP_SCALE, 0, 0);
			gn_titlebar_style->set_anti_aliased(true);

			Ref<StyleBoxFlat> gn_titlebar_selected_style = gn_titlebar_style->duplicate();
			gn_titlebar_selected_style->set_border_color(gn_selected_border_color);
			gn_titlebar_selected_style->set_border_width(SIDE_TOP, 2 * APP_SCALE);
			gn_titlebar_selected_style->set_border_width(SIDE_LEFT, 2 * APP_SCALE);
			gn_titlebar_selected_style->set_border_width(SIDE_RIGHT, 2 * APP_SCALE);
			gn_titlebar_selected_style->set_expand_margin(SIDE_TOP, 2 * APP_SCALE);

			Color gn_decoration_color = p_config.dark_color_1.inverted();

			// GraphElement.

			p_theme->set_stylebox(SceneStringName(panel), "GraphElement", gn_panel_style);
			p_theme->set_stylebox("panel_selected", "GraphElement", gn_panel_selected_style);
			p_theme->set_stylebox("titlebar", "GraphElement", gn_titlebar_style);
			p_theme->set_stylebox("titlebar_selected", "GraphElement", gn_titlebar_selected_style);

			p_theme->set_color("resizer_color", "GraphElement", gn_decoration_color);
			p_theme->set_icon("resizer", "GraphElement", p_theme->get_icon(SNAME("GuiResizer"), AppStringName(AppIcons)));

			// GraphNode.

			Ref<StyleBoxEmpty> gn_slot_style = make_empty_stylebox(12, 0, 12, 0);

			p_theme->set_stylebox(SceneStringName(panel), "GraphNode", gn_panel_style);
			p_theme->set_stylebox("panel_selected", "GraphNode", gn_panel_selected_style);
			p_theme->set_stylebox("panel_focus", "GraphNode", p_config.button_style_focus);
			p_theme->set_stylebox("titlebar", "GraphNode", gn_titlebar_style);
			p_theme->set_stylebox("titlebar_selected", "GraphNode", gn_titlebar_selected_style);
			p_theme->set_stylebox("slot", "GraphNode", gn_slot_style);
			p_theme->set_stylebox("slot_selected", "GraphNode", p_config.button_style_focus);

			p_theme->set_color("resizer_color", "GraphNode", gn_decoration_color);

			p_theme->set_constant("port_h_offset", "GraphNode", 1);
			p_theme->set_constant("separation", "GraphNode", 1 * APP_SCALE);

			Ref<DPITexture> port_icon = p_theme->get_icon(SNAME("GuiGraphNodePort"), AppStringName(AppIcons));
			// The true size is 24x24 This is necessary for sharp port icons at high zoom levels in GraphEdit (up to ~200%).
			port_icon->set_size_override(Size2(12, 12));
			p_theme->set_icon("port", "GraphNode", port_icon);

			// GraphNode's title Label.
			p_theme->set_type_variation("GraphNodeTitleLabel", "Label");
			p_theme->set_stylebox(CoreStringName(normal), "GraphNodeTitleLabel", make_empty_stylebox(0, 0, 0, 0));
			p_theme->set_color("font_shadow_color", "GraphNodeTitleLabel", p_config.shadow_color);
			p_theme->set_constant("shadow_outline_size", "GraphNodeTitleLabel", 4);
			p_theme->set_constant("shadow_offset_x", "GraphNodeTitleLabel", 0);
			p_theme->set_constant("shadow_offset_y", "GraphNodeTitleLabel", 1);
			p_theme->set_constant("line_spacing", "GraphNodeTitleLabel", 3 * APP_SCALE);

			// GraphFrame.

			const int gf_corner_width = 7 * APP_SCALE;
			const int gf_border_width = 2 * MAX(1, APP_SCALE);

			Ref<StyleBoxFlat> graphframe_sb = make_flat_stylebox(Color(0.0, 0.0, 0.0, 0.2), gn_margin_side, gn_margin_side, gn_margin_side, gn_margin_bottom, gf_corner_width);
			graphframe_sb->set_expand_margin(SIDE_TOP, 38 * APP_SCALE);
			graphframe_sb->set_border_width_all(gf_border_width);
			graphframe_sb->set_border_color(high_contrast_borders ? gn_bg_color.lightened(0.2) : gn_bg_color.darkened(0.3));
			graphframe_sb->set_shadow_size(8 * APP_SCALE);
			graphframe_sb->set_shadow_color(Color(p_config.shadow_color, p_config.shadow_color.a * 0.25));
			graphframe_sb->set_anti_aliased(true);

			Ref<StyleBoxFlat> graphframe_sb_selected = graphframe_sb->duplicate();
			graphframe_sb_selected->set_border_color(gn_selected_border_color);

			p_theme->set_stylebox(SceneStringName(panel), "GraphFrame", graphframe_sb);
			p_theme->set_stylebox("panel_selected", "GraphFrame", graphframe_sb_selected);
			p_theme->set_stylebox("titlebar", "GraphFrame", make_empty_stylebox(4, 4, 4, 4));
			p_theme->set_stylebox("titlebar_selected", "GraphFrame", make_empty_stylebox(4, 4, 4, 4));
			p_theme->set_color("resizer_color", "GraphFrame", gn_decoration_color);

			// GraphFrame's title Label.
			p_theme->set_type_variation("GraphFrameTitleLabel", "Label");
			p_theme->set_stylebox(CoreStringName(normal), "GraphFrameTitleLabel", memnew(StyleBoxEmpty));
			p_theme->set_font_size(SceneStringName(font_size), "GraphFrameTitleLabel", 22 * APP_SCALE);
			p_theme->set_color(SceneStringName(font_color), "GraphFrameTitleLabel", Color(1, 1, 1));
			p_theme->set_color("font_shadow_color", "GraphFrameTitleLabel", Color(0, 0, 0, 0));
			p_theme->set_color("font_outline_color", "GraphFrameTitleLabel", Color(1, 1, 1));
			p_theme->set_constant("shadow_offset_x", "GraphFrameTitleLabel", 1 * APP_SCALE);
			p_theme->set_constant("shadow_offset_y", "GraphFrameTitleLabel", 1 * APP_SCALE);
			p_theme->set_constant("outline_size", "GraphFrameTitleLabel", 0);
			p_theme->set_constant("shadow_outline_size", "GraphFrameTitleLabel", 1 * APP_SCALE);
			p_theme->set_constant("line_spacing", "GraphFrameTitleLabel", 3 * APP_SCALE);
		}

		// VisualShader reroute node.
		{
			Ref<StyleBox> vs_reroute_panel_style = make_empty_stylebox();
			Ref<StyleBox> vs_reroute_titlebar_style = vs_reroute_panel_style->duplicate();
			vs_reroute_titlebar_style->set_content_margin_all(16 * APP_SCALE);
			p_theme->set_stylebox(SceneStringName(panel), "VSRerouteNode", vs_reroute_panel_style);
			p_theme->set_stylebox("panel_selected", "VSRerouteNode", vs_reroute_panel_style);
			p_theme->set_stylebox("titlebar", "VSRerouteNode", vs_reroute_titlebar_style);
			p_theme->set_stylebox("titlebar_selected", "VSRerouteNode", vs_reroute_titlebar_style);
			p_theme->set_stylebox("slot", "VSRerouteNode", make_empty_stylebox());

			p_theme->set_color("drag_background", "VSRerouteNode", p_config.dark_theme ? Color(0.19, 0.21, 0.24) : Color(0.8, 0.8, 0.8));
			p_theme->set_color("selected_rim_color", "VSRerouteNode", p_config.dark_theme ? Color(1, 1, 1) : Color(0, 0, 0));
		}
	}

	// ColorPicker and related nodes.
	{
		// ColorPicker.
		p_config.circle_style_focus = p_config.button_style_focus->duplicate();
		p_config.circle_style_focus->set_corner_radius_all(256 * APP_SCALE);
		p_config.circle_style_focus->set_corner_detail(32 * APP_SCALE);

		p_theme->set_constant("margin", "ColorPicker", p_config.base_margin);
		p_theme->set_constant("sv_width", "ColorPicker", 256 * APP_SCALE);
		p_theme->set_constant("sv_height", "ColorPicker", 256 * APP_SCALE);
		p_theme->set_constant("h_width", "ColorPicker", 30 * APP_SCALE);
		p_theme->set_constant("label_width", "ColorPicker", 10 * APP_SCALE);
		p_theme->set_constant("center_slider_grabbers", "ColorPicker", 1);

		p_theme->set_stylebox("sample_focus", "ColorPicker", p_config.button_style_focus);
		p_theme->set_stylebox("picker_focus_rectangle", "ColorPicker", p_config.button_style_focus);
		p_theme->set_stylebox("picker_focus_circle", "ColorPicker", p_config.circle_style_focus);
		p_theme->set_color("focused_not_editing_cursor_color", "ColorPicker", p_config.highlight_color);

		p_theme->set_icon("screen_picker", "ColorPicker", p_theme->get_icon(SNAME("ColorPick"), AppStringName(AppIcons)));
		p_theme->set_icon("shape_circle", "ColorPicker", p_theme->get_icon(SNAME("PickerShapeCircle"), AppStringName(AppIcons)));
		p_theme->set_icon("shape_rect", "ColorPicker", p_theme->get_icon(SNAME("PickerShapeRectangle"), AppStringName(AppIcons)));
		p_theme->set_icon("shape_rect_wheel", "ColorPicker", p_theme->get_icon(SNAME("PickerShapeRectangleWheel"), AppStringName(AppIcons)));
		p_theme->set_icon("add_preset", "ColorPicker", p_theme->get_icon(SNAME("Add"), AppStringName(AppIcons)));
		p_theme->set_icon("sample_bg", "ColorPicker", p_theme->get_icon(SNAME("GuiMiniCheckerboard"), AppStringName(AppIcons)));
		p_theme->set_icon("sample_revert", "ColorPicker", p_theme->get_icon(SNAME("Reload"), AppStringName(AppIcons)));
		p_theme->set_icon("overbright_indicator", "ColorPicker", p_theme->get_icon(SNAME("OverbrightIndicator"), AppStringName(AppIcons)));
		p_theme->set_icon("bar_arrow", "ColorPicker", p_theme->get_icon(SNAME("ColorPickerBarArrow"), AppStringName(AppIcons)));
		p_theme->set_icon("picker_cursor", "ColorPicker", p_theme->get_icon(SNAME("PickerCursor"), AppStringName(AppIcons)));
		p_theme->set_icon("picker_cursor_bg", "ColorPicker", p_theme->get_icon(SNAME("PickerCursorBg"), AppStringName(AppIcons)));
		p_theme->set_icon("color_script", "ColorPicker", p_theme->get_icon(SNAME("Script"), AppStringName(AppIcons)));

		// ColorPickerButton.
		p_theme->set_icon("bg", "ColorPickerButton", p_theme->get_icon(SNAME("GuiMiniCheckerboard"), AppStringName(AppIcons)));

		// ColorPresetButton.
		p_theme->set_stylebox("preset_fg", "ColorPresetButton", make_flat_stylebox(Color(1, 1, 1), 2, 2, 2, 2, 2));
		p_theme->set_icon("preset_bg", "ColorPresetButton", p_theme->get_icon(SNAME("GuiMiniCheckerboard"), AppStringName(AppIcons)));
		p_theme->set_icon("overbright_indicator", "ColorPresetButton", p_theme->get_icon(SNAME("OverbrightIndicator"), AppStringName(AppIcons)));
	}
#endif
}

void AppThemeManager::_populate_app_styles(const Ref<AppTheme> &p_theme, ThemeConfiguration &p_config) {
	// App and main screen.
	{
		// App background.
		Color background_color_opaque = p_config.dark_color_2;
		background_color_opaque.a = 1.0;
		p_theme->set_color("background", AppStringName(App), background_color_opaque);
		p_theme->set_stylebox("Background", AppStringName(AppStyles), make_flat_stylebox(background_color_opaque, p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin));

		Ref<StyleBoxFlat> app_panel_foreground = p_config.base_style->duplicate();
		app_panel_foreground->set_corner_radius_all(0);
		p_theme->set_stylebox("PanelForeground", AppStringName(AppStyles), app_panel_foreground);

		// App focus.
		p_theme->set_stylebox("Focus", AppStringName(AppStyles), p_config.button_style_focus);
	}

	// Standard GUI variations.
	{
		// Custom theme type for MarginContainer with 4px margins.
		p_theme->set_type_variation("MarginContainer4px", "MarginContainer");
		p_theme->set_constant("margin_left", "MarginContainer4px", 4 * APP_SCALE);
		p_theme->set_constant("margin_top", "MarginContainer4px", 4 * APP_SCALE);
		p_theme->set_constant("margin_right", "MarginContainer4px", 4 * APP_SCALE);
		p_theme->set_constant("margin_bottom", "MarginContainer4px", 4 * APP_SCALE);

		// Header LinkButton variation.
		p_theme->set_type_variation("HeaderSmallLink", "LinkButton");
		p_theme->set_font(SceneStringName(font), "HeaderSmallLink", p_theme->get_font(SceneStringName(font), SNAME("HeaderSmall")));
		p_theme->set_font_size(SceneStringName(font_size), "HeaderSmallLink", p_theme->get_font_size(SceneStringName(font_size), SNAME("HeaderSmall")));

		// Flat button variations.
		{
			Ref<StyleBoxEmpty> style_flat_button = make_empty_stylebox();
			Ref<StyleBoxFlat> style_flat_button_hover = p_config.button_style_hover->duplicate();
			Ref<StyleBoxFlat> style_flat_button_pressed = p_config.button_style_pressed->duplicate();

			for (int i = 0; i < 4; i++) {
				style_flat_button->set_content_margin((Side)i, p_config.button_style->get_content_margin((Side)i));
				style_flat_button_hover->set_content_margin((Side)i, p_config.button_style->get_content_margin((Side)i));
				style_flat_button_pressed->set_content_margin((Side)i, p_config.button_style->get_content_margin((Side)i));
			}
			Color flat_pressed_color = p_config.dark_color_1.lightened(0.24).lerp(p_config.accent_color, 0.2) * Color(0.8, 0.8, 0.8, 0.85);
			if (p_config.dark_theme) {
				flat_pressed_color = p_config.dark_color_1.lerp(p_config.accent_color, 0.12) * Color(0.6, 0.6, 0.6, 0.85);
			}
			style_flat_button_pressed->set_bg_color(flat_pressed_color);

			p_theme->set_stylebox(CoreStringName(normal), SceneStringName(FlatButton), style_flat_button);
			p_theme->set_stylebox(SceneStringName(hover), SceneStringName(FlatButton), style_flat_button_hover);
			p_theme->set_stylebox(SceneStringName(pressed), SceneStringName(FlatButton), style_flat_button_pressed);
			p_theme->set_stylebox("disabled", SceneStringName(FlatButton), style_flat_button);

			p_theme->set_stylebox(CoreStringName(normal), "FlatMenuButton", style_flat_button);
			p_theme->set_stylebox(SceneStringName(hover), "FlatMenuButton", style_flat_button_hover);
			p_theme->set_stylebox(SceneStringName(pressed), "FlatMenuButton", style_flat_button_pressed);
			p_theme->set_stylebox("disabled", "FlatMenuButton", style_flat_button);
		}

		// TabContainerOdd variation.
		{
			// Can be used on tabs against the base color background (e.g. nested tabs).
			p_theme->set_type_variation("TabContainerOdd", "TabContainer");

			Ref<StyleBoxFlat> style_tab_selected_odd = p_theme->get_stylebox(SNAME("tab_selected"), SNAME("TabContainer"))->duplicate();
			style_tab_selected_odd->set_bg_color(p_config.disabled_bg_color);
			p_theme->set_stylebox("tab_selected", "TabContainerOdd", style_tab_selected_odd);

			Ref<StyleBoxFlat> style_content_panel_odd = p_config.content_panel_style->duplicate();
			style_content_panel_odd->set_bg_color(p_config.disabled_bg_color);
			p_theme->set_stylebox(SceneStringName(panel), "TabContainerOdd", style_content_panel_odd);
		}
	}
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

// bool AppThemeManager::is_generated_theme_outdated() {
// 	// This list includes settings used by files in the editor/themes folder.
// 	// Note that the editor scale is purposefully omitted because it cannot be changed
// 	// without a restart, so there is no point regenerating the theme.

// 	if (outdated_cache_dirty) {
// 		// TODO: We can use this information more intelligently to do partial theme updates and speed things up.
// 		outdated_cache = AppSettings::get_singleton()->check_changed_settings_in_group("interface/theme") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("interface/editor/font") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("interface/editor/main_font") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("interface/editor/code_font") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("editors/visual_editors") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("text_editor/theme") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("text_editor/help/help") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("docks/property_editor/subresource_hue_tint") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("filesystem/file_dialog/thumbnail_size") ||
// 				AppSettings::get_singleton()->check_changed_settings_in_group("run/output/font_size");

// 		// The outdated flag is relevant at the moment of changing editor settings.
// 		callable_mp_static(&AppThemeManager::_reset_dirty_flag).call_deferred();
// 		outdated_cache_dirty = false;
// 	}

// 	return outdated_cache;
// }

bool AppThemeManager::is_dark_theme() {
	// Light color mode for icons and fonts means it's a dark theme, and vice versa.
	int icon_font_color_setting = APP_GET("interface/theme/icon_and_font_color");

	if (icon_font_color_setting == ColorMode::AUTO_COLOR) {
		Color base_color = APP_GET("interface/theme/base_color");
		return base_color.get_luminance() < 0.5;
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
