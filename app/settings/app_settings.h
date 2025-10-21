#pragma once

#include "core/io/config_file.h"
#include "core/io/resource.h"
#include "core/os/thread_safe.h"

class AppSettings : public Resource {
	GDCLASS(AppSettings, Resource);

	_THREAD_SAFE_CLASS_

private:
	struct VariantContainer {
		int order = 0;
		Variant variant;
		Variant initial;
		bool basic = false;
		bool has_default_value = false;
		bool hide_from_app = false;
		bool save = false;
		bool restart_if_changed = false;

		VariantContainer() {}

		VariantContainer(const Variant &p_variant, int p_order) :
				order(p_order),
				variant(p_variant) {
		}
	};

	static Ref<AppSettings> singleton;

	HashSet<String> changed_settings;

	mutable Ref<ConfigFile> project_metadata;
	HashMap<String, PropertyInfo> hints;
	HashMap<String, VariantContainer> props;
	int last_order;

	bool save_changed_setting = true;
	bool optimize_save = true; //do not save stuff that came from config but was not set from engine
	bool initialized = false;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _set_only(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _initial_set(const StringName &p_name, const Variant &p_value, bool p_basic = false);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _add_property_info_bind(const Dictionary &p_info);
	bool _property_can_revert(const StringName &p_name) const;
	bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

	void _set_initialized();
	void _load_defaults(Ref<ConfigFile> p_extra_config = Ref<ConfigFile>());

protected:
	static void _bind_methods();

public:
	enum {
		NOTIFICATION_APP_SETTINGS_CHANGED = 10000
	};

	static AppSettings *get_singleton();
	static String get_settings_path();

	static void create();
	static void save();
	static void destroy();
	void set_optimize_save(bool p_optimize);

	bool has_default_value(const String &p_setting) const;
	void set_setting(const String &p_setting, const Variant &p_value);
	Variant get_setting(const String &p_setting) const;
	bool has_setting(const String &p_setting) const;
	void erase(const String &p_setting);
	void raise_order(const String &p_setting);
	void set_initial_value(const StringName &p_setting, const Variant &p_value, bool p_update_current = false);
	void set_restart_if_changed(const StringName &p_setting, bool p_restart);
	void set_basic(const StringName &p_setting, bool p_basic);
	void set_manually(const StringName &p_setting, const Variant &p_value, bool p_emit_signal = false) {
		if (p_emit_signal) {
			_set(p_setting, p_value);
		} else {
			_set_only(p_setting, p_value);
		}
	}
	void add_property_hint(const PropertyInfo &p_hint);
	PackedStringArray get_changed_settings() const;
	bool check_changed_settings_in_group(const String &p_setting_prefix) const;
	void mark_setting_changed(const String &p_setting);

	void notify_changes();

	AppSettings();
	~AppSettings();
};

#define APP_SCALE 1.0 // TODO: EDSCALE, APP_SCALE move to AppSettings

//not a macro any longer

#define APP_DEF(m_var, m_val) _APP_DEF(m_var, Variant(m_val))
#define APP_DEF_RST(m_var, m_val) _APP_DEF(m_var, Variant(m_val), true)
#define APP_DEF_BASIC(m_var, m_val) _APP_DEF(m_var, Variant(m_val), false, true)
Variant _APP_DEF(const String &p_setting, const Variant &p_default, bool p_restart_if_changed = false, bool p_basic = false);

#define APP_GET(m_var) _APP_GET(m_var)
Variant _APP_GET(const String &p_setting);
