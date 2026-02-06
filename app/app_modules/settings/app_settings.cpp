#include "app_settings.h"

#include "app/app_core/app_paths.h"
#include "app/app_core/io/file_system_access.h"

#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/string/translation_server.h"
#include "core/templates/rb_set.h"
#include "core/version.h"

#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

#include "main/main.h"

#include "app/translations/app_translation.h"

Ref<AppSettings> AppSettings::singleton = nullptr;

// Properties

bool AppSettings::_set(const StringName &p_name, const Variant &p_value) {
	_THREAD_SAFE_METHOD_

	bool changed = _set_only(p_name, p_value);
	if (changed && initialized) {
		changed_settings.insert(p_name);
		emit_signal(SNAME("settings_changed"));

		if (p_name == SNAME("interface/app/language")) {
			setup_language(false);
		}
	}
	return true;
}

bool AppSettings::_set_only(const StringName &p_name, const Variant &p_value) {
	_THREAD_SAFE_METHOD_

	if (p_name == "shortcuts") {
		Array arr = p_value;
		for (int i = 0; i < arr.size(); i++) {
			Dictionary dict = arr[i];
			String shortcut_name = dict["name"];

			Array shortcut_events = dict["shortcuts"];

			Ref<Shortcut> sc;
			sc.instantiate();
			sc->set_events(shortcut_events);
			_add_shortcut_default(shortcut_name, sc);
		}

		return false;
	} else if (p_name == "builtin_action_overrides") {
		Array actions_arr = p_value;
		for (int i = 0; i < actions_arr.size(); i++) {
			Dictionary action_dict = actions_arr[i];

			String action_name = action_dict["name"];
			Array events = action_dict["events"];

			InputMap *im = InputMap::get_singleton();
			im->action_erase_events(action_name);

			builtin_action_overrides[action_name].clear();
			for (int ev_idx = 0; ev_idx < events.size(); ev_idx++) {
				im->action_add_event(action_name, events[ev_idx]);
				builtin_action_overrides[action_name].push_back(events[ev_idx]);
			}
		}
		return false;
	}

	bool changed = false;

	if (p_value.get_type() == Variant::NIL) {
		if (props.has(p_name)) {
			props.erase(p_name);
			changed = true;
		}
	} else {
		if (props.has(p_name)) {
			if (p_value != props[p_name].variant) {
				props[p_name].variant = p_value;
				changed = true;
			}
		} else {
			props[p_name] = VariantContainer(p_value, last_order++);
			changed = true;
		}

		if (save_changed_setting) {
			if (!props[p_name].save) {
				props[p_name].save = true;
				changed = true;
			}
		}
	}

	return changed;
}

bool AppSettings::_get(const StringName &p_name, Variant &r_ret) const {
	_THREAD_SAFE_METHOD_

	if (p_name == "shortcuts") {
		Array save_array;
		const HashMap<String, List<Ref<InputEvent>>> &builtin_list = InputMap::get_singleton()->get_builtins();
		for (const KeyValue<String, Ref<Shortcut>> &shortcut_definition : shortcuts) {
			Ref<Shortcut> sc = shortcut_definition.value;

			if (builtin_list.has(shortcut_definition.key)) {
				// This shortcut was auto-generated from built in actions: don't save.
				// If the builtin is overridden, it will be saved in the "builtin_action_overrides" section below.
				continue;
			}

			Array shortcut_events = sc->get_events();

			Dictionary dict;
			dict["name"] = shortcut_definition.key;
			dict["shortcuts"] = shortcut_events;

			if (!sc->has_meta("original")) {
				// Getting the meta when it doesn't exist will return an empty array. If the 'shortcut_events' have been cleared,
				// we still want save the shortcut in this case so that shortcuts that the user has customized are not reset,
				// even if the 'original' has not been populated yet. This can happen when calling save() from the Project Manager.
				save_array.push_back(dict);
				continue;
			}

			Array original_events = sc->get_meta("original");

			bool is_same = Shortcut::is_event_array_equal(original_events, shortcut_events);
			if (is_same) {
				continue; // Not changed from default; don't save.
			}

			save_array.push_back(dict);
		}
		r_ret = save_array;
		return true;
	} else if (p_name == "builtin_action_overrides") {
		Array actions_arr;
		for (const KeyValue<String, List<Ref<InputEvent>>> &action_override : builtin_action_overrides) {
			const List<Ref<InputEvent>> *defaults = InputMap::get_singleton()->get_builtins().getptr(action_override.key);
			if (!defaults) {
				continue;
			}

			List<Ref<InputEvent>> events = action_override.value;

			Dictionary action_dict;
			action_dict["name"] = action_override.key;

			// Convert the list to an array, and only keep key events as this is for the app.
			Array events_arr;
			for (const Ref<InputEvent> &ie : events) {
				Ref<InputEventKey> iek = ie;
				if (iek.is_valid()) {
					events_arr.append(iek);
				}
			}

			Array defaults_arr;
			for (const Ref<InputEvent> &default_input_event : *defaults) {
				if (default_input_event.is_valid()) {
					defaults_arr.append(default_input_event);
				}
			}

			bool same = Shortcut::is_event_array_equal(events_arr, defaults_arr);

			// Don't save if same as default.
			if (same) {
				continue;
			}

			action_dict["events"] = events_arr;
			actions_arr.push_back(action_dict);
		}

		r_ret = actions_arr;
		return true;
	}

	const VariantContainer *v = props.getptr(p_name);
	if (!v) {
		return false;
	}
	r_ret = v->variant;
	return true;
}

void AppSettings::_initial_set(const StringName &p_name, const Variant &p_value, bool p_basic) {
	set(p_name, p_value);
	props[p_name].initial = p_value;
	props[p_name].has_default_value = true;
	props[p_name].basic = p_basic;
}

struct _EVCSort {
	String name;
	Variant::Type type = Variant::Type::NIL;
	int order = 0;
	bool basic = false;
	bool save = false;
	bool restart_if_changed = false;

	bool operator<(const _EVCSort &p_vcs) const { return order < p_vcs.order; }
};

