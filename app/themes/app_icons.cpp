#include "app_icons.h"

#include "app/app_string_names.h"
#include "app/themes/app_icons.gen.h"
#include "app/themes/app_scale.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"

#include "modules/svg/image_loader_svg.h"

HashMap<Color, Color> AppIcons::AppColorMap::color_conversion_map;
HashSet<StringName> AppIcons::AppColorMap::color_conversion_exceptions;

void AppIcons::AppColorMap::add_conversion_color_pair(const String &p_from_color, const String &p_to_color) {
	color_conversion_map[Color::html(p_from_color)] = Color::html(p_to_color);
}

void AppIcons::AppColorMap::add_conversion_exception(const StringName &p_icon_name) {
	color_conversion_exceptions.insert(p_icon_name);
}

void AppIcons::AppColorMap::create() {
	// Some of the colors below are listed for completeness sake.
	// This can be a basis for proper palette validation later.

	// Convert:               FROM       TO
	add_conversion_color_pair("#478cbf", "#478cbf"); // Godot Blue
	add_conversion_color_pair("#414042", "#414042"); // Godot Gray

	add_conversion_color_pair("#ffffff", "#414141"); // Pure white
	add_conversion_color_pair("#fefefe", "#fefefe"); // Forced light color
	add_conversion_color_pair("#000000", "#bfbfbf"); // Pure black
	add_conversion_color_pair("#010101", "#010101"); // Forced dark color

	// Keep pure RGB colors as is, but list them for explicitness.
	add_conversion_color_pair("#ff0000", "#ff0000"); // Pure red
	add_conversion_color_pair("#00ff00", "#00ff00"); // Pure green
	add_conversion_color_pair("#0000ff", "#0000ff"); // Pure blue

	// GUI Colors
	add_conversion_color_pair("#e0e0e0", "#5a5a5a"); // Common icon color
	add_conversion_color_pair("#808080", "#808080"); // GUI disabled color
	add_conversion_color_pair("#b3b3b3", "#363636"); // GUI disabled light color
	add_conversion_color_pair("#699ce8", "#699ce8"); // GUI highlight color
	add_conversion_color_pair("#f9f9f9", "#606060"); // Scrollbar grabber highlight color

	add_conversion_color_pair("#5fb2ff", "#0079f0"); // Selection (blue)
	add_conversion_color_pair("#003e7a", "#2b74bb"); // Selection (darker blue)
	add_conversion_color_pair("#f7f5cf", "#615f3a"); // Gizmo (yellow)

	add_conversion_color_pair("#c38ef1", "#a85de9"); // Animation
	add_conversion_color_pair("#8da5f3", "#3d64dd"); // 2D Node
	add_conversion_color_pair("#7582a8", "#6d83c8"); // 2D Node Abstract
	add_conversion_color_pair("#fc7f7f", "#cd3838"); // 3D Node
	add_conversion_color_pair("#b56d6d", "#be6a6a"); // 3D Node Abstract
	add_conversion_color_pair("#99c4ff", "#4589e6"); // 2D Non-Node
	add_conversion_color_pair("#869ebf", "#7097cd"); // 2D Non-Node Abstract
	add_conversion_color_pair("#ffa6bd", "#e65c7f"); // 3D Non-Node
	add_conversion_color_pair("#bf909c", "#cd8b9c"); // 3D Non-Node Abstract
	add_conversion_color_pair("#8eef97", "#2fa139"); // GUI Control
	add_conversion_color_pair("#76ad7b", "#64a66a"); // GUI Control Abstract

	add_conversion_color_pair("#f0caa0", "#844b0e"); // Editor-only

	// Other objects
	add_conversion_color_pair("#69c4d4", "#29a3cc"); // Input event highlight (light blue)

	// Control layouts
	add_conversion_color_pair("#d6d6d6", "#474747"); // Highlighted part
	add_conversion_color_pair("#474747", "#d6d6d6"); // Background part
	add_conversion_color_pair("#919191", "#6e6e6e"); // Border part

	// GUI
	add_conversion_exception("GuiChecked");
	add_conversion_exception("GuiRadioChecked");
	add_conversion_exception("GuiIndeterminate");
	add_conversion_exception("GuiCloseCustomizable");
	add_conversion_exception("GuiGraphNodePort");
	add_conversion_exception("GuiResizer");
	add_conversion_exception("GuiMiniCheckerboard");
}

