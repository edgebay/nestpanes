#include "navigation_pane.h"

#include "app/app_modules/file_management/gui/file_context_menu.h"
#include "app/app_modules/file_management/gui/file_system_tree.h"

#include "app/gui/app_control.h"
#include "app/themes/app_scale.h"

#include "app/app_modules/file_management/file_system.h"

String NavigationPane::_get_pane_title() const {
	return "Navigation";
}

Ref<Texture2D> NavigationPane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("Filesystem"));
}

void NavigationPane::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			if (tree->get_root() != nullptr) {
				_update_tree();
			}
		} break;
	}
}

void NavigationPane::_update_tree() {
	ERR_FAIL_NULL(file_system);

	if (tree->is_connected("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed))) {
		tree->disconnect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
	}
	updating_tree = true;

	const FileSystemDirectory *file_system_root = file_system->get_root();
	bool uncollapse_root = (tree->get_root() == nullptr);
	Vector<String> paths = _get_uncollapsed_paths(uncollapse_root);
	// print_line("uncollapse: ", uncollapse_root, paths);

	tree->clear();
	TreeItem *root = tree->create_item();
	_create_tree(root, file_system_root, paths);

	_clear_uncollapsed_paths();

	tree->ensure_cursor_is_visible();

	updating_tree = false;
	tree->connect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
}

void NavigationPane::_update_subtree(TreeItem *p_parent, const FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(file_system);
	ERR_FAIL_NULL(p_parent);
	ERR_FAIL_NULL(p_dir);

	Dictionary d = p_parent->get_metadata(0);
	String path = d["path"];
	if (path != p_dir->get_path()) {
		return;
	}

	if (tree->is_connected("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed))) {
		tree->disconnect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
	}
	updating_tree = true;

	Vector<String> paths = _get_uncollapsed_paths();
	bool uncollapsed = paths.has(path);
	p_parent->set_collapsed(!uncollapsed);
	// print_line("uncollapse: ", path, uncollapsed, paths);

	p_parent->clear_children();

	// Create items for all subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		FileSystemDirectory *subdir = p_dir->get_subdir(i);
		_create_tree(p_parent, subdir, paths);
	}

	_clear_uncollapsed_paths();

	// Build the tree.
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		_create_file_item(p_parent, p_dir->get_file(i));
	}

	updating_tree = false;
	tree->connect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
}

void NavigationPane::_create_tree(TreeItem *p_parent, const FileSystemDirectory *p_dir, const Vector<String> &p_uncollapsed_paths) {
	ERR_FAIL_NULL(p_parent);
	ERR_FAIL_NULL(p_dir);

	// Create a tree item for the subdirectory.

	TreeItem *subdirectory_item = tree->add_item(p_dir->get_info(), p_parent);

	String path = p_dir->get_path();
	if (selected_path == path || (selected_path.get_base_dir() == path)) {
		subdirectory_item->select(0);
		// Keep select an item when re-created a tree
		// To prevent crashing when nothing is selected.
		subdirectory_item->set_as_cursor(0);
	}

	bool uncollapsed = p_uncollapsed_paths.has(path);
	// TODO
	// if (p_unfold_path && selected_path.begins_with(path) && selected_path != path) {
	// 	subdirectory_item->set_collapsed(false);
	// } else {
	subdirectory_item->set_collapsed(!uncollapsed);
	// }

	// print_line("uncollapse: ", path, uncollapsed);

	// Create items for all subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		FileSystemDirectory *subdir = p_dir->get_subdir(i);
		_create_tree(subdirectory_item, subdir, p_uncollapsed_paths);
	}

	// Build the tree.
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		_create_file_item(subdirectory_item, p_dir->get_file(i));
	}
}

void NavigationPane::_create_file_item(TreeItem *p_parent, const FileInfo *p_file_info) {
	ERR_FAIL_NULL(p_parent);
	ERR_FAIL_NULL(p_file_info);

	TreeItem *file_item = tree->add_item(*p_file_info, p_parent);

	if (selected_path == p_file_info->path) {
		file_item->select(0);
		file_item->set_as_cursor(0);
	}
}

void NavigationPane::_on_item_activated() {
	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	// print_line("on item_activated", path, is_dir);
	if (is_dir) {
		callable_mp(selected, &TreeItem::set_collapsed).call_deferred(!selected->is_collapsed());
	} else {
		emit_signal(SNAME("item_activated"), path, is_dir);
	}
}

