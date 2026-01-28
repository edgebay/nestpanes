#include "object_inspector.h"

#include "scene/gui/box_container.h"
#include "scene/gui/line_edit.h"

#include "app/app_string_names.h"
#include "app/gui/app_control.h"

// TODO: app/gui/* should not depend on app/app_modules/*
#include "app/app_modules/settings/app_settings.h"

// bool EditorInspector::_is_property_disabled_by_feature_profile(const StringName &p_property) {
// 	Ref<EditorFeatureProfile> profile = EditorFeatureProfileManager::get_singleton()->get_current_profile();
// 	if (profile.is_null()) {
// 		return false;
// 	}

// 	StringName class_name = object->get_class();

// 	while (class_name != StringName()) {
// 		if (profile->is_class_property_disabled(class_name, p_property)) {
// 			return true;
// 		}
// 		if (profile->is_class_disabled(class_name)) {
// 			//won't see properties of a disabled class
// 			return true;
// 		}
// 		class_name = ClassDB::get_parent_class(class_name);
// 	}

// 	return false;
// }

void ObjectInspector::update_tree() {
	if (!object) {
		return;
	}

	// Temporarily disable focus following on the root inspector to avoid jumping while the inspector is updating.
	set_follow_focus(false);

	// Store currently selected and focused elements to restore after the update.
	// TODO: Can be useful to store more context for the focusable, such as the caret position in LineEdit.
	StringName current_selected = property_selected;
	int current_focusable = -1;

	if (property_focusable != -1) {
		// Check that focusable is actually focusable.
		bool restore_focus = false;
		Control *focused = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
		if (focused) {
			Node *parent = focused->get_parent();
			while (parent) {
				ObjectInspector *inspector = Object::cast_to<ObjectInspector>(parent);
				if (inspector) {
					restore_focus = inspector == this; // May be owned by another inspector.
					break; // Exit after the first inspector is found, since there may be nested ones.
				}
				parent = parent->get_parent();
			}
		}

		if (restore_focus) {
			current_focusable = property_focusable;
		}
	}

	// The call here is for the edited object that has not changed, but the tree needs to be updated (for example, the object's property list has been modified).
	// Since the edited object has not changed, there is no need to hide the plugin at this time.
	_clear(false);

	// List<Ref<ObjectInspectorPlugin>> valid_plugins;

	// for (int i = inspector_plugin_count - 1; i >= 0; i--) { //start by last, so lastly added can override newly added
	// 	if (!inspector_plugins[i]->can_handle(object)) {
	// 		continue;
	// 	}
	// 	valid_plugins.push_back(inspector_plugins[i]);
	// }

	// // Decide if properties should be drawn with the warning color (yellow),
	// // or if the whole object should be considered read-only.
	// bool draw_warning = false;
	// bool all_read_only = false;
	// if (is_inside_tree()) {
	// 	if (object->has_method("_is_read_only")) {
	// 		all_read_only = object->call("_is_read_only");
	// 	}

	// 	// TODO
	// 	// Node *nod = Object::cast_to<Node>(object);
	// 	// Node *es = EditorNode::get_singleton()->get_edited_scene();
	// 	// if (nod && es != nod && nod->get_owner() != es) {
	// 	// 	// Draw in warning color edited nodes that are not in the currently edited scene,
	// 	// 	// as changes may be lost in the future.
	// 	// 	draw_warning = true;
	// 	// } else {
	// 	// 	if (!all_read_only) {
	// 	// 		Resource *res = Object::cast_to<Resource>(object);
	// 	// 		if (res) {
	// 	// 			all_read_only = EditorNode::get_singleton()->is_resource_read_only(res);
	// 	// 		}
	// 	// 	}
	// 	// }
	// }

	// String filter = search_box ? search_box->get_text() : "";
	// String group;
	// String group_base;
	// ObjectInspectorSection *group_togglable_property = nullptr;
	// String subgroup;
	// String subgroup_base;
	// ObjectInspectorSection *subgroup_togglable_property = nullptr;
	// int section_depth = 0;
	// bool disable_favorite = false;
	// VBoxContainer *category_vbox = nullptr;

	// List<PropertyInfo> plist;
	// object->get_property_list(&plist, true);

	// HashMap<VBoxContainer *, HashMap<String, VBoxContainer *>> vbox_per_path;
	// HashMap<String, ObjectInspectorArray *> editor_inspector_array_per_prefix;
	// HashMap<String, ObjectInspectorSection *> togglable_editor_inspector_sections;

	// const Color sscolor = theme_cache.prop_subsection;
	// bool sub_inspectors_enabled = EDITOR_GET("interface/inspector/open_resources_in_current_inspector");

	// if (!valid_plugins.is_empty()) {
	// 	begin_vbox->show();

	// 	// Get the lists of editors to add the beginning.
	// 	for (Ref<ObjectInspectorPlugin> &ped : valid_plugins) {
	// 		ped->parse_begin(object);
	// 		_parse_added_editors(begin_vbox, nullptr, ped);
	// 	}
	// }

	// StringName doc_name;

	// // Get the lists of editors for properties.
	// for (List<PropertyInfo>::Element *E_property = plist.front(); E_property; E_property = E_property->next()) {
	// 	PropertyInfo &p = E_property->get();

	// 	if (p.usage & PROPERTY_USAGE_SUBGROUP) {
	// 		// Setup a property sub-group.
	// 		subgroup = p.name;
	// 		subgroup_togglable_property = nullptr;

	// 		Vector<String> hint_parts = p.hint_string.split(",");
	// 		subgroup_base = hint_parts[0];
	// 		if (hint_parts.size() > 1) {
	// 			section_depth = hint_parts[1].to_int();
	// 		} else {
	// 			section_depth = 0;
	// 		}

	// 		continue;

	// 	} else if (p.usage & PROPERTY_USAGE_GROUP) {
	// 		// Setup a property group.
	// 		group = p.name;
	// 		group_togglable_property = nullptr;

	// 		Vector<String> hint_parts = p.hint_string.split(",");
	// 		group_base = hint_parts[0];
	// 		if (hint_parts.size() > 1) {
	// 			section_depth = hint_parts[1].to_int();
	// 		} else {
	// 			section_depth = 0;
	// 		}

	// 		subgroup = "";
	// 		subgroup_base = "";
	// 		subgroup_togglable_property = nullptr;

	// 		continue;

	// 	} else if (p.usage & PROPERTY_USAGE_CATEGORY) {
	// 		// Setup a property category.
	// 		group = "";
	// 		group_base = "";
	// 		group_togglable_property = nullptr;
	// 		subgroup = "";
	// 		subgroup_base = "";
	// 		subgroup_togglable_property = nullptr;
	// 		section_depth = 0;
	// 		disable_favorite = false;

	// 		vbox_per_path.clear();
	// 		editor_inspector_array_per_prefix.clear();

	// 		// `hint_script` should contain a native class name or a script path.
	// 		// Otherwise the category was probably added via `@export_category` or `_get_property_list()`.
	// 		const bool is_custom_category = p.hint_string.is_empty();

	// 		// Iterate over remaining properties. If no properties in category, skip the category.
	// 		List<PropertyInfo>::Element *N = E_property->next();
	// 		bool valid = true;
	// 		while (N) {
	// 			if (!N->get().name.begins_with("metadata/_") && N->get().usage & PROPERTY_USAGE_EDITOR &&
	// 					(!filter.is_empty() || !restrict_to_basic || (N->get().usage & PROPERTY_USAGE_EDITOR_BASIC_SETTING))) {
	// 				break;
	// 			}
	// 			// Treat custom categories as second-level ones. Do not skip a normal category if it is followed by a custom one.
	// 			// Skip in the other 3 cases (normal -> normal, custom -> custom, custom -> normal).
	// 			if ((N->get().usage & PROPERTY_USAGE_CATEGORY) && (is_custom_category || !N->get().hint_string.is_empty())) {
	// 				valid = false;
	// 				break;
	// 			}
	// 			N = N->next();
	// 		}
	// 		if (!valid) {
	// 			continue; // Empty, ignore it.
	// 		}

	// 		String category_tooltip;

	// 		// Do not add an icon, do not change the current class (`doc_name`) for custom categories.
	// 		if (is_custom_category) {
	// 			category_tooltip = p.name;
	// 		} else {
	// 			doc_name = p.name;

	// 			// Use category's owner script to update some of its information.
	// 			if (!EditorNode::get_editor_data().is_type_recognized(p.name) && ResourceLoader::exists(p.hint_string, "Script")) {
	// 				Ref<Script> scr = ResourceLoader::load(p.hint_string, "Script");
	// 				if (scr.is_valid()) {
	// 					doc_name = scr->get_doc_class_name();

	// 					// Property favorites aren't compatible with built-in scripts.
	// 					if (scr->is_built_in()) {
	// 						disable_favorite = true;
	// 					}
	// 				}
	// 			}

	// 			if (use_doc_hints) {
	// 				// `|` separators used in `EditorHelpBit`.
	// 				category_tooltip = "class|" + doc_name + "|";
	// 			}
	// 		}

	// 		if ((is_custom_category && !show_custom_categories) || (!is_custom_category && !show_standard_categories)) {
	// 			continue;
	// 		}

	// 		// Hide the "MultiNodeEdit" category for MultiNodeEdit.
	// 		if (Object::cast_to<MultiNodeEdit>(object) && p.name == "MultiNodeEdit") {
	// 			continue;
	// 		}

	// 		// Create an ObjectInspectorCategory and add it to the inspector.
	// 		ObjectInspectorCategory *category = memnew(ObjectInspectorCategory);
	// 		category->set_property_info(p);
	// 		main_vbox->add_child(category);
	// 		category_vbox = nullptr; // Reset.

	// 		// Set the category info.
	// 		category->set_tooltip_text(category_tooltip);
	// 		if (!is_custom_category) {
	// 			category->set_doc_class_name(doc_name);
	// 		}

	// 		// Add editors at the start of a category.
	// 		for (Ref<ObjectInspectorPlugin> &ped : valid_plugins) {
	// 			ped->parse_category(object, p.name);
	// 			_parse_added_editors(main_vbox, nullptr, ped);
	// 		}

	// 		continue;

	// 	} else if (p.name.begins_with("metadata/_") || !(p.usage & PROPERTY_USAGE_EDITOR) || _is_property_disabled_by_feature_profile(p.name) ||
	// 			(filter.is_empty() && restrict_to_basic && !(p.usage & PROPERTY_USAGE_EDITOR_BASIC_SETTING))) {
	// 		// Ignore properties that are not supposed to be in the inspector.
	// 		continue;
	// 	}

	// 	if (p.name == "script") {
	// 		// Script should go into its own category.
	// 		category_vbox = nullptr;
	// 	}

	// 	if (p.usage & PROPERTY_USAGE_HIGH_END_GFX && RS::get_singleton()->is_low_end()) {
	// 		// Do not show this property in low end gfx.
	// 		continue;
	// 	}

	// 	if (p.name == "script" && (hide_script || bool(object->call("_hide_script_from_inspector")))) {
	// 		// Hide script variables from inspector if required.
	// 		continue;
	// 	}

	// 	if (p.name.begins_with("metadata/") && bool(object->call(SNAME("_hide_metadata_from_inspector")))) {
	// 		// Hide metadata from inspector if required.
	// 		continue;
	// 	}

	// 	// Get the path for property.
	// 	String path = p.name;

	// 	// First check if we have an array that fits the prefix.
	// 	String array_prefix = "";
	// 	int array_index = -1;
	// 	for (KeyValue<String, ObjectInspectorArray *> &E : editor_inspector_array_per_prefix) {
	// 		if (p.name.begins_with(E.key) && E.key.length() > array_prefix.length()) {
	// 			array_prefix = E.key;
	// 		}
	// 	}

	// 	if (!array_prefix.is_empty()) {
	// 		// If we have an array element, find the according index in array.
	// 		String str = p.name.trim_prefix(array_prefix);
	// 		int to_char_index = 0;
	// 		while (to_char_index < str.length()) {
	// 			if (!is_digit(str[to_char_index])) {
	// 				break;
	// 			}
	// 			to_char_index++;
	// 		}
	// 		if (to_char_index > 0) {
	// 			array_index = str.left(to_char_index).to_int();
	// 		} else {
	// 			array_prefix = "";
	// 		}
	// 	}

	// 	// Don't allow to favorite array items.
	// 	if (!disable_favorite) {
	// 		disable_favorite = !array_prefix.is_empty();
	// 	}

	// 	if (!array_prefix.is_empty()) {
	// 		path = path.trim_prefix(array_prefix);
	// 		int char_index = path.find_char('/');
	// 		if (char_index >= 0) {
	// 			path = path.right(-char_index - 1);
	// 		} else {
	// 			path = vformat(TTR("Element %s"), array_index);
	// 		}
	// 	} else {
	// 		// Check if we exit or not a subgroup. If there is a prefix, remove it from the property label string.
	// 		if (!subgroup.is_empty() && !subgroup_base.is_empty()) {
	// 			if (path.begins_with(subgroup_base)) {
	// 				path = path.trim_prefix(subgroup_base);
	// 			} else if (subgroup_base.begins_with(path)) {
	// 				// Keep it, this is used pretty often.
	// 			} else {
	// 				subgroup = ""; // The prefix changed, we are no longer in the subgroup.
	// 				subgroup_togglable_property = nullptr;
	// 			}
	// 		}

	// 		// Check if we exit or not a group. If there is a prefix, remove it from the property label string.
	// 		if (!group.is_empty() && !group_base.is_empty() && subgroup.is_empty()) {
	// 			if (path.begins_with(group_base)) {
	// 				path = path.trim_prefix(group_base);
	// 			} else if (group_base.begins_with(path)) {
	// 				// Keep it, this is used pretty often.
	// 			} else {
	// 				group = ""; // The prefix changed, we are no longer in the group.
	// 				group_togglable_property = nullptr;
	// 				subgroup = "";
	// 				subgroup_togglable_property = nullptr;
	// 			}
	// 		}

	// 		// Add the group and subgroup to the path.
	// 		if (!subgroup.is_empty()) {
	// 			path = subgroup + "/" + path;
	// 		}
	// 		if (!group.is_empty()) {
	// 			path = group + "/" + path;
	// 		}
	// 	}

	// 	// Get the property label's string.
	// 	String name_override = (path.contains_char('/')) ? path.substr(path.rfind_char('/') + 1) : path;
	// 	String feature_tag;
	// 	{
	// 		const int dot = name_override.find_char('.');
	// 		if (dot != -1) {
	// 			feature_tag = name_override.substr(dot);
	// 			name_override = name_override.substr(0, dot);
	// 		}
	// 	}
	// 	name_override = name_override.uri_decode();

	// 	// Don't localize script variables.
	// 	PropertyNameProcessor::Style name_style = property_name_style;
	// 	if ((p.usage & PROPERTY_USAGE_SCRIPT_VARIABLE) && name_style == PropertyNameProcessor::STYLE_LOCALIZED) {
	// 		name_style = PropertyNameProcessor::STYLE_CAPITALIZED;
	// 	}
	// 	const String property_label_string = PropertyNameProcessor::get_singleton()->process_name(name_override, name_style, p.name, doc_name) + feature_tag;

	// 	// Remove the property from the path.
	// 	int idx = path.rfind_char('/');
	// 	if (idx > -1) {
	// 		path = path.left(idx);
	// 	} else {
	// 		path = "";
	// 	}

	// 	// Ignore properties that do not fit the filter.
	// 	bool sub_inspector_use_filter = false;
	// 	if (use_filter && !filter.is_empty()) {
	// 		const String property_path = property_prefix + (path.is_empty() ? "" : path + "/") + name_override;
	// 		if (!_property_path_matches(property_path, filter, property_name_style)) {
	// 			if (!sub_inspectors_enabled || p.hint != PROPERTY_HINT_RESOURCE_TYPE) {
	// 				continue;
	// 			}

	// 			Ref<Resource> res = object->get(p.name);
	// 			if (res.is_null()) {
	// 				continue;
	// 			}

	// 			// Check if the sub-resource has any properties that match the filter.
	// 			if (!_resource_properties_matches(res, filter)) {
	// 				continue;
	// 			}

	// 			sub_inspector_use_filter = true;
	// 		}
	// 	}

	// 	// Recreate the category vbox if it was reset.
	// 	if (category_vbox == nullptr) {
	// 		category_vbox = memnew(VBoxContainer);
	// 		category_vbox->add_theme_constant_override(SNAME("separation"), theme_cache.vertical_separation);
	// 		category_vbox->hide();
	// 		main_vbox->add_child(category_vbox);
	// 	}

	// 	// Find the correct section/vbox to add the property editor to.
	// 	VBoxContainer *root_vbox = array_prefix.is_empty() ? main_vbox : editor_inspector_array_per_prefix[array_prefix]->get_vbox(array_index);
	// 	if (!root_vbox) {
	// 		continue;
	// 	}

	// 	if (!vbox_per_path.has(root_vbox)) {
	// 		vbox_per_path[root_vbox] = HashMap<String, VBoxContainer *>();
	// 		vbox_per_path[root_vbox][""] = root_vbox;
	// 	}

	// 	VBoxContainer *current_vbox = root_vbox;
	// 	String acc_path = "";
	// 	int level = 1;

	// 	Vector<String> components = path.split("/");
	// 	for (int i = 0; i < components.size(); i++) {
	// 		const String &component = components[i];
	// 		acc_path += (i > 0) ? "/" + component : component;

	// 		if (!vbox_per_path[root_vbox].has(acc_path)) {
	// 			// If the section does not exists, create it.
	// 			ObjectInspectorSection *section = memnew(ObjectInspectorSection);
	// 			get_root_inspector()->get_v_scroll_bar()->connect(SceneStringName(value_changed), callable_mp(section, &ObjectInspectorSection::reset_timer).unbind(1));
	// 			current_vbox->add_child(section);
	// 			sections.push_back(section);

	// 			String label;
	// 			String tooltip;

	// 			// Don't localize groups for script variables.
	// 			PropertyNameProcessor::Style section_name_style = property_name_style;
	// 			if ((p.usage & PROPERTY_USAGE_SCRIPT_VARIABLE) && section_name_style == PropertyNameProcessor::STYLE_LOCALIZED) {
	// 				section_name_style = PropertyNameProcessor::STYLE_CAPITALIZED;
	// 			}

	// 			// Only process group label if this is not the group or subgroup.
	// 			if ((i == 0 && component == group) || (i == 1 && component == subgroup)) {
	// 				if (section_name_style == PropertyNameProcessor::STYLE_LOCALIZED) {
	// 					label = PropertyNameProcessor::get_singleton()->translate_group_name(component);
	// 					tooltip = component;
	// 				} else {
	// 					label = component;
	// 					tooltip = PropertyNameProcessor::get_singleton()->translate_group_name(component);
	// 				}
	// 			} else {
	// 				label = PropertyNameProcessor::get_singleton()->process_name(component, section_name_style, p.name, doc_name);
	// 				tooltip = PropertyNameProcessor::get_singleton()->process_name(component, PropertyNameProcessor::get_tooltip_style(section_name_style), p.name, doc_name);
	// 			}

	// 			Color c = sscolor;
	// 			c.a /= level;
	// 			section->setup(acc_path, label, object, c, use_folding, section_depth, level);
	// 			section->set_tooltip_text(tooltip);

	// 			section->connect("section_toggled_by_user", callable_mp(this, &ObjectInspector::_section_toggled_by_user));
	// 			section->connect("property_keyed", callable_mp(this, &ObjectInspector::_property_keyed));

	// 			// Add editors at the start of a group.
	// 			for (Ref<ObjectInspectorPlugin> &ped : valid_plugins) {
	// 				ped->parse_group(object, path);
	// 				_parse_added_editors(section->get_vbox(), section, ped);
	// 			}

	// 			vbox_per_path[root_vbox][acc_path] = section->get_vbox();
	// 		}

	// 		current_vbox = vbox_per_path[root_vbox][acc_path];
	// 		level = (MIN(level + 1, 4));
	// 	}

	// 	// If we did not find a section to add the property to, add it to the category vbox instead (the category vbox handles margins correctly).
	// 	if (current_vbox == main_vbox) {
	// 		category_vbox->show();
	// 		current_vbox = category_vbox;
	// 	}

	// 	// Check if the property is an array counter, if so create a dedicated array editor for the array.
	// 	if (p.usage & PROPERTY_USAGE_ARRAY) {
	// 		ObjectInspectorArray *editor_inspector_array = nullptr;
	// 		StringName array_element_prefix;
	// 		Color c = sscolor;
	// 		c.a /= level;

	// 		Vector<String> class_name_components = String(p.class_name).split(",");

	// 		int page_size = 5;
	// 		bool movable = true;
	// 		bool is_const = false;
	// 		bool numbered = false;
	// 		bool foldable = use_folding;
	// 		String add_button_text = TTRC("Add Element");
	// 		String swap_method;
	// 		for (int i = (p.type == Variant::NIL ? 1 : 2); i < class_name_components.size(); i++) {
	// 			if (class_name_components[i].begins_with("page_size") && class_name_components[i].get_slice_count("=") == 2) {
	// 				page_size = class_name_components[i].get_slicec('=', 1).to_int();
	// 			} else if (class_name_components[i].begins_with("add_button_text") && class_name_components[i].get_slice_count("=") == 2) {
	// 				add_button_text = class_name_components[i].get_slicec('=', 1).strip_edges();
	// 			} else if (class_name_components[i] == "static") {
	// 				movable = false;
	// 			} else if (class_name_components[i] == "const") {
	// 				is_const = true;
	// 			} else if (class_name_components[i] == "numbered") {
	// 				numbered = true;
	// 			} else if (class_name_components[i] == "unfoldable") {
	// 				foldable = false;
	// 			} else if (class_name_components[i].begins_with("swap_method") && class_name_components[i].get_slice_count("=") == 2) {
	// 				swap_method = class_name_components[i].get_slicec('=', 1).strip_edges();
	// 			}
	// 		}

	// 		if (p.type == Variant::NIL) {
	// 			// Setup the array to use a method to create/move/delete elements.
	// 			array_element_prefix = class_name_components[0];
	// 			editor_inspector_array = memnew(ObjectInspectorArray(all_read_only));

	// 			String array_label = path.contains_char('/') ? path.substr(path.rfind_char('/') + 1) : path;
	// 			array_label = PropertyNameProcessor::get_singleton()->process_name(property_label_string, property_name_style, p.name, doc_name);
	// 			int page = per_array_page.has(array_element_prefix) ? per_array_page[array_element_prefix] : 0;
	// 			editor_inspector_array->setup_with_move_element_function(object, array_label, array_element_prefix, page, c, use_folding);
	// 			editor_inspector_array->connect("page_change_request", callable_mp(this, &ObjectInspector::_page_change_request).bind(array_element_prefix));
	// 		} else if (p.type == Variant::INT) {
	// 			// Setup the array to use the count property and built-in functions to create/move/delete elements.
	// 			if (class_name_components.size() >= 2) {
	// 				array_element_prefix = class_name_components[1];
	// 				editor_inspector_array = memnew(ObjectInspectorArray(all_read_only));
	// 				int page = per_array_page.has(array_element_prefix) ? per_array_page[array_element_prefix] : 0;

	// 				editor_inspector_array->setup_with_count_property(object, class_name_components[0], p.name, array_element_prefix, page, c, foldable, movable, is_const, numbered, page_size, add_button_text, swap_method);
	// 				editor_inspector_array->connect("page_change_request", callable_mp(this, &ObjectInspector::_page_change_request).bind(array_element_prefix));
	// 			}
	// 		}

	// 		if (editor_inspector_array) {
	// 			current_vbox->add_child(editor_inspector_array);
	// 			editor_inspector_array_per_prefix[array_element_prefix] = editor_inspector_array;
	// 		}

	// 		continue;
	// 	}

	// 	// Checkable and checked properties.
	// 	bool checkable = false;
	// 	bool checked = false;
	// 	if (p.usage & PROPERTY_USAGE_CHECKABLE) {
	// 		checkable = true;
	// 		checked = p.usage & PROPERTY_USAGE_CHECKED;
	// 	}

	// 	bool property_read_only = (p.usage & PROPERTY_USAGE_READ_ONLY) || read_only;

	// 	// Mark properties that would require an editor restart (mostly when editing editor settings).
	// 	if (p.usage & PROPERTY_USAGE_RESTART_IF_CHANGED) {
	// 		restart_request_props.insert(p.name);
	// 	}

	// 	String doc_path;
	// 	String theme_item_name;
	// 	String doc_tooltip_text;
	// 	StringName classname = doc_name;

	// 	// Build the doc hint, to use as tooltip.
	// 	if (use_doc_hints) {
	// 		if (!object_class.is_empty()) {
	// 			classname = object_class;
	// 		} else if (Object::cast_to<MultiNodeEdit>(object)) {
	// 			classname = Object::cast_to<MultiNodeEdit>(object)->get_edited_class_name();
	// 		} else if (classname == "") {
	// 			classname = object->get_class_name();
	// 			Resource *res = Object::cast_to<Resource>(object);
	// 			if (res && !res->get_script().is_null()) {
	// 				// Grab the script of this resource to get the evaluated script class.
	// 				Ref<Script> scr = res->get_script();
	// 				if (scr.is_valid()) {
	// 					Vector<DocData::ClassDoc> docs = scr->get_documentation();
	// 					if (!docs.is_empty()) {
	// 						// The documentation of a GDScript's main class is at the end of the array.
	// 						// Hacky because this isn't necessarily always guaranteed.
	// 						classname = docs[docs.size() - 1].name;
	// 					}
	// 				}
	// 			}
	// 		}

	// 		StringName propname = property_prefix + p.name;
	// 		bool found = false;

	// 		// Small hack for theme_overrides. They are listed under Control, but come from another class.
	// 		if (classname == "Control" && p.name.begins_with("theme_override_")) {
	// 			classname = get_edited_object()->get_class();
	// 		}

	// 		// Search for the doc path in the cache.
	// 		HashMap<StringName, HashMap<StringName, DocCacheInfo>>::Iterator E = doc_cache.find(classname);
	// 		if (E) {
	// 			HashMap<StringName, DocCacheInfo>::Iterator F = E->value.find(propname);
	// 			if (F) {
	// 				found = true;
	// 				doc_path = F->value.doc_path;
	// 				theme_item_name = F->value.theme_item_name;
	// 			}
	// 		}

	// 		if (!found) {
	// 			DocTools *dd = EditorHelp::get_doc_data();
	// 			// Do not cache the doc path information of scripts.
	// 			bool is_native_class = ClassDB::class_exists(classname);

	// 			HashMap<String, DocData::ClassDoc>::ConstIterator F = dd->class_list.find(classname);
	// 			while (F) {
	// 				Vector<String> slices = propname.operator String().split("/");
	// 				// Check if it's a theme item first.
	// 				if (slices.size() == 2 && slices[0].begins_with("theme_override_")) {
	// 					for (int i = 0; i < F->value.theme_properties.size(); i++) {
	// 						String doc_path_current = "class_theme_item:" + F->value.name + ":" + F->value.theme_properties[i].name;
	// 						if (F->value.theme_properties[i].name == slices[1]) {
	// 							doc_path = doc_path_current;
	// 							theme_item_name = F->value.theme_properties[i].name;
	// 						}
	// 					}
	// 				} else {
	// 					for (int i = 0; i < F->value.properties.size(); i++) {
	// 						String doc_path_current = "class_property:" + F->value.name + ":" + F->value.properties[i].name;
	// 						if (F->value.properties[i].name == propname.operator String()) {
	// 							doc_path = doc_path_current;
	// 						}
	// 					}
	// 				}

	// 				if (is_native_class) {
	// 					DocCacheInfo cache_info;
	// 					cache_info.doc_path = doc_path;
	// 					cache_info.theme_item_name = theme_item_name;
	// 					doc_cache[classname][propname] = cache_info;
	// 				}

	// 				if (!doc_path.is_empty() || F->value.inherits.is_empty()) {
	// 					break;
	// 				}
	// 				// Couldn't find the doc path in the class itself, try its super class.
	// 				F = dd->class_list.find(F->value.inherits);
	// 			}
	// 		}

	// 		// `|` separators used in `EditorHelpBit`.
	// 		if (theme_item_name.is_empty()) {
	// 			if (p.name.contains("shader_parameter/")) {
	// 				ShaderMaterial *shader_material = Object::cast_to<ShaderMaterial>(object);
	// 				if (shader_material) {
	// 					doc_tooltip_text = "property|" + shader_material->get_shader()->get_path() + "|" + property_prefix + p.name;
	// 				}
	// 			} else if (p.usage & PROPERTY_USAGE_INTERNAL) {
	// 				doc_tooltip_text = "internal_property|" + classname + "|" + property_prefix + p.name;
	// 			} else {
	// 				doc_tooltip_text = "property|" + classname + "|" + property_prefix + p.name;
	// 			}
	// 		} else {
	// 			doc_tooltip_text = "theme_item|" + classname + "|" + theme_item_name;
	// 		}
	// 	}

	// 	// Only used for boolean types. Makes the section header a checkable group and adds tooltips.
	// 	if (p.hint == PROPERTY_HINT_GROUP_ENABLE) {
	// 		if (p.type == Variant::BOOL && (p.name.begins_with(group_base) || p.name.begins_with(subgroup_base))) {
	// 			ObjectInspectorSection *last_created_section = Object::cast_to<ObjectInspectorSection>(current_vbox->get_parent());
	// 			if (last_created_section) {
	// 				bool valid = false;
	// 				Variant value_checked = object->get(p.name, &valid);

	// 				if (valid) {
	// 					last_created_section->set_checkable(p.name, p.hint_string == "checkbox_only", value_checked.operator bool());
	// 					last_created_section->set_keying(keying);

	// 					if (p.name.begins_with(group_base)) {
	// 						group_togglable_property = last_created_section;
	// 					} else {
	// 						subgroup_togglable_property = last_created_section;
	// 					}

	// 					if (use_doc_hints) {
	// 						last_created_section->set_tooltip_text(doc_tooltip_text);
	// 					}
	// 					continue;
	// 				}
	// 			}
	// 		} else {
	// 			ERR_PRINT("PROPERTY_HINT_GROUP_ENABLE can only be used on boolean types and must have the same prefix as the group.");
	// 		}
	// 	}

	// 	Vector<ObjectInspectorPlugin::AddedEditor> editors;
	// 	Vector<ObjectInspectorPlugin::AddedEditor> late_editors;

	// 	// Search for the inspector plugin that will handle the properties. Then add the correct property editor to it.
	// 	for (Ref<ObjectInspectorPlugin> &ped : valid_plugins) {
	// 		bool exclusive = ped->parse_property(object, p.type, p.name, p.hint, p.hint_string, p.usage, wide_editors);

	// 		for (const ObjectInspectorPlugin::AddedEditor &F : ped->added_editors) {
	// 			if (F.add_to_end) {
	// 				late_editors.push_back(F);
	// 			} else {
	// 				editors.push_back(F);
	// 			}
	// 		}

	// 		ped->added_editors.clear();

	// 		if (exclusive) {
	// 			break;
	// 		}
	// 	}

	// 	editors.append_array(late_editors);

	// 	const Node *node = Object::cast_to<Node>(object);

	// 	Vector<SceneState::PackState> sstack;
	// 	if (node != nullptr) {
	// 		const Node *es = EditorNode::get_singleton()->get_edited_scene();
	// 		sstack = PropertyUtils::get_node_states_stack(node, es);
	// 	}

	// 	for (int i = 0; i < editors.size(); i++) {
	// 		EditorProperty *ep = Object::cast_to<EditorProperty>(editors[i].property_editor);
	// 		const Vector<String> &properties = editors[i].properties;

	// 		if (ep) {
	// 			// Set all this before the control gets the ENTER_TREE notification.
	// 			ep->object = object;

	// 			if (properties.size()) {
	// 				if (properties.size() == 1) {
	// 					// Since it's one, associate:
	// 					ep->property = properties[0];
	// 					ep->property_path = property_prefix + properties[0];
	// 					ep->property_usage = p.usage;
	// 					// And set label?
	// 				}
	// 				if (!editors[i].label.is_empty()) {
	// 					ep->set_label(editors[i].label);
	// 				} else {
	// 					// Use the existing one.
	// 					ep->set_label(property_label_string);
	// 				}

	// 				for (int j = 0; j < properties.size(); j++) {
	// 					String prop = properties[j];

	// 					if (!editor_property_map.has(prop)) {
	// 						editor_property_map[prop] = List<EditorProperty *>();
	// 					}
	// 					editor_property_map[prop].push_back(ep);
	// 				}
	// 			}

	// 			if (sub_inspector_use_filter) {
	// 				EditorPropertyResource *epr = Object::cast_to<EditorPropertyResource>(ep);
	// 				if (epr) {
	// 					epr->set_use_filter(true);
	// 				}
	// 			}

	// 			if (p.name.begins_with("metadata/")) {
	// 				Variant _default = Variant();
	// 				if (node != nullptr) {
	// 					_default = PropertyUtils::get_property_default_value(node, p.name, nullptr, &sstack, false, nullptr, nullptr);
	// 				}
	// 				ep->set_deletable(_default == Variant());
	// 			} else {
	// 				ep->set_deletable(deletable_properties);
	// 			}

	// 			ep->set_draw_warning(draw_warning);
	// 			ep->set_use_folding(use_folding);
	// 			ep->set_favoritable(can_favorite && !disable_favorite && !ep->is_deletable());
	// 			ep->set_checkable(checkable);
	// 			ep->set_checked(checked);
	// 			ep->set_keying(keying);
	// 			ep->set_read_only(property_read_only || all_read_only);
	// 		}

	// 		{
	// 			current_vbox->add_child(editors[i].property_editor);

	// 			if (ep) {
	// 				Node *section_search = current_vbox->get_parent();
	// 				while (section_search) {
	// 					ObjectInspectorSection *section = Object::cast_to<ObjectInspectorSection>(section_search);
	// 					if (section) {
	// 						ep->connect("property_can_revert_changed", callable_mp(section, &ObjectInspectorSection::property_can_revert_changed));
	// 					}
	// 					section_search = section_search->get_parent();
	// 					if (Object::cast_to<ObjectInspector>(section_search)) {
	// 						// Skip sub-resource inspectors.
	// 						break;
	// 					}
	// 				}
	// 			}
	// 		}

	// 		if (ep) {
	// 			// Eventually, set other properties/signals after the property editor got added to the tree.
	// 			bool update_all = (p.usage & PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED);
	// 			ep->connect("property_changed", callable_mp(this, &ObjectInspector::_property_changed).bind(update_all));
	// 			ep->connect("property_keyed", callable_mp(this, &ObjectInspector::_property_keyed));
	// 			ep->connect("property_deleted", callable_mp(this, &ObjectInspector::_property_deleted), CONNECT_DEFERRED);
	// 			ep->connect("property_keyed_with_value", callable_mp(this, &ObjectInspector::_property_keyed_with_value));
	// 			ep->connect("property_favorited", callable_mp(this, &ObjectInspector::_set_property_favorited), CONNECT_DEFERRED);
	// 			ep->connect("property_checked", callable_mp(this, &ObjectInspector::_property_checked));
	// 			ep->connect("property_pinned", callable_mp(this, &ObjectInspector::_property_pinned));
	// 			ep->connect("selected", callable_mp(this, &ObjectInspector::_property_selected));
	// 			ep->connect("multiple_properties_changed", callable_mp(this, &ObjectInspector::_multiple_properties_changed));
	// 			ep->connect("resource_selected", callable_mp(get_root_inspector(), &ObjectInspector::_resource_selected), CONNECT_DEFERRED);
	// 			ep->connect("object_id_selected", callable_mp(this, &ObjectInspector::_object_id_selected), CONNECT_DEFERRED);

	// 			ep->set_tooltip_text(doc_tooltip_text);
	// 			ep->has_doc_tooltip = use_doc_hints;
	// 			ep->set_doc_path(doc_path);
	// 			ep->set_internal(p.usage & PROPERTY_USAGE_INTERNAL);

	// 			// If this property is favorited, it won't be in the tree yet. So don't do this setup right now.
	// 			if (ep->is_inside_tree()) {
	// 				ep->update_property();
	// 				ep->_update_flags();
	// 				ep->update_editor_property_status();
	// 				ep->update_cache();

	// 				if (current_selected && ep->property == current_selected) {
	// 					ep->select(current_focusable);
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// // Hide metadata by default.

	// // Get the lists of to add at the end.
	// for (Ref<ObjectInspectorPlugin> &ped : valid_plugins) {
	// 	ped->parse_end(object);
	// 	_parse_added_editors(main_vbox, nullptr, ped);
	// }

	set_follow_focus(true);
}