void AppSettings::_get_property_list(List<PropertyInfo> *p_list) const {
	_THREAD_SAFE_METHOD_

	RBSet<_EVCSort> vclist;

	for (const KeyValue<String, VariantContainer> &E : props) {
		const VariantContainer *v = &E.value;

		if (v->hide_from_app) {
			continue;
		}

		_EVCSort vc;
		vc.name = E.key;
		vc.order = v->order;
		vc.type = v->variant.get_type();
		vc.basic = v->basic;
		vc.save = v->save;
		if (vc.save) {
			if (v->initial.get_type() != Variant::NIL && v->initial == v->variant) {
				vc.save = false;
			}
		}
		vc.restart_if_changed = v->restart_if_changed;

		vclist.insert(vc);
	}

	for (const _EVCSort &E : vclist) {
		uint32_t pusage = PROPERTY_USAGE_NONE;
		if (E.save || !optimize_save) {
			pusage |= PROPERTY_USAGE_STORAGE;
		}

		if (!E.name.begins_with("_") && !E.name.begins_with("projects/")) {
			pusage |= PROPERTY_USAGE_EDITOR; // TODO
		} else {
			pusage |= PROPERTY_USAGE_STORAGE; //hiddens must always be saved
		}

		PropertyInfo pi(E.type, E.name);
		pi.usage = pusage;
		if (hints.has(E.name)) {
			pi = hints[E.name];
		}

		if (E.basic) {
			pi.usage |= PROPERTY_USAGE_EDITOR_BASIC_SETTING;
		}

		if (E.restart_if_changed) {
			pi.usage |= PROPERTY_USAGE_RESTART_IF_CHANGED;
		}

		p_list->push_back(pi);
	}

	p_list->push_back(PropertyInfo(Variant::ARRAY, "shortcuts", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL)); //do not edit
	p_list->push_back(PropertyInfo(Variant::ARRAY, "builtin_action_overrides", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
}

void AppSettings::_add_property_info_bind(const Dictionary &p_info) {
	ERR_FAIL_COND_MSG(!p_info.has("name"), "Property info is missing \"name\" field.");
	ERR_FAIL_COND_MSG(!p_info.has("type"), "Property info is missing \"type\" field.");

	if (p_info.has("usage")) {
		WARN_PRINT("\"usage\" is not supported in add_property_info().");
	}

	PropertyInfo pinfo;
	pinfo.name = p_info["name"];
	ERR_FAIL_COND(!props.has(pinfo.name));
	pinfo.type = Variant::Type(p_info["type"].operator int());
	ERR_FAIL_INDEX(pinfo.type, Variant::VARIANT_MAX);

	if (p_info.has("hint")) {
		pinfo.hint = PropertyHint(p_info["hint"].operator int());
	}
	if (p_info.has("hint_string")) {
		pinfo.hint_string = p_info["hint_string"];
	}

	add_property_hint(pinfo);
}

// Default configs
bool AppSettings::has_default_value(const String &p_setting) const {
	_THREAD_SAFE_METHOD_

	if (!props.has(p_setting)) {
		return false;
	}
	return props[p_setting].has_default_value;
}

void AppSettings::_set_initialized() {
	initialized = true;
}

static LocalVector<String> _get_skipped_locales() {
	// Skip locales if Text server lack required features.
	LocalVector<String> locales_to_skip;
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT) || !TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("ar"); // Arabic.
		locales_to_skip.push_back("fa"); // Persian.
		locales_to_skip.push_back("ur"); // Urdu.
	}
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT)) {
		locales_to_skip.push_back("he"); // Hebrew.
	}
	if (!TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("bn"); // Bengali.
		locales_to_skip.push_back("hi"); // Hindi.
		locales_to_skip.push_back("ml"); // Malayalam.
		locales_to_skip.push_back("si"); // Sinhala.
		locales_to_skip.push_back("ta"); // Tamil.
		locales_to_skip.push_back("te"); // Telugu.
	}
	return locales_to_skip;
}