void NavigationPane::_on_multi_selected(Object *p_item, int p_column, bool p_selected) {
	if (!p_selected) {
		return;
	}

	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	emit_signal(SceneStringName(item_selected), path, is_dir);
}

void NavigationPane::_tree_item_collapsed(TreeItem *p_item) {
	Dictionary d = p_item->get_metadata(0);
	String path = d["path"];

	if (!p_item->is_collapsed()) {
		file_system->scan(path);
	}

	_data_changed();
}

TreeItem *NavigationPane::_search_item(const String &p_path) {
	for (TreeItem *current = tree->get_root(); current; current = current->get_next_in_tree()) {
		Dictionary d = current->get_metadata(0);
		if (d["path"] == p_path) {
			return current;
		}
	}
	return nullptr;
}

void NavigationPane::_on_file_system_changed(const String &p_path) {
	TreeItem *item = _search_item(p_path);
	FileSystemDirectory *dir = file_system->get_dir(p_path);
	// print_line("fs changed: ", p_path, item, dir);
	if (item && dir) {
		callable_mp(this, &NavigationPane::_update_subtree).call_deferred(item, dir);
	} else {
		callable_mp(this, &NavigationPane::_update_tree).call_deferred();
	}
}

Vector<String> NavigationPane::_get_uncollapsed_paths(bool p_uncollapse_root) {
	Vector<String> paths = tree->get_uncollapsed_paths();
	if (!uncollapsed_paths.is_empty()) {
		paths.append_array(uncollapsed_paths);
	}
	if (p_uncollapse_root && file_system) {
		const FileSystemDirectory *file_system_root = file_system->get_root();
		if (!paths.has(file_system_root->get_path())) {
			paths.push_back(file_system_root->get_path());
		}
	}

	return paths;
}

void NavigationPane::_clear_uncollapsed_paths() {
	if (!uncollapsed_paths.is_empty()) {
		bool loaded = true;
		Vector<String> updated_paths = tree->get_uncollapsed_paths();
		for (const String &path : uncollapsed_paths) {
			if (!updated_paths.has(path)) {
				loaded = false;
				break;
			}
		}
		// print_line("loaded: ", loaded, updated_paths);
		if (loaded) {
			uncollapsed_paths.clear();
		}
	}
}

void NavigationPane::set_file_system(FileSystem *p_file_system) {
	if (file_system == p_file_system) {
		return;
	}

	file_system = p_file_system;
	file_system->connect("file_system_changed", callable_mp(this, &NavigationPane::_on_file_system_changed));

	context_menu->set_file_system(file_system);

	const FileSystemDirectory *file_system_root = file_system->get_root();
	String root_path = file_system_root->get_path();

	callable_mp(this, &NavigationPane::_update_tree).call_deferred();
}

void NavigationPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("item_activated", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
	ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
}

Dictionary NavigationPane::get_config_data() {
	Dictionary d;
	d["uncollapsed_paths"] = tree->get_uncollapsed_paths();
	// print_line("config data: ", d["uncollapsed_paths"]);
	return d;
}

void NavigationPane::apply_config_data(const Dictionary &p_dict) {
	ERR_FAIL_NULL(file_system);

	if (p_dict.has("uncollapsed_paths")) {
		uncollapsed_paths.clear();

		Vector<String> paths = p_dict.get("uncollapsed_paths", Vector<String>());
		for (const String &path : paths) {
			if (file_system->is_valid_dir_path(path)) {
				uncollapsed_paths.push_back(path);
			}
		}

		if (!uncollapsed_paths.is_empty()) {
			file_system->scan(uncollapsed_paths, true);
		}
		// print_line("load: ", uncollapsed_paths);
	}
}

NavigationPane::NavigationPane() :
		PaneBase(get_class_static()) {
	set_custom_minimum_size(Size2(200, 320) * APP_SCALE);

	tree = memnew(FileSystemTree);
	add_child(tree);
	tree->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// TODO
	// SET_DRAG_FORWARDING_GCD(tree, NavigationPane);
	tree->set_custom_minimum_size(Size2(40 * APP_SCALE, 15 * APP_SCALE));

	// double-clicking selected.
	tree->connect("item_activated", callable_mp(this, &NavigationPane::_on_item_activated));
	tree->connect("multi_selected", callable_mp(this, &NavigationPane::_on_multi_selected));
	// TODO: edit
	// tree->connect("item_edited", callable_mp(this, &NavigationPane::_item_edited));

	context_menu = memnew(FileContextMenu);
	add_child(context_menu);
	tree->set_context_menu(context_menu);
}

NavigationPane::~NavigationPane() {
	file_system = nullptr;
}