void AppIcons::AppColorMap::finish() {
	color_conversion_map.clear();
	color_conversion_exceptions.clear();
}

void AppIcons::app_configure_icons(bool p_dark_icon_and_font) {
	if (p_dark_icon_and_font) {
		ImageLoaderSVG::set_forced_color_map(HashMap<Color, Color>());
	} else {
		ImageLoaderSVG::set_forced_color_map(AppIcons::AppColorMap::get_color_conversion_map());
	}
}

// See also `generate_icon()` in `scene/theme/default_theme.cpp`.
Ref<DPITexture> app_generate_icon(int p_index, float p_scale, float p_saturation, const Dictionary &p_convert_colors = Dictionary()) {
	return DPITexture::create_from_string(app_icons_sources[p_index], p_scale, p_saturation, p_convert_colors);
}

float get_gizmo_handle_scale(const String &p_gizmo_handle_name, float p_gizmo_handle_scale) {
	if (p_gizmo_handle_scale > 1.0f) {
		// The names of the icons that require additional scaling.
		static HashSet<StringName> gizmo_to_scale;
		if (gizmo_to_scale.is_empty()) {
			// gizmo_to_scale.insert("EditorHandle");
			// gizmo_to_scale.insert("EditorHandleAdd");
			// gizmo_to_scale.insert("EditorHandleDisabled");
			// gizmo_to_scale.insert("EditorCurveHandle");
			// gizmo_to_scale.insert("EditorPathSharpHandle");
			// gizmo_to_scale.insert("EditorPathSmoothHandle");
			// gizmo_to_scale.insert("EditorControlAnchor");
		}

		if (gizmo_to_scale.has(p_gizmo_handle_name)) {
			return APP_SCALE * p_gizmo_handle_scale;
		}
	}

	return APP_SCALE;
}