void AppSettings::_load_defaults(Ref<ConfigFile> p_extra_config) {
	_THREAD_SAFE_METHOD_
// Sets up the setting with a default value and hint PropertyInfo.
#define APP_SETTING(m_type, m_property_hint, m_name, m_default_value, m_hint_string) \
	_initial_set(m_name, m_default_value);                                           \
	hints[m_name] = PropertyInfo(m_type, m_name, m_property_hint, m_hint_string);

#define APP_SETTING_BASIC(m_type, m_property_hint, m_name, m_default_value, m_hint_string) \
	_initial_set(m_name, m_default_value, true);                                           \
	hints[m_name] = PropertyInfo(m_type, m_name, m_property_hint, m_hint_string);

#define APP_SETTING_USAGE(m_type, m_property_hint, m_name, m_default_value, m_hint_string, m_usage) \
	_initial_set(m_name, m_default_value);                                                          \
	hints[m_name] = PropertyInfo(m_type, m_name, m_property_hint, m_hint_string, m_usage);

	// TODO: Remove
#define EDITOR_SETTING APP_SETTING
#define EDITOR_SETTING_BASIC APP_SETTING_BASIC
#define EDITOR_SETTING_USAGE APP_SETTING_USAGE

	/* Languages */

	{
		String lang_hint;
		const String host_lang = OS::get_singleton()->get_locale();

		// Skip locales which we can't render properly.
		const LocalVector<String> locales_to_skip = _get_skipped_locales();
		if (!locales_to_skip.is_empty()) {
			WARN_PRINT("Some locales are not properly supported by selected Text Server and are disabled.");
		}

		String best = "en";
		int best_score = 0;
		for (const String &locale : get_locales()) {
			// Test against language code without regional variants (e.g. ur_PK).
			String lang_code = locale.get_slicec('_', 0);
			if (locales_to_skip.has(lang_code)) {
				continue;
			}

			lang_hint += ";";
			const String lang_name = TranslationServer::get_singleton()->get_locale_name(locale);
			lang_hint += vformat("%s/[%s] %s", locale, locale, lang_name);

			int score = TranslationServer::get_singleton()->compare_locales(host_lang, locale);
			if (score > 0 && score >= best_score) {
				best = locale;
				best_score = score;
			}
		}
		lang_hint = vformat(";auto/Auto (%s);en/[en] English", TranslationServer::get_singleton()->get_locale_name(best)) + lang_hint;

		EDITOR_SETTING_USAGE(Variant::STRING, PROPERTY_HINT_ENUM, "interface/editor/editor_language", "auto", lang_hint, PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_RESTART_IF_CHANGED | PROPERTY_USAGE_EDITOR_BASIC_SETTING);
	}

	APP_SETTING(Variant::STRING, PROPERTY_HINT_NONE, "app_version", "0.0.1", "")

	// Display what the Auto display scale setting effectively corresponds to.
	const String display_scale_hint_string = vformat("Auto (%d%%),75%%,100%%,125%%,150%%,175%%,200%%,Custom", Math::round(get_auto_display_scale() * 100));
	APP_SETTING_USAGE(Variant::INT, PROPERTY_HINT_ENUM, "interface/app/display_scale", 0, display_scale_hint_string, PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_RESTART_IF_CHANGED | PROPERTY_USAGE_EDITOR_BASIC_SETTING)
	APP_SETTING_USAGE(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/app/custom_display_scale", 1.0, "0.5,3,0.01", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_RESTART_IF_CHANGED | PROPERTY_USAGE_EDITOR_BASIC_SETTING)

	APP_SETTING_USAGE(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/use_embedded_menu", false, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_BASIC_SETTING)
	APP_SETTING_USAGE(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/use_native_file_dialogs", false, "", PROPERTY_USAGE_DEFAULT)
	APP_SETTING_USAGE(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/expand_to_title", true, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_RESTART_IF_CHANGED | PROPERTY_USAGE_EDITOR_BASIC_SETTING)

	// Font
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "interface/app/main_font_size", 14, "8,48,1")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "interface/app/code_font_size", 14, "8,48,1")
	_initial_set("interface/app/main_font_custom_opentype_features", "");
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/app/code_font_contextual_ligatures", 1, "Enabled,Disable Contextual Alternates (Coding Ligatures),Use Custom OpenType Feature Set")
	_initial_set("interface/app/code_font_custom_opentype_features", "");
	_initial_set("interface/app/code_font_custom_variations", "");
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_ENUM, "interface/app/font_antialiasing", 1, "None,Grayscale,LCD Subpixel")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_ENUM, "interface/app/font_hinting", 0, "Auto (Light),None,Light,Normal")
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/app/font_subpixel_positioning", 1, "Disabled,Auto,One Half of a Pixel,One Quarter of a Pixel")
	APP_SETTING(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/font_disable_embedded_bitmaps", true, "");
	APP_SETTING(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/font_allow_msdf", true, "")

	APP_SETTING(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "interface/app/main_font", "", "*.ttf,*.otf,*.woff,*.woff2,*.pfb,*.pfm")
	APP_SETTING(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "interface/app/main_font_bold", "", "*.ttf,*.otf,*.woff,*.woff2,*.pfb,*.pfm")
	APP_SETTING(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "interface/app/code_font", "", "*.ttf,*.otf,*.woff,*.woff2,*.pfb,*.pfm")
	EDITOR_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/app/dragging_hover_wait_seconds", 0.5, "0.01,10,0.01,or_greater,suffix:s");

	APP_SETTING(Variant::BOOL, PROPERTY_HINT_NONE, "interface/app/collapse_main_menu", false, "")

	// Theme
	EDITOR_SETTING_BASIC(Variant::BOOL, PROPERTY_HINT_ENUM, "interface/theme/follow_system_theme", false, "")
	EDITOR_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "interface/theme/style", "Modern", "Modern,Classic")
	EDITOR_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "interface/theme/color_preset", "Default", "Default,Breeze Dark,Godot 2,Godot 3,Gray,Light,Solarized (Dark),Solarized (Light),Black (OLED),Custom")
	EDITOR_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "interface/theme/spacing_preset", "Default", "Compact,Default,Spacious,Custom")
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/theme/icon_and_font_color", 0, "Auto,Dark,Light")
	EDITOR_SETTING_BASIC(Variant::COLOR, PROPERTY_HINT_NONE, "interface/theme/base_color", Color(0.14, 0.14, 0.14), "")
	EDITOR_SETTING_BASIC(Variant::COLOR, PROPERTY_HINT_NONE, "interface/theme/accent_color", Color(0.34, 0.62, 1.0), "")
	EDITOR_SETTING_BASIC(Variant::BOOL, PROPERTY_HINT_NONE, "interface/theme/use_system_accent_color", false, "")
	EDITOR_SETTING_BASIC(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/contrast", 0.3, "-1,1,0.01")
	EDITOR_SETTING(Variant::BOOL, PROPERTY_HINT_NONE, "interface/theme/draw_extra_borders", false, "")
	EDITOR_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/icon_saturation", 2.0, "0,2,0.01")
	// EDITOR_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/theme/draw_relationship_lines", (int32_t)AppThemeManager::RELATIONSHIP_SELECTED_ONLY, "None,Selected Only,All")
	EDITOR_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/relationship_line_opacity", 0.1, "0.00,1,0.01")
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/border_size", 0, "0,2,1")
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/corner_radius", 4, "0,6,1")
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/base_spacing", 4, "0,8,1")
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/additional_spacing", 0, "0,8,1")
	EDITOR_SETTING_USAGE(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "interface/theme/custom_theme", "", "*.res,*.tres,*.theme", PROPERTY_USAGE_DEFAULT)

	// Tabs
	EDITOR_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/tabs/display_close_button", 1, "Never,If Tab Active,Always"); // TabBar::CloseButtonDisplayPolicy
	_initial_set("interface/tabs/show_thumbnail_on_hover", true);
	EDITOR_SETTING_USAGE(Variant::INT, PROPERTY_HINT_RANGE, "interface/tabs/maximum_width", 350, "0,9999,1", PROPERTY_USAGE_DEFAULT)

	// File dialog
	_initial_set("filesystem/file_dialog/show_hidden_files", false);
	// APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "filesystem/file_dialog/display_mode", 0, "Thumbnails,List")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "filesystem/file_dialog/thumbnail_size", 64, "32,128,16")

	/* Text editor */

	// Theme
	APP_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "text_editor/theme/color_theme", "Default", "Default,Godot 2,Custom")

	// Theme: Highlighting
	const LocalVector<StringName> basic_text_editor_settings = {
		"text_editor/theme/highlighting/symbol_color",
		"text_editor/theme/highlighting/keyword_color",
		"text_editor/theme/highlighting/control_flow_keyword_color",
		"text_editor/theme/highlighting/base_type_color",
		"text_editor/theme/highlighting/engine_type_color",
		"text_editor/theme/highlighting/user_type_color",
		"text_editor/theme/highlighting/comment_color",
		"text_editor/theme/highlighting/doc_comment_color",
		"text_editor/theme/highlighting/string_color",
		"text_editor/theme/highlighting/background_color",
		"text_editor/theme/highlighting/text_color",
		"text_editor/theme/highlighting/line_number_color",
		"text_editor/theme/highlighting/safe_line_number_color",
		"text_editor/theme/highlighting/caret_color",
		"text_editor/theme/highlighting/caret_background_color",
		"text_editor/theme/highlighting/text_selected_color",
		"text_editor/theme/highlighting/selection_color",
		"text_editor/theme/highlighting/brace_mismatch_color",
		"text_editor/theme/highlighting/current_line_color",
		"text_editor/theme/highlighting/line_length_guideline_color",
		"text_editor/theme/highlighting/word_highlighted_color",
		"text_editor/theme/highlighting/number_color",
		"text_editor/theme/highlighting/function_color",
		"text_editor/theme/highlighting/member_variable_color",
		"text_editor/theme/highlighting/mark_color",
	};

	// TODO: text editor theme
	// // These values will be overwritten by AppThemeManager, but can still be seen in some edge cases.
	// const HashMap<StringName, Color> text_colors = get_godot2_text_editor_theme();
	// for (const KeyValue<StringName, Color> &text_color : text_colors) {
	// 	if (basic_text_editor_settings.has(text_color.key)) {
	// 		APP_SETTING_BASIC(Variant::COLOR, PROPERTY_HINT_NONE, text_color.key, text_color.value, "")
	// 	} else {
	// 		APP_SETTING(Variant::COLOR, PROPERTY_HINT_NONE, text_color.key, text_color.value, "")
	// 	}
	// }

	// The list is based on <https://github.com/KDE/syntax-highlighting/blob/master/data/syntax/alert.xml>.
	_initial_set("text_editor/theme/highlighting/comment_markers/critical_list", "ALERT,ATTENTION,CAUTION,CRITICAL,DANGER,SECURITY");
	_initial_set("text_editor/theme/highlighting/comment_markers/warning_list", "BUG,DEPRECATED,FIXME,HACK,TASK,TBD,TODO,WARNING");
	_initial_set("text_editor/theme/highlighting/comment_markers/notice_list", "INFO,NOTE,NOTICE,TEST,TESTING");

	// Appearance
	APP_SETTING_BASIC(Variant::BOOL, PROPERTY_HINT_NONE, "text_editor/appearance/enable_inline_color_picker", true, "");

	// Appearance: Caret
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/appearance/caret/type", 0, "Line,Block")
	_initial_set("text_editor/appearance/caret/caret_blink", true, true);
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "text_editor/appearance/caret/caret_blink_interval", 0.5, "0.1,10,0.01")
	_initial_set("text_editor/appearance/caret/highlight_current_line", true, true);
	_initial_set("text_editor/appearance/caret/highlight_all_occurrences", true, true);

	// Appearance: Guidelines
	_initial_set("text_editor/appearance/guidelines/show_line_length_guidelines", true, true);
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/appearance/guidelines/line_length_guideline_soft_column", 80, "20,160,1")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/appearance/guidelines/line_length_guideline_hard_column", 100, "20,160,1")

	// Appearance: Gutters
	_initial_set("text_editor/appearance/gutters/show_line_numbers", true, true);
	_initial_set("text_editor/appearance/gutters/line_numbers_zero_padded", false, true);
	_initial_set("text_editor/appearance/gutters/highlight_type_safe_lines", true, true);
	_initial_set("text_editor/appearance/gutters/show_info_gutter", true, true);

	// Appearance: Minimap
	_initial_set("text_editor/appearance/minimap/show_minimap", true, true);
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/appearance/minimap/minimap_width", 80, "50,250,1")

	// Appearance: Lines
	_initial_set("text_editor/appearance/lines/code_folding", true, true);
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/appearance/lines/word_wrap", 0, "None,Boundary")
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/appearance/lines/autowrap_mode", 3, "Arbitrary:1,Word:2,Word (Smart):3")

	// Appearance: Whitespace
	_initial_set("text_editor/appearance/whitespace/draw_tabs", true, true);
	_initial_set("text_editor/appearance/whitespace/draw_spaces", false, true);
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/appearance/whitespace/line_spacing", 4, "0,50,1")

	// Behavior
	// Behavior: General
	_initial_set("text_editor/behavior/general/empty_selection_clipboard", true);

	// Behavior: Navigation
	_initial_set("text_editor/behavior/navigation/move_caret_on_right_click", true, true);
	_initial_set("text_editor/behavior/navigation/scroll_past_end_of_file", false, true);
	_initial_set("text_editor/behavior/navigation/smooth_scrolling", true, true);
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/behavior/navigation/v_scroll_speed", 80, "1,10000,1")
	_initial_set("text_editor/behavior/navigation/drag_and_drop_selection", true, true);
	_initial_set("text_editor/behavior/navigation/stay_in_script_editor_on_node_selected", true, true);
	_initial_set("text_editor/behavior/navigation/open_script_when_connecting_signal_to_existing_method", true, true);
	_initial_set("text_editor/behavior/navigation/use_default_word_separators", true); // Includes ´`~$^=+|<> General punctuation and CJK punctuation.
	_initial_set("text_editor/behavior/navigation/use_custom_word_separators", false);
	_initial_set("text_editor/behavior/navigation/custom_word_separators", ""); // Custom word separators.

	// Behavior: Indent
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/behavior/indent/type", 0, "Tabs,Spaces")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/behavior/indent/size", 4, "1,64,1") // size of 0 crashes.
	_initial_set("text_editor/behavior/indent/auto_indent", true);
	_initial_set("text_editor/behavior/indent/indent_wrapped_lines", true);

	// Behavior: Files
	_initial_set("text_editor/behavior/files/trim_trailing_whitespace_on_save", false);
	_initial_set("text_editor/behavior/files/trim_final_newlines_on_save", true);
	_initial_set("text_editor/behavior/files/autosave_interval_secs", 0);
	_initial_set("text_editor/behavior/files/restore_scripts_on_load", true);
	_initial_set("text_editor/behavior/files/convert_indent_on_save", true);
	_initial_set("text_editor/behavior/files/auto_reload_scripts_on_external_change", true);
	_initial_set("text_editor/behavior/files/auto_reload_and_parse_scripts_on_save", true);
	_initial_set("text_editor/behavior/files/open_dominant_script_on_scene_change", false, true);
	_initial_set("text_editor/behavior/files/drop_preload_resources_as_uid", true, true);

	// Behavior: Documentation
	_initial_set("text_editor/behavior/documentation/enable_tooltips", true, true);

	// Script list
	_initial_set("text_editor/script_list/show_members_overview", true, true);
	_initial_set("text_editor/script_list/sort_members_outline_alphabetically", false, true);
	_initial_set("text_editor/script_list/script_temperature_enabled", true);
	_initial_set("text_editor/script_list/script_temperature_history_size", 15);
	_initial_set("text_editor/script_list/highlight_scene_scripts", true);
	_initial_set("text_editor/script_list/group_help_pages", true);
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/script_list/sort_scripts_by", 0, "Name,Path,None");
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/script_list/list_script_names_as", 0, "Name,Parent Directory And Name,Full Path");
	APP_SETTING(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "text_editor/external/exec_path", "", "");
	APP_SETTING(Variant::STRING, PROPERTY_HINT_PLACEHOLDER_TEXT, "text_editor/external/exec_flags", "{file}", "Call flags with placeholders: {project}, {file}, {col}, {line}.");

	// Completion
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "text_editor/completion/idle_parse_delay", 1.5, "0.1,10,0.01")
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "text_editor/completion/idle_parse_delay_with_errors_found", 0.5, "0.1,5,0.01")
	_initial_set("text_editor/completion/auto_brace_complete", true, true);
	_initial_set("text_editor/completion/code_complete_enabled", true, true);
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "text_editor/completion/code_complete_delay", 0.3, "0.01,5,0.01,or_greater")
	_initial_set("text_editor/completion/put_callhint_tooltip_below_current_line", true);
	_initial_set("text_editor/completion/complete_file_paths", true);
	_initial_set("text_editor/completion/add_type_hints", true, true);
	_initial_set("text_editor/completion/add_string_name_literals", false, true);
	_initial_set("text_editor/completion/add_node_path_literals", false, true);
	_initial_set("text_editor/completion/use_single_quotes", false, true);
	_initial_set("text_editor/completion/colorize_suggestions", true);

	// External editor (ScriptEditorPlugin)
	_initial_set("text_editor/external/use_external_editor", false, true);
	_initial_set("text_editor/external/exec_path", "");

	// Help
	_initial_set("text_editor/help/show_help_index", true);
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/help/help_font_size", 16, "8,48,1")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/help/help_source_font_size", 15, "8,48,1")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "text_editor/help/help_title_font_size", 23, "8,64,1")
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_ENUM, "text_editor/help/class_reference_examples", 0, "GDScript,C#,GDScript and C#")
	_initial_set("text_editor/help/sort_functions_alphabetically", true);

	/* Editors */

	// Output
	APP_SETTING_BASIC(Variant::INT, PROPERTY_HINT_RANGE, "run/output/font_size", 13, "8,48,1")
	_initial_set("run/output/always_clear_output_on_play", true, true);

	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "run/output/max_lines", 10000, "100,100000,1")