void ObjectInspector::update_property(const String &p_prop) {
	// if (!editor_property_map.has(p_prop)) {
	// 	return;
	// }

	// for (EditorProperty *E : editor_property_map[p_prop]) {
	// 	E->update_property();
	// 	E->update_editor_property_status();
	// 	E->update_cache();
	// }

	// for (ObjectInspectorSection *S : sections) {
	// 	if (S->is_checkable()) {
	// 		S->_property_edited(p_prop);
	// 	}
	// }
}

void ObjectInspector::_clear(bool p_hide_plugins) {
	begin_vbox->hide();
	while (begin_vbox->get_child_count()) {
		memdelete(begin_vbox->get_child(0));
	}

	while (main_vbox->get_child_count()) {
		memdelete(main_vbox->get_child(0));
	}

	property_selected = StringName();
	property_focusable = -1;
	// TODO
	// editor_property_map.clear();
	// sections.clear();
	pending.clear();
	restart_request_props.clear();

	// if (p_hide_plugins && is_main_editor_inspector()) {
	// 	EditorNode::get_singleton()->hide_unused_editors(this);
	// }
}

Object *ObjectInspector::get_edited_object() {
	return object;
}

Object *ObjectInspector::get_next_edited_object() {
	return next_object;
}

void ObjectInspector::edit(Object *p_object) {
	if (object == p_object) {
		return;
	}

	next_object = p_object; // Some plugins need to know the next edited object when clearing the inspector.
	if (object) {
		if (likely(Variant(object).get_validated_object())) {
			object->disconnect(CoreStringName(property_list_changed), callable_mp(this, &ObjectInspector::_changed_callback));
		}
		_clear();
	}
	per_array_page.clear();

	object = p_object;

	if (object) {
		update_scroll_request = 0; //reset
		if (scroll_cache.has(object->get_instance_id())) { //if exists, set something else
			update_scroll_request = scroll_cache[object->get_instance_id()]; //done this way because wait until full size is accommodated
		}
		object->connect(CoreStringName(property_list_changed), callable_mp(this, &ObjectInspector::_changed_callback));

		update_tree();
	}

	// Keep it available until the end so it works with both main and sub inspectors.
	next_object = nullptr;

	emit_signal(SNAME("edited_object_changed"));
}