void AppIcons::app_register_icons(const Ref<Theme> &p_theme, bool p_dark_icon_and_font, float p_icon_saturation, int p_thumb_size, float p_gizmo_handle_scale) {
	// Before we register the icons, we adjust their colors and saturation.
	// Most icons follow the standard rules for color conversion to follow the app
	// theme's polarity (dark/light). We also adjust the saturation for most icons,
	// following the app setting.
	// Some icons are excluded from this conversion, and instead use the configured
	// accent color to replace their innate accent color to match the app theme.
	// And then some icons are completely excluded from the conversion.

	// Standard color conversion map.
	Dictionary color_conversion_map_light;
	Dictionary color_conversion_map_dark;
	// Icons by default are set up for the dark theme, so if the theme is light,
	// we apply the dark-to-light color conversion map.
	for (KeyValue<Color, Color> &E : AppColorMap::get_color_conversion_map()) {
		color_conversion_map_light[E.key] = E.value;
	}
	// These colors should be converted even if we are using a dark theme.
	const Color error_color = p_theme->get_color(SNAME("error_color"), AppStringName(App));
	const Color success_color = p_theme->get_color(SNAME("success_color"), AppStringName(App));
	const Color warning_color = p_theme->get_color(SNAME("warning_color"), AppStringName(App));
	color_conversion_map_dark[Color::html("#ff5f5f")] = error_color;
	color_conversion_map_dark[Color::html("#5fff97")] = success_color;
	color_conversion_map_dark[Color::html("#ffdd65")] = warning_color;
	color_conversion_map_light[Color::html("#ff5f5f")] = error_color;
	color_conversion_map_light[Color::html("#5fff97")] = success_color;
	color_conversion_map_light[Color::html("#ffdd65")] = warning_color;

	Dictionary color_conversion_map = p_dark_icon_and_font ? color_conversion_map_dark : color_conversion_map_light;

	// The names of the icons to exclude from the standard color conversion.
	HashSet<StringName> conversion_exceptions = AppColorMap::get_color_conversion_exceptions();

	// The names of the icons to exclude when adjusting for saturation.
	HashSet<StringName> saturation_exceptions;
	saturation_exceptions.insert("DefaultProjectIcon");
	saturation_exceptions.insert("Godot");
	saturation_exceptions.insert("Logo");

	// Accent color conversion map.
	// It is used on some icons (checkbox, radio, toggle, etc.), regardless of the dark
	// or light mode.
	Dictionary accent_color_map;
	HashSet<StringName> accent_color_icons;

	const Color accent_color = p_theme->get_color(SNAME("accent_color"), AppStringName(App));
	accent_color_map[Color::html("699ce8")] = accent_color;
	if (accent_color.get_luminance() > 0.75) {
		accent_color_map[Color::html("ffffff")] = Color(0.2, 0.2, 0.2);
	}

	accent_color_icons.insert("GuiChecked");
	accent_color_icons.insert("GuiRadioChecked");
	accent_color_icons.insert("GuiIndeterminate");
	accent_color_icons.insert("GuiToggleOn");
	accent_color_icons.insert("GuiToggleOnMirrored");
	accent_color_icons.insert("PlayOverlay");

	// Generate icons.
	{
		for (int i = 0; i < app_icons_count; i++) {
			const String &app_icon_name = app_icons_names[i];
			Ref<DPITexture> icon;

			if (accent_color_icons.has(app_icon_name)) {
				icon = app_generate_icon(i, get_gizmo_handle_scale(app_icon_name, p_gizmo_handle_scale), 1.0, accent_color_map);
			} else {
				float saturation = p_icon_saturation;
				if (saturation_exceptions.has(app_icon_name)) {
					saturation = 1.0;
				}

				if (conversion_exceptions.has(app_icon_name)) {
					icon = app_generate_icon(i, get_gizmo_handle_scale(app_icon_name, p_gizmo_handle_scale), saturation);
				} else {
					icon = app_generate_icon(i, get_gizmo_handle_scale(app_icon_name, p_gizmo_handle_scale), saturation, color_conversion_map);
				}
			}

			p_theme->set_icon(app_icon_name, AppStringName(AppIcons), icon);
		}
	}

	// Generate thumbnail icons with the given thumbnail size.
	// See app\icons\app_icons_builders.py for the code that determines which icons are thumbnails.
	if (p_thumb_size >= 64) {
		const float scale = (float)p_thumb_size / 64.0 * APP_SCALE;
		for (int i = 0; i < app_bg_thumbs_count; i++) {
			const int index = app_bg_thumbs_indices[i];
			Ref<DPITexture> icon;

			if (accent_color_icons.has(app_icons_names[index])) {
				icon = app_generate_icon(index, scale, 1.0, accent_color_map);
			} else {
				float saturation = p_icon_saturation;
				if (saturation_exceptions.has(app_icons_names[index])) {
					saturation = 1.0;
				}

				if (conversion_exceptions.has(app_icons_names[index])) {
					icon = app_generate_icon(index, scale, saturation);
				} else {
					icon = app_generate_icon(index, scale, saturation, color_conversion_map);
				}
			}

			p_theme->set_icon(app_icons_names[index], AppStringName(AppIcons), icon);
		}
	} else {
		const float scale = (float)p_thumb_size / 32.0 * APP_SCALE;
		for (int i = 0; i < app_md_thumbs_count; i++) {
			const int index = app_md_thumbs_indices[i];
			Ref<DPITexture> icon;

			if (accent_color_icons.has(app_icons_names[index])) {
				icon = app_generate_icon(index, scale, 1.0, accent_color_map);
			} else {
				float saturation = p_icon_saturation;
				if (saturation_exceptions.has(app_icons_names[index])) {
					saturation = 1.0;
				}

				if (conversion_exceptions.has(app_icons_names[index])) {
					icon = app_generate_icon(index, scale, saturation);
				} else {
					icon = app_generate_icon(index, scale, saturation, color_conversion_map);
				}
			}

			p_theme->set_icon(app_icons_names[index], AppStringName(AppIcons), icon);
		}
	}
}

void AppIcons::app_copy_icons(const Ref<Theme> &p_theme, const Ref<Theme> &p_old_theme) {
	for (int i = 0; i < app_icons_count; i++) {
		p_theme->set_icon(app_icons_names[i], AppStringName(AppIcons), p_old_theme->get_icon(app_icons_names[i], AppStringName(AppIcons)));
	}
}