#undef APP_SETTING
#undef APP_SETTING_BASIC
#undef APP_SETTING_USAGE

	if (p_extra_config.is_valid()) {
		if (p_extra_config->has_section("init_projects") && p_extra_config->has_section_key("init_projects", "list")) {
			Vector<String> list = p_extra_config->get_value("init_projects", "list");
			for (int i = 0; i < list.size(); i++) {
				String proj_name = list[i].replace("/", "::");
				set("projects/" + proj_name, list[i]);
			}
		}

		if (p_extra_config->has_section("presets")) {
			Vector<String> keys = p_extra_config->get_section_keys("presets");

			for (const String &key : keys) {
				Variant val = p_extra_config->get_value("presets", key);
				set(key, val);
			}
		}
	}
}

// PUBLIC METHODS

AppSettings *AppSettings::get_singleton() {
	return singleton.ptr();
}

String AppSettings::get_settings_path() {
	const String config_file_name = "app_settings.tres";
	return AppPaths::get_singleton()->get_config_dir().path_join(config_file_name);
}

void AppSettings::create() {
	// IMPORTANT: create() *must* create a valid AppSettings singleton,
	// as the rest of the engine code will assume it. As such, it should never
	// return (incl. via ERR_FAIL) without initializing the singleton member.

	if (singleton.ptr()) {
		ERR_PRINT("Can't recreate AppSettings as it already exists.");
		return;
	}

	bool success = false;
	String config_file_path = get_settings_path();

	do {
		if (!AppPaths::get_singleton()->are_paths_valid()) {
			break;
		}

		if (!FileSystemAccess::file_exists(config_file_path)) {
			break;
		}

		singleton = ResourceLoader::load(config_file_path, "AppSettings");
		if (singleton.is_null()) {
			ERR_PRINT("Could not load settings from path: " + config_file_path);
			break;
		}

		singleton->save_changed_setting = true;

		print_verbose("AppSettings: Load OK!");

		singleton->setup_language(true);

		// TODO
		// singleton->list_text_editor_themes();

		success = true;
	} while (0);

	if (!success) {
		// patch init projects
		String exe_path = OS::get_singleton()->get_executable_path().get_base_dir();

		singleton.instantiate();
		singleton->set_path(config_file_path, true);
		singleton->save_changed_setting = true;
		singleton->_load_defaults();
		singleton->setup_language(true);

		// TODO
		// singleton->list_text_editor_themes();

		singleton->set_manually("app_version", vformat("%d.%d.%d", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH));
		singleton->save();
	}
}