void ObjectInspector::set_read_only(bool p_read_only) {
	if (p_read_only == read_only) {
		return;
	}
	read_only = p_read_only;
	update_tree();
}

EditorPropertyNameProcessor::Style ObjectInspector::get_property_name_style() const {
	return property_name_style;
}

void ObjectInspector::set_property_name_style(EditorPropertyNameProcessor::Style p_style) {
	if (property_name_style == p_style) {
		return;
	}
	property_name_style = p_style;
	update_tree();
}

void ObjectInspector::set_use_settings_name_style(bool p_enable) {
	if (use_settings_name_style == p_enable) {
		return;
	}
	use_settings_name_style = p_enable;
	if (use_settings_name_style) {
		set_property_name_style(EditorPropertyNameProcessor::get_singleton()->get_settings_style());
	}
}

void ObjectInspector::set_use_filter(bool p_use) {
	use_filter = p_use;
	update_tree();
}

void ObjectInspector::register_text_enter(Node *p_line_edit) {
	search_box = Object::cast_to<LineEdit>(p_line_edit);
	if (search_box) {
		search_box->connect(SceneStringName(text_changed), callable_mp(this, &ObjectInspector::update_tree).unbind(1));
	}
}

void ObjectInspector::set_scroll_offset(int p_offset) {
	// This can be called before the container finishes sorting its children, so defer it.
	callable_mp((ScrollContainer *)this, &ScrollContainer::set_v_scroll).call_deferred(p_offset);
}

