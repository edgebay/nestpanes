#include "app_settings.h"

#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/templates/rb_set.h"
#include "core/version.h"

#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

Ref<AppSettings> AppSettings::singleton = nullptr;

bool AppSettings::_set(const StringName &p_name, const Variant &p_value) {
	_THREAD_SAFE_METHOD_

	bool changed = _set_only(p_name, p_value);
	if (changed && initialized) {
		changed_settings.insert(p_name);
		emit_signal(SNAME("settings_changed"));
	}
	return true;
}

bool AppSettings::_set_only(const StringName &p_name, const Variant &p_value) {
	_THREAD_SAFE_METHOD_

	// if (p_name == "shortcuts") {
	// 	Array arr = p_value;
	// 	for (int i = 0; i < arr.size(); i++) {
	// 		Dictionary dict = arr[i];
	// 		String shortcut_name = dict["name"];

	// 		Array shortcut_events = dict["shortcuts"];

	// 		Ref<Shortcut> sc;
	// 		sc.instantiate();
	// 		sc->set_events(shortcut_events);
	// 		add_shortcut(shortcut_name, sc);
	// 	}

	// 	return false;
	// } else if (p_name == "builtin_action_overrides") {
	// 	Array actions_arr = p_value;
	// 	for (int i = 0; i < actions_arr.size(); i++) {
	// 		Dictionary action_dict = actions_arr[i];

	// 		String action_name = action_dict["name"];
	// 		Array events = action_dict["events"];

	// 		InputMap *im = InputMap::get_singleton();
	// 		im->action_erase_events(action_name);

	// 		builtin_action_overrides[action_name].clear();
	// 		for (int ev_idx = 0; ev_idx < events.size(); ev_idx++) {
	// 			im->action_add_event(action_name, events[ev_idx]);
	// 			builtin_action_overrides[action_name].push_back(events[ev_idx]);
	// 		}
	// 	}
	// 	return false;
	// }

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

	// if (p_name == "shortcuts") {
	// 	Array save_array;
	// 	const HashMap<String, List<Ref<InputEvent>>> &builtin_list = InputMap::get_singleton()->get_builtins();
	// 	for (const KeyValue<String, Ref<Shortcut>> &shortcut_definition : shortcuts) {
	// 		Ref<Shortcut> sc = shortcut_definition.value;

	// 		if (builtin_list.has(shortcut_definition.key)) {
	// 			// This shortcut was auto-generated from built in actions: don't save.
	// 			// If the builtin is overridden, it will be saved in the "builtin_action_overrides" section below.
	// 			continue;
	// 		}

	// 		Array shortcut_events = sc->get_events();

	// 		Dictionary dict;
	// 		dict["name"] = shortcut_definition.key;
	// 		dict["shortcuts"] = shortcut_events;

	// 		if (!sc->has_meta("original")) {
	// 			// Getting the meta when it doesn't exist will return an empty array. If the 'shortcut_events' have been cleared,
	// 			// we still want save the shortcut in this case so that shortcuts that the user has customized are not reset,
	// 			// even if the 'original' has not been populated yet. This can happen when calling save() from the Project Manager.
	// 			save_array.push_back(dict);
	// 			continue;
	// 		}

	// 		Array original_events = sc->get_meta("original");

	// 		bool is_same = Shortcut::is_event_array_equal(original_events, shortcut_events);
	// 		if (is_same) {
	// 			continue; // Not changed from default; don't save.
	// 		}

	// 		save_array.push_back(dict);
	// 	}
	// 	r_ret = save_array;
	// 	return true;
	// } else if (p_name == "builtin_action_overrides") {
	// 	Array actions_arr;
	// 	for (const KeyValue<String, List<Ref<InputEvent>>> &action_override : builtin_action_overrides) {
	// 		const List<Ref<InputEvent>> *defaults = InputMap::get_singleton()->get_builtins().getptr(action_override.key);
	// 		if (!defaults) {
	// 			continue;
	// 		}

	// 		List<Ref<InputEvent>> events = action_override.value;

	// 		Dictionary action_dict;
	// 		action_dict["name"] = action_override.key;

	// 		// Convert the list to an array, and only keep key events as this is for the app.
	// 		Array events_arr;
	// 		for (const Ref<InputEvent> &ie : events) {
	// 			Ref<InputEventKey> iek = ie;
	// 			if (iek.is_valid()) {
	// 				events_arr.append(iek);
	// 			}
	// 		}

	// 		Array defaults_arr;
	// 		for (const Ref<InputEvent> &default_input_event : *defaults) {
	// 			if (default_input_event.is_valid()) {
	// 				defaults_arr.append(default_input_event);
	// 			}
	// 		}

	// 		bool same = Shortcut::is_event_array_equal(events_arr, defaults_arr);

	// 		// Don't save if same as default.
	// 		if (same) {
	// 			continue;
	// 		}

	// 		action_dict["events"] = events_arr;
	// 		actions_arr.push_back(action_dict);
	// 	}

	// 	r_ret = actions_arr;
	// 	return true;
	// }

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

	// p_list->push_back(PropertyInfo(Variant::ARRAY, "shortcuts", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL)); //do not edit
	// p_list->push_back(PropertyInfo(Variant::ARRAY, "builtin_action_overrides", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
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

	// Theme
	APP_SETTING_BASIC(Variant::BOOL, PROPERTY_HINT_ENUM, "interface/theme/follow_system_theme", false, "")
	APP_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "interface/theme/preset", "Default", "Default,Breeze Dark,Godot 2,Gray,Light,Solarized (Dark),Solarized (Light),Black (OLED),Custom")
	APP_SETTING_BASIC(Variant::STRING, PROPERTY_HINT_ENUM, "interface/theme/spacing_preset", "Default", "Compact,Default,Spacious,Custom")
	APP_SETTING(Variant::INT, PROPERTY_HINT_ENUM, "interface/theme/icon_and_font_color", 0, "Auto,Dark,Light")
	APP_SETTING_BASIC(Variant::COLOR, PROPERTY_HINT_NONE, "interface/theme/base_color", Color(0.2, 0.23, 0.31), "")
	APP_SETTING_BASIC(Variant::COLOR, PROPERTY_HINT_NONE, "interface/theme/accent_color", Color(0.41, 0.61, 0.91), "")
	APP_SETTING_BASIC(Variant::BOOL, PROPERTY_HINT_NONE, "interface/theme/use_system_accent_color", false, "")
	APP_SETTING_BASIC(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/contrast", 0.3, "-1,1,0.01")
	APP_SETTING(Variant::BOOL, PROPERTY_HINT_NONE, "interface/theme/draw_extra_borders", false, "")
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/icon_saturation", 1.0, "0,2,0.01")
	APP_SETTING(Variant::FLOAT, PROPERTY_HINT_RANGE, "interface/theme/relationship_line_opacity", 0.1, "0.00,1,0.01")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/border_size", 0, "0,2,1")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/corner_radius", 3, "0,6,1")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/base_spacing", 4, "0,8,1")
	APP_SETTING(Variant::INT, PROPERTY_HINT_RANGE, "interface/theme/additional_spacing", 0, "0,8,1")
	APP_SETTING_USAGE(Variant::STRING, PROPERTY_HINT_GLOBAL_FILE, "interface/theme/custom_theme", "", "*.res,*.tres,*.theme", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_RESTART_IF_CHANGED)

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