void EditorSettings::setup_language(bool p_initial_setup) {
	// String lang = _EDITOR_GET("interface/app/language");
	String lang = get_language();
	if (p_initial_setup) {
		String lang_ov = Main::get_locale_override();
		if (!lang_ov.is_empty()) {
			lang = lang_ov;
		}
	}

	if (lang == "en") {
		TranslationServer::get_singleton()->set_locale(lang);
		return; // Default, nothing to do.
	}
	load_translations(lang);

	TranslationServer::get_singleton()->set_locale(lang);
}

void AppSettings::save() {
	//_THREAD_SAFE_METHOD_

	if (!singleton.ptr()) {
		return;
	}

	Error err = ResourceSaver::save(singleton);

	if (err != OK) {
		ERR_PRINT("Error saving app settings to " + singleton->get_path());
	} else {
		singleton->changed_settings.clear();
		print_verbose("AppSettings: Save OK!");
	}
}

PackedStringArray AppSettings::get_changed_settings() const {
	PackedStringArray arr;
	for (const String &setting : changed_settings) {
		arr.push_back(setting);
	}

	return arr;
}

bool AppSettings::check_changed_settings_in_group(const String &p_setting_prefix) const {
	for (const String &setting : changed_settings) {
		if (setting.begins_with(p_setting_prefix)) {
			return true;
		}
	}

	return false;
}

void AppSettings::mark_setting_changed(const String &p_setting) {
	changed_settings.insert(p_setting);
}