void ObjectInspector::_page_change_request(int p_new_page, const StringName &p_array_prefix) {
	int prev_page = per_array_page.has(p_array_prefix) ? per_array_page[p_array_prefix] : 0;
	int new_page = MAX(0, p_new_page);
	if (new_page != prev_page) {
		per_array_page[p_array_prefix] = new_page;
		update_tree_pending = true;
	}
}

void ObjectInspector::_edit_request_change(Object *p_object, const String &p_property) {
	if (object != p_object) { //may be undoing/redoing for a non edited object, so ignore
		return;
	}

	if (changing) {
		return;
	}

	if (p_property.is_empty()) {
		update_tree_pending = true;
	} else {
		pending.insert(p_property);
	}
}

int ObjectInspector::get_scroll_offset() const {
	return get_v_scroll();
}

void ObjectInspector::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (property_name_style == PropertyNameProcessor::STYLE_LOCALIZED) {
				update_tree_pending = true;
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			if (updating_theme) {
				break;
			}

			theme_cache.vertical_separation = get_theme_constant(SNAME("v_separation"), SNAME("ObjectInspector"));
			theme_cache.prop_subsection = get_theme_color(SNAME("prop_subsection"), AppStringName(App));
			theme_cache.icon_add = get_app_theme_icon(SNAME("Add"));

			// TODO
			// initialize_section_theme(section_theme_cache, this);
			// initialize_category_theme(category_theme_cache, this);

			base_vbox->add_theme_constant_override("separation", theme_cache.vertical_separation);
			begin_vbox->add_theme_constant_override("separation", theme_cache.vertical_separation);
			main_vbox->add_theme_constant_override("separation", theme_cache.vertical_separation);
		} break;

		case NOTIFICATION_READY: {
			set_process(is_visible_in_tree());
			// TODO
			// if (!is_sub_inspector()) {
			// 	get_tree()->connect("node_removed", callable_mp(this, &ObjectInspector::_node_removed));
			// }
		} break;

		case NOTIFICATION_PREDELETE: {
			// if (!is_sub_inspector() && is_inside_tree()) {
			// 	get_tree()->disconnect("node_removed", callable_mp(this, &ObjectInspector::_node_removed));
			// }
			edit(nullptr);
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process(is_visible_in_tree());
		} break;

		case NOTIFICATION_PROCESS: {
			// 	if (update_scroll_request >= 0) {
			// 		callable_mp((Range *)get_v_scroll_bar(), &Range::set_value).call_deferred(update_scroll_request);
			// 		update_scroll_request = -1;
			// 	}
			// 	if (update_tree_pending) {
			// 		refresh_countdown = float(EDITOR_GET("docks/property_editor/auto_refresh_interval"));
			// 	} else if (refresh_countdown > 0) {
			// 		refresh_countdown -= get_process_delta_time();
			// 		if (refresh_countdown <= 0) {
			// 			for (const KeyValue<StringName, List<EditorProperty *>> &F : editor_property_map) {
			// 				for (EditorProperty *E : F.value) {
			// 					if (E && !E->is_cache_valid()) {
			// 						E->update_property();
			// 						E->update_editor_property_status();
			// 						E->update_cache();
			// 					}
			// 				}
			// 			}

			// 			for (ObjectInspectorSection *S : sections) {
			// 				S->update_property();
			// 			}

			// 			refresh_countdown = float(EDITOR_GET("docks/property_editor/auto_refresh_interval"));
			// 		}
			// 	}

			// 	changing++;

			// 	if (update_tree_pending) {
			// 		update_tree();
			// 		update_tree_pending = false;
			// 		pending.clear();

			// 	} else {
			// 		while (pending.size()) {
			// 			StringName prop = *pending.begin();
			// 			if (editor_property_map.has(prop)) {
			// 				for (EditorProperty *E : editor_property_map[prop]) {
			// 					E->update_property();
			// 					E->update_editor_property_status();
			// 					E->update_cache();
			// 				}
			// 			}
			// 			pending.remove(pending.begin());
			// 		}

			// 		for (ObjectInspectorSection *S : sections) {
			// 			S->update_property();
			// 		}
			// 	}

			// 	changing--;
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (use_settings_name_style && EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/localize_settings")) {
				PropertyNameProcessor::Style style = PropertyNameProcessor::get_settings_style();
				if (property_name_style != style) {
					property_name_style = style;
					update_tree_pending = true;
				}
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/inspector")) {
				update_tree_pending = true;
			}
		} break;
	}
}

