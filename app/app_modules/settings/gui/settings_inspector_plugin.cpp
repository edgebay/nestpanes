#include "settings_inspector_plugin.h"

#include "app/gui/inspector/sectioned_inspector.h"

#include "app/app_modules/settings/app_settings.h"

bool SettingsInspectorPlugin::can_handle(Object *p_object) {
	return p_object && p_object->is_class("SectionedInspectorFilter") && p_object != current_object;
}

bool SettingsInspectorPlugin::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (!p_object->is_class("SectionedInspectorFilter")) {
		return false;
	}

	const String property = inspector->get_full_item_path(p_path);
	if (!EditorSettings::get_singleton()->has_setting(property)) {
		return false;
	}
	current_object = p_object;

	EditorProperty *real_property = inspector->get_inspector()->instantiate_property_editor(p_object, p_type, p_path, p_hint, p_hint_text, p_usage, p_wide);
	if (!real_property) {
		return false;
	}
	real_property->set_object_and_property(p_object, p_path);
	real_property->set_name_split_ratio(0.0);

	add_property_editor(p_path, real_property);
	current_object = nullptr;
	return true;
}