void AppSettings::destroy() {
	if (!singleton.ptr()) {
		return;
	}
	save();
	singleton = Ref<AppSettings>();
}

void AppSettings::set_optimize_save(bool p_optimize) {
	optimize_save = p_optimize;
}

// Properties

void AppSettings::set_setting(const String &p_setting, const Variant &p_value) {
	_THREAD_SAFE_METHOD_
	set(p_setting, p_value);
}

Variant AppSettings::get_setting(const String &p_setting) const {
	_THREAD_SAFE_METHOD_
	// TODO
	// if (ProjectSettings::get_singleton()->has_editor_setting_override(p_setting)) {
	// 	return ProjectSettings::get_singleton()->get_editor_setting_override(p_setting);
	// }
	return get(p_setting);
}

bool AppSettings::has_setting(const String &p_setting) const {
	_THREAD_SAFE_METHOD_

	return props.has(p_setting);
}

void AppSettings::erase(const String &p_setting) {
	_THREAD_SAFE_METHOD_

	props.erase(p_setting);
}

void AppSettings::raise_order(const String &p_setting) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!props.has(p_setting));
	props[p_setting].order = ++last_order;
}

void AppSettings::set_restart_if_changed(const StringName &p_setting, bool p_restart) {
	_THREAD_SAFE_METHOD_

	if (!props.has(p_setting)) {
		return;
	}
	props[p_setting].restart_if_changed = p_restart;
}

void AppSettings::set_basic(const StringName &p_setting, bool p_basic) {
	_THREAD_SAFE_METHOD_

	if (!props.has(p_setting)) {
		return;
	}
	props[p_setting].basic = p_basic;
}

void AppSettings::set_initial_value(const StringName &p_setting, const Variant &p_value, bool p_update_current) {
	_THREAD_SAFE_METHOD_

	if (!props.has(p_setting)) {
		return;
	}
	props[p_setting].initial = p_value;
	props[p_setting].has_default_value = true;
	if (p_update_current) {
		set(p_setting, p_value);
	}
}

Variant _APP_DEF(const String &p_setting, const Variant &p_default, bool p_restart_if_changed, bool p_basic) {
	ERR_FAIL_NULL_V_MSG(AppSettings::get_singleton(), p_default, "AppSettings not instantiated yet.");

	Variant ret = p_default;
	if (AppSettings::get_singleton()->has_setting(p_setting)) {
		ret = APP_GET(p_setting);
	} else {
		AppSettings::get_singleton()->set_manually(p_setting, p_default);
	}
	AppSettings::get_singleton()->set_restart_if_changed(p_setting, p_restart_if_changed);
	AppSettings::get_singleton()->set_basic(p_setting, p_basic);

	if (!AppSettings::get_singleton()->has_default_value(p_setting)) {
		AppSettings::get_singleton()->set_initial_value(p_setting, p_default);
	}

	return ret;
}

Variant _APP_GET(const String &p_setting) {
	ERR_FAIL_NULL_V_MSG(EditorSettings::get_singleton(), Variant(), vformat(R"(EditorSettings not instantiated yet when getting setting "%s".)", p_setting));
	ERR_FAIL_COND_V_MSG(!EditorSettings::get_singleton()->has_setting(p_setting), Variant(), vformat(R"(Editor setting "%s" does not exist.)", p_setting));
	return EditorSettings::get_singleton()->get_setting(p_setting);
}

bool AppSettings::_property_can_revert(const StringName &p_name) const {
	const VariantContainer *property = props.getptr(p_name);
	if (property) {
		return property->has_default_value;
	}
	return false;
}

bool AppSettings::_property_get_revert(const StringName &p_name, Variant &r_property) const {
	const VariantContainer *value = props.getptr(p_name);
	if (value && value->has_default_value) {
		r_property = value->initial;
		return true;
	}
	return false;
}

void AppSettings::add_property_hint(const PropertyInfo &p_hint) {
	_THREAD_SAFE_METHOD_

	hints[p_hint.name] = p_hint;
}

float AppSettings::get_auto_display_scale() {
	const int screen = DisplayServer::get_singleton()->window_get_current_screen();

	if (DisplayServer::get_singleton()->screen_get_size(screen) == Vector2i()) {
		// Invalid screen size, skip.
		return 1.0;
	}

#if defined(WINDOWS_ENABLED)
	return DisplayServer::get_singleton()->screen_get_dpi(screen) / 96.0;
#else
	// Use the smallest dimension to use a correct display scale on portrait displays.
	const int smallest_dimension = MIN(DisplayServer::get_singleton()->screen_get_size(screen).x, DisplayServer::get_singleton()->screen_get_size(screen).y);
	if (DisplayServer::get_singleton()->screen_get_dpi(screen) >= 192 && smallest_dimension >= 1400) {
		// hiDPI display.
		return 2.0;
	} else if (smallest_dimension >= 1700) {
		// Likely a hiDPI display, but we aren't certain due to the returned DPI.
		// Use an intermediate scale to handle this situation.
		return 1.5;
	} else if (smallest_dimension <= 800) {
		// Small loDPI display. Use a smaller display scale so that editor elements fit more easily.
		// Icons won't look great, but this is better than having editor elements overflow from its window.
		return 0.75;
	}
	return 1.0;
#endif // defined(WINDOWS_ENABLED)
}

String EditorSettings::get_language() const {
	const String language = has_setting("interface/editor/editor_language") ? get("interface/editor/editor_language") : "auto";
	if (language != "auto") {
		return language;
	}

	if (auto_language.is_empty()) {
		// Skip locales which we can't render properly.
		const LocalVector<String> locales_to_skip = _get_skipped_locales();
		const String host_lang = OS::get_singleton()->get_locale();

		String best = "en";
		int best_score = 0;
		for (const String &locale : get_locales()) {
			// Test against language code without regional variants (e.g. ur_PK).
			String lang_code = locale.get_slicec('_', 0);
			if (locales_to_skip.has(lang_code)) {
				continue;
			}

			int score = TranslationServer::get_singleton()->compare_locales(host_lang, locale);
			if (score > 0 && score >= best_score) {
				best = locale;
				best_score = score;
			}
		}
		auto_language = best;
	}
	return auto_language;
}

// Shortcuts

void EditorSettings::_add_shortcut_default(const String &p_path, const Ref<Shortcut> &p_shortcut) {
	shortcuts[p_path] = p_shortcut;
}