void ObjectInspector::_changed_callback() {
	//this is called when property change is notified via notify_property_list_changed()
	if (object != nullptr) {
		_edit_request_change(object, String());
	}
}

void ObjectInspector::_vscroll_changed(double p_offset) {
	if (update_scroll_request >= 0) { //waiting, do nothing
		return;
	}

	if (object) {
		scroll_cache[object->get_instance_id()] = p_offset; // TODO: clear cache
	}
}

void ObjectInspector::set_property_prefix(const String &p_prefix) {
	property_prefix = p_prefix;
}

String ObjectInspector::get_property_prefix() const {
	return property_prefix;
}

// void ObjectInspector::add_custom_property_description(const String &p_class, const String &p_property, const String &p_description) {
// 	const String key = vformat("property|%s|%s", p_class, p_property);
// 	custom_property_descriptions[key] = p_description;
// }

// String ObjectInspector::get_custom_property_description(const String &p_property) const {
// 	HashMap<String, String>::ConstIterator E = custom_property_descriptions.find(p_property);
// 	if (E) {
// 		return E->value;
// 	}
// 	return "";
// }

void ObjectInspector::set_object_class(const String &p_class) {
	object_class = p_class;
}

String ObjectInspector::get_object_class() const {
	return object_class;
}

void ObjectInspector::set_restrict_to_basic_settings(bool p_restrict) {
	restrict_to_basic = p_restrict;
	update_tree();
}

