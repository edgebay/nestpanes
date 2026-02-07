#pragma once

#include "app/gui/inspector/object_inspector.h"

class SectionedInspector;

class SettingsInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(SettingsInspectorPlugin, EditorInspectorPlugin);

	Object *current_object = nullptr;

public:
	SectionedInspector *inspector = nullptr;

	virtual bool can_handle(Object *p_object) override;
	virtual bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide = false) override;
};
