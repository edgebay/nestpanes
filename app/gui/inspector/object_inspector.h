#pragma once

#include "scene/gui/scroll_container.h"

#include "app/gui/inspector/property_name_processor.h"

class LineEdit;
class VBoxContainer;

//TODO: Remove
#define EditorPropertyNameProcessor PropertyNameProcessor

class ObjectInspector : public ScrollContainer {
	GDCLASS(ObjectInspector, ScrollContainer);

private:
	struct ThemeCache {
		int vertical_separation = 0;
		Color prop_subsection;
		Ref<Texture2D> icon_add;
	} theme_cache;

	VBoxContainer *base_vbox = nullptr;
	VBoxContainer *begin_vbox = nullptr;
	VBoxContainer *main_vbox = nullptr;

	// // Map used to cache the instantiated editors.
	// HashMap<StringName, List<EditorProperty *>> editor_property_map;
	// List<EditorInspectorSection *> sections;
	HashSet<StringName> pending;

	void _clear(bool p_hide_plugins = true);
	Object *object = nullptr;
	Object *next_object = nullptr;

	LineEdit *search_box = nullptr;
	// bool show_standard_categories = false;
	// bool show_custom_categories = false;
	// bool hide_script = true;
	// bool hide_metadata = true;
	// bool use_doc_hints = false;
	PropertyNameProcessor::Style property_name_style = PropertyNameProcessor::STYLE_CAPITALIZED;
	bool use_settings_name_style = true;
	bool use_filter = false;
	// bool autoclear = false;
	// bool use_folding = false;
	int changing;
	// bool update_all_pending = false;
	bool read_only = false;
	// bool keying = false;
	// bool wide_editors = false;
	// bool deletable_properties = false;
	bool mark_unsaved = true;

	float refresh_countdown;
	bool update_tree_pending = false;
	StringName _prop_edited;
	StringName property_selected;
	int property_focusable;
	int update_scroll_request;

	bool updating_theme = false;

	// HashMap<StringName, HashMap<StringName, DocCacheInfo>> doc_cache;
	HashSet<StringName> restart_request_props;
	HashMap<String, String> custom_property_descriptions;

	HashMap<ObjectID, int> scroll_cache;

	String property_prefix; // Used for sectioned inspector.
	String object_class;
	Variant property_clipboard;

	bool restrict_to_basic = false;

	HashMap<StringName, int> per_array_page;
	void _page_change_request(int p_new_page, const StringName &p_array_prefix);

	void _changed_callback();
	void _edit_request_change(Object *p_object, const String &p_prop);

	void _vscroll_changed(double p_offset);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void update_tree();
	void update_property(const String &p_prop);
	void edit(Object *p_object);
	Object *get_edited_object();
	Object *get_next_edited_object();

	// void set_keying(bool p_active);
	void set_read_only(bool p_read_only);
	void set_mark_unsaved(bool p_mark) { mark_unsaved = p_mark; }

	EditorPropertyNameProcessor::Style get_property_name_style() const;
	void set_property_name_style(EditorPropertyNameProcessor::Style p_style);

	// If true, the inspector will update its property name style according to the current editor settings.
	void set_use_settings_name_style(bool p_enable);

	void set_use_filter(bool p_use);
	void register_text_enter(Node *p_line_edit);

	void set_scroll_offset(int p_offset);
	int get_scroll_offset() const;

	void set_property_prefix(const String &p_prefix);
	String get_property_prefix() const;

	// void add_custom_property_description(const String &p_class, const String &p_property, const String &p_description);
	// String get_custom_property_description(const String &p_property) const;

	void set_object_class(const String &p_class);
	String get_object_class() const;

	void set_restrict_to_basic_settings(bool p_restrict);
	void set_property_clipboard(const Variant &p_value);
	Variant get_property_clipboard() const;

	ObjectInspector();
};