void ObjectInspector::set_property_clipboard(const Variant &p_value) {
	property_clipboard = p_value;
}

Variant ObjectInspector::get_property_clipboard() const {
	return property_clipboard;
}

void ObjectInspector::_bind_methods() {
	// ClassDB::bind_method(D_METHOD("edit", "object"), &ObjectInspector::edit);
	// ClassDB::bind_method("_edit_request_change", &ObjectInspector::_edit_request_change);
	// ClassDB::bind_method("get_selected_path", &ObjectInspector::get_selected_path);
	// ClassDB::bind_method("get_edited_object", &ObjectInspector::get_edited_object);

	// ClassDB::bind_static_method("ObjectInspector", D_METHOD("instantiate_property_editor", "object", "type", "path", "hint", "hint_text", "usage", "wide"), &ObjectInspector::instantiate_property_editor, DEFVAL(false));

	// ADD_SIGNAL(MethodInfo("property_selected", PropertyInfo(Variant::STRING, "property")));
	// ADD_SIGNAL(MethodInfo("property_keyed", PropertyInfo(Variant::STRING, "property"), PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT), PropertyInfo(Variant::BOOL, "advance")));
	// ADD_SIGNAL(MethodInfo("property_deleted", PropertyInfo(Variant::STRING, "property")));
	// ADD_SIGNAL(MethodInfo("resource_selected", PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), PropertyInfo(Variant::STRING, "path")));
	// ADD_SIGNAL(MethodInfo("object_id_selected", PropertyInfo(Variant::INT, "id")));
	// ADD_SIGNAL(MethodInfo("property_edited", PropertyInfo(Variant::STRING, "property")));
	// ADD_SIGNAL(MethodInfo("property_toggled", PropertyInfo(Variant::STRING, "property"), PropertyInfo(Variant::BOOL, "checked")));
	// ADD_SIGNAL(MethodInfo("edited_object_changed"));
	// ADD_SIGNAL(MethodInfo("restart_requested"));
}