void EditorSettings::add_shortcut(const String &p_path, const Ref<Shortcut> &p_shortcut) {
	ERR_FAIL_COND_MSG(p_shortcut.is_null(), "Cannot add a null shortcut for path: " + p_path);

	Array use_events = p_shortcut->get_events();
	if (shortcuts.has(p_path)) {
		Ref<Shortcut> existing = shortcuts.get(p_path);
		if (!existing->has_meta("original")) {
			// Loaded from editor settings, but plugin not loaded yet.
			// Keep the events from editor settings but still override the shortcut in the shortcuts map
			use_events = existing->get_events();
		} else if (!Shortcut::is_event_array_equal(existing->get_events(), existing->get_meta("original"))) {
			// Shortcut exists and is customized - don't override with default.
			return;
		}
	}

	p_shortcut->set_meta("original", p_shortcut->get_events());
	p_shortcut->set_events(use_events);
	if (p_shortcut->get_name().is_empty()) {
		String shortcut_name = p_path.get_slicec('/', 1);
		if (shortcut_name.is_empty()) {
			shortcut_name = p_path;
		}
		p_shortcut->set_name(shortcut_name);
	}
	shortcuts[p_path] = p_shortcut;
}

void EditorSettings::remove_shortcut(const String &p_path) {
	shortcuts.erase(p_path);
}

bool EditorSettings::is_shortcut(const String &p_path, const Ref<InputEvent> &p_event) const {
	HashMap<String, Ref<Shortcut>>::ConstIterator E = shortcuts.find(p_path);
	ERR_FAIL_COND_V_MSG(!E, false, "Unknown Shortcut: " + p_path + ".");

	return E->value->matches_event(p_event);
}

bool EditorSettings::has_shortcut(const String &p_path) const {
	return get_shortcut(p_path).is_valid();
}

Ref<Shortcut> EditorSettings::get_shortcut(const String &p_path) const {
	HashMap<String, Ref<Shortcut>>::ConstIterator SC = shortcuts.find(p_path);
	if (SC) {
		return SC->value;
	}

	// If no shortcut with the provided name is found in the list, check the built-in shortcuts.
	// Use the first item in the action list for the shortcut event, since a shortcut can only have 1 linked event.

	Ref<Shortcut> sc;
	HashMap<String, List<Ref<InputEvent>>>::ConstIterator builtin_override = builtin_action_overrides.find(p_path);
	if (builtin_override) {
		sc.instantiate();
		sc->set_events_list(&builtin_override->value);
		sc->set_name(InputMap::get_singleton()->get_builtin_display_name(p_path));
	}

	// If there was no override, check the default builtins to see if it has an InputEvent for the provided name.
	if (sc.is_null()) {
		HashMap<String, List<Ref<InputEvent>>>::ConstIterator builtin_default = InputMap::get_singleton()->get_builtins_with_feature_overrides_applied().find(p_path);
		if (builtin_default) {
			sc.instantiate();
			sc->set_events_list(&builtin_default->value);
			sc->set_name(InputMap::get_singleton()->get_builtin_display_name(p_path));
		}
	}

	if (sc.is_valid()) {
		// Add the shortcut to the list.
		shortcuts[p_path] = sc;
		return sc;
	}

	return Ref<Shortcut>();
}

Vector<String> EditorSettings::_get_shortcut_list() {
	List<String> shortcut_list;
	get_shortcut_list(&shortcut_list);
	Vector<String> ret;
	for (const String &shortcut : shortcut_list) {
		ret.push_back(shortcut);
	}
	return ret;
}

void AppSettings::get_shortcut_list(List<String> *r_shortcuts) {
	for (const KeyValue<String, Ref<Shortcut>> &E : shortcuts) {
		r_shortcuts->push_back(E.key);
	}
}

Ref<Shortcut> APP_GET_SHORTCUT(const String &p_path) {
	ERR_FAIL_NULL_V_MSG(AppSettings::get_singleton(), nullptr, "AppSettings not instantiated yet.");

	Ref<Shortcut> sc = AppSettings::get_singleton()->get_shortcut(p_path);

	ERR_FAIL_COND_V_MSG(sc.is_null(), sc, "Used APP_GET_SHORTCUT with invalid shortcut: " + p_path);

	return sc;
}

void APP_SHORTCUT_OVERRIDE(const String &p_path, const String &p_feature, Key p_keycode, bool p_physical) {
	if (!AppSettings::get_singleton()) {
		return;
	}

	Ref<Shortcut> sc = AppSettings::get_singleton()->get_shortcut(p_path);
	ERR_FAIL_COND_MSG(sc.is_null(), "Used APP_SHORTCUT_OVERRIDE with invalid shortcut: " + p_path);

	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);

	APP_SHORTCUT_OVERRIDE_ARRAY(p_path, p_feature, arr, p_physical);
}

void APP_SHORTCUT_OVERRIDE_ARRAY(const String &p_path, const String &p_feature, const PackedInt32Array &p_keycodes, bool p_physical) {
	if (!AppSettings::get_singleton()) {
		return;
	}

	Ref<Shortcut> sc = AppSettings::get_singleton()->get_shortcut(p_path);
	ERR_FAIL_COND_MSG(sc.is_null(), "Used APP_SHORTCUT_OVERRIDE_ARRAY with invalid shortcut: " + p_path);

	// Only add the override if the OS supports the provided feature.
	if (!OS::get_singleton()->has_feature(p_feature)) {
		if (!(p_feature == "macos" && (OS::get_singleton()->has_feature("web_macos") || OS::get_singleton()->has_feature("web_ios")))) {
			return;
		}
	}

	Array events;

	for (int i = 0; i < p_keycodes.size(); i++) {
		Key keycode = (Key)p_keycodes[i];

		if (OS::prefer_meta_over_ctrl()) {
			// Use Cmd+Backspace as a general replacement for Delete shortcuts on macOS
			if (keycode == Key::KEY_DELETE) {
				keycode = KeyModifierMask::META | Key::BACKSPACE;
			}
		}

		Ref<InputEventKey> ie;
		if (keycode != Key::NONE) {
			ie = InputEventKey::create_reference(keycode, p_physical);
			events.push_back(ie);
		}
	}

	// Override the existing shortcut only if it wasn't customized by the user.
	if (Shortcut::is_event_array_equal(sc->get_events(), sc->get_meta("original"))) {
		sc->set_events(events);
	}

	sc->set_meta("original", events.duplicate(true));
}