AppSettings *AppSettings::get_singleton() {
	return singleton.ptr();
}

// String AppSettings::get_existing_settings_path() {
// 	const String config_dir = EditorPaths::get_singleton()->get_config_dir();
// 	int minor = APP_VERSION_MINOR;
// 	String filename;

// 	do {
// 		if (APP_VERSION_MAJOR == 4 && minor < 3) {
// 			// Minor version is used since 4.3, so special case to load older settings.
// 			filename = vformat("app_settings-%d.tres", APP_VERSION_MAJOR);
// 			minor = -1;
// 		} else {
// 			filename = vformat("app_settings-%d.%d.tres", APP_VERSION_MAJOR, minor);
// 			minor--;
// 		}
// 	} while (minor >= 0 && !FileAccess::exists(config_dir.path_join(filename)));
// 	return config_dir.path_join(filename);
// }

// String AppSettings::get_newest_settings_path() {
// 	const String config_file_name = vformat("app_settings-%d.%d.tres", APP_VERSION_MAJOR, APP_VERSION_MINOR);
// 	return EditorPaths::get_singleton()->get_config_dir().path_join(config_file_name);
// }

void AppSettings::create() {
	// IMPORTANT: create() *must* create a valid AppSettings singleton,
	// as the rest of the engine code will assume it. As such, it should never
	// return (incl. via ERR_FAIL) without initializing the singleton member.

	if (singleton.ptr()) {
		ERR_PRINT("Can't recreate AppSettings as it already exists.");
		return;
	}

	String config_file_path;
	Ref<ConfigFile> extra_config = memnew(ConfigFile);

	// 	if (!EditorPaths::get_singleton()) {
	// 		ERR_PRINT("Bug (please report): EditorPaths haven't been initialized, AppSettings cannot be created properly.");
	// 		goto fail;
	// 	}

	// 	if (EditorPaths::get_singleton()->is_self_contained()) {
	// 		Error err = extra_config->load(EditorPaths::get_singleton()->get_self_contained_file());
	// 		if (err != OK) {
	// 			ERR_PRINT("Can't load extra config from path: " + EditorPaths::get_singleton()->get_self_contained_file());
	// 		}
	// 	}

	// 	if (EditorPaths::get_singleton()->are_paths_valid()) {
	// 		// Validate config file.
	// 		ERR_FAIL_COND(!DirAccess::dir_exists_absolute(EditorPaths::get_singleton()->get_config_dir()));

	// 		config_file_path = get_existing_settings_path();
	// 		if (!FileAccess::exists(config_file_path)) {
	// 			config_file_path = get_newest_settings_path();
	// 			goto fail;
	// 		}

	// 		singleton = ResourceLoader::load(config_file_path, "AppSettings");
	// 		if (singleton.is_null()) {
	// 			ERR_PRINT("Could not load settings from path: " + config_file_path);
	// 			config_file_path = get_newest_settings_path();
	// 			goto fail;
	// 		}

	// 		singleton->set_path(get_newest_settings_path()); // Settings can be loaded from older version file, so make sure it's newest.
	// 		singleton->save_changed_setting = true;

	// 		print_verbose("AppSettings: Load OK!");

	// 		singleton->setup_language();
	// 		singleton->setup_network();
	// 		singleton->load_favorites_and_recent_dirs();
	// 		singleton->list_text_editor_themes();
	// #ifndef DISABLE_DEPRECATED
	// 		singleton->_remove_deprecated_settings();
	// #endif

	// 		return;
	// 	}

	// fail:
	// patch init projects
	String exe_path = OS::get_singleton()->get_executable_path().get_base_dir();

	if (extra_config->has_section("init_projects")) {
		Vector<String> list = extra_config->get_value("init_projects", "list");
		for (int i = 0; i < list.size(); i++) {
			list.write[i] = exe_path.path_join(list[i]);
		}
		extra_config->set_value("init_projects", "list", list);
	}

	singleton.instantiate();
	singleton->set_path(config_file_path, true);
	singleton->save_changed_setting = true;
	singleton->_load_defaults(extra_config);
	// singleton->setup_language();
	// singleton->setup_network();
	// singleton->list_text_editor_themes();
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

void AppSettings::set_setting(const String &p_setting, const Variant &p_value) {
	_THREAD_SAFE_METHOD_
	set(p_setting, p_value);
}

Variant AppSettings::get_setting(const String &p_setting) const {
	_THREAD_SAFE_METHOD_
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
	ERR_FAIL_COND_V(!AppSettings::get_singleton() || !AppSettings::get_singleton()->has_setting(p_setting), Variant());
	return AppSettings::get_singleton()->get(p_setting);
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

AppSettings::AppSettings() {
	last_order = 0;

	_load_defaults();
	callable_mp(this, &AppSettings::_set_initialized).call_deferred();
}

AppSettings::~AppSettings() {
}