ObjectInspector::ObjectInspector() {
	object = nullptr;

	base_vbox = memnew(VBoxContainer);
	base_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(base_vbox);

	begin_vbox = memnew(VBoxContainer);
	base_vbox->add_child(begin_vbox);
	begin_vbox->hide();

	main_vbox = memnew(VBoxContainer);
	base_vbox->add_child(main_vbox);

	set_horizontal_scroll_mode(SCROLL_MODE_DISABLED);
	set_follow_focus(true);

	changing = 0;
	search_box = nullptr;
	_prop_edited = "property_edited";
	set_process(false);
	set_focus_mode(FocusMode::FOCUS_ALL);
	property_focusable = -1;
	property_clipboard = Variant();

	get_v_scroll_bar()->connect(SceneStringName(value_changed), callable_mp(this, &ObjectInspector::_vscroll_changed));
	update_scroll_request = -1;
	if (EditorSettings::get_singleton()) {
		refresh_countdown = float(EDITOR_GET("docks/property_editor/auto_refresh_interval"));
	} else {
		//used when class is created by the docgen to dump default values of everything bindable, editorsettings may not be created
		refresh_countdown = 0.33;
	}

	ED_SHORTCUT("property_editor/copy_value", TTRC("Copy Value"), KeyModifierMask::CMD_OR_CTRL | Key::C);
	ED_SHORTCUT("property_editor/paste_value", TTRC("Paste Value"), KeyModifierMask::CMD_OR_CTRL | Key::V);
	ED_SHORTCUT("property_editor/copy_property_path", TTRC("Copy Property Path"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::C);

	// // `use_settings_name_style` is true by default, set the name style accordingly.
	// set_property_name_style(PropertyNameProcessor::get_singleton()->get_settings_style());

	set_draw_focus_border(true);
	set_scroll_on_drag_hover(true);
}