Ref<Shortcut> APP_SHORTCUT(const String &p_path, const String &p_name, Key p_keycode, bool p_physical) {
	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);
	return APP_SHORTCUT_ARRAY(p_path, p_name, arr, p_physical);
}

Ref<Shortcut> APP_SHORTCUT_ARRAY(const String &p_path, const String &p_name, const PackedInt32Array &p_keycodes, bool p_physical) {
	Array events;

	for (int i = 0; i < p_keycodes.size(); i++) {
		Key keycode = (Key)p_keycodes[i];

		if (OS::prefer_meta_over_ctrl()) {
			// Use Cmd+Backspace as a general replacement for Delete shortcuts on macOS
			if (keycode == Key::KEY_DELETE) {
				keycode = KeyModifierMask::META | Key::BACKSPACE;
			}
		}

		Ref<InputEventKey> ie;
		if (keycode != Key::NONE) {
			ie = InputEventKey::create_reference(keycode, p_physical);
			events.push_back(ie);
		}
	}

	if (!AppSettings::get_singleton()) {
		Ref<Shortcut> sc;
		sc.instantiate();
		sc->set_name(p_name);
		sc->set_events(events);
		sc->set_meta("original", events.duplicate(true));
		return sc;
	}

	Ref<Shortcut> sc = AppSettings::get_singleton()->get_shortcut(p_path);
	if (sc.is_valid()) {
		sc->set_name(p_name); //keep name (the ones that come from disk have no name)
		sc->set_meta("original", events.duplicate(true)); //to compare against changes
		return sc;
	}

	sc.instantiate();
	sc->set_name(p_name);
	sc->set_events(events);
	sc->set_meta("original", events.duplicate(true)); //to compare against changes
	AppSettings::get_singleton()->_add_shortcut_default(p_path, sc);

	return sc;
}

void AppSettings::set_builtin_action_override(const String &p_name, const TypedArray<InputEvent> &p_events) {
	List<Ref<InputEvent>> event_list;

	// Override the whole list, since events may have their order changed or be added, removed or edited.
	InputMap::get_singleton()->action_erase_events(p_name);
	for (int i = 0; i < p_events.size(); i++) {
		event_list.push_back(p_events[i]);
		InputMap::get_singleton()->action_add_event(p_name, p_events[i]);
	}

	// Check if the provided event array is same as built-in. If it is, it does not need to be added to the overrides.
	// Note that event order must also be the same.
	bool same_as_builtin = true;
	HashMap<String, List<Ref<InputEvent>>>::ConstIterator builtin_default = InputMap::get_singleton()->get_builtins_with_feature_overrides_applied().find(p_name);
	if (builtin_default) {
		const List<Ref<InputEvent>> &builtin_events = builtin_default->value;

		// In the editor we only care about key events.
		List<Ref<InputEventKey>> builtin_key_events;
		for (Ref<InputEventKey> iek : builtin_events) {
			if (iek.is_valid()) {
				builtin_key_events.push_back(iek);
			}
		}

		if (p_events.size() == builtin_key_events.size()) {
			int event_idx = 0;

			// Check equality of each event.
			for (const Ref<InputEventKey> &E : builtin_key_events) {
				if (!E->is_match(p_events[event_idx])) {
					same_as_builtin = false;
					break;
				}
				event_idx++;
			}
		} else {
			same_as_builtin = false;
		}
	}

	if (same_as_builtin && builtin_action_overrides.has(p_name)) {
		builtin_action_overrides.erase(p_name);
	} else {
		builtin_action_overrides[p_name] = event_list;
	}

	// Update the shortcut (if it is used somewhere in the editor) to be the first event of the new list.
	if (shortcuts.has(p_name)) {
		shortcuts[p_name]->set_events_list(&event_list);
	}
}

const Array AppSettings::get_builtin_action_overrides(const String &p_name) const {
	HashMap<String, List<Ref<InputEvent>>>::ConstIterator AO = builtin_action_overrides.find(p_name);
	if (AO) {
		Array event_array;

		List<Ref<InputEvent>> events_list = AO->value;
		for (const Ref<InputEvent> &E : events_list) {
			event_array.push_back(E);
		}
		return event_array;
	}

	return Array();
}

void AppSettings::notify_changes() {
	_THREAD_SAFE_METHOD_

	SceneTree *sml = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());

	if (!sml) {
		return;
	}

	Node *root = sml->get_root()->get_child(0);

	if (!root) {
		return;
	}
	root->propagate_notification(NOTIFICATION_APP_SETTINGS_CHANGED);
}

void AppSettings::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_setting", "name"), &AppSettings::has_setting);
	ClassDB::bind_method(D_METHOD("set_setting", "name", "value"), &AppSettings::set_setting);
	ClassDB::bind_method(D_METHOD("get_setting", "name"), &AppSettings::get_setting);
	ClassDB::bind_method(D_METHOD("erase", "property"), &AppSettings::erase);
	ClassDB::bind_method(D_METHOD("set_initial_value", "name", "value", "update_current"), &AppSettings::set_initial_value);
	ClassDB::bind_method(D_METHOD("add_property_info", "info"), &AppSettings::_add_property_info_bind);

	ClassDB::bind_method(D_METHOD("set_builtin_action_override", "name", "actions_list"), &AppSettings::set_builtin_action_override);

	ClassDB::bind_method(D_METHOD("check_changed_settings_in_group", "setting_prefix"), &AppSettings::check_changed_settings_in_group);
	ClassDB::bind_method(D_METHOD("get_changed_settings"), &AppSettings::get_changed_settings);
	ClassDB::bind_method(D_METHOD("mark_setting_changed", "setting"), &AppSettings::mark_setting_changed);

	ADD_SIGNAL(MethodInfo("settings_changed"));

	BIND_CONSTANT(NOTIFICATION_APP_SETTINGS_CHANGED);
}

AppSettings::AppSettings() {
	last_order = 0;

	_load_defaults();
	callable_mp(this, &AppSettings::_set_initialized).call_deferred();
}

AppSettings::~AppSettings() {
}

// TODO: Remove
Ref<Shortcut> ED_SHORTCUT_AND_COMMAND(const String &p_path, const String &p_name, Key p_keycode, String p_command) {
	return ED_SHORTCUT(p_path, p_name, p_keycode);
}
Ref<Shortcut> ED_SHORTCUT_ARRAY_AND_COMMAND(const String &p_path, const String &p_name, const PackedInt32Array &p_keycodes, String p_command) {
	return ED_SHORTCUT_ARRAY(p_path, p_name, p_keycodes);
}
