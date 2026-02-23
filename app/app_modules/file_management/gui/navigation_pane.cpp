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

	if (tree->has_connections("item_collapsed")) {
		tree->disconnect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
	}
	updating_tree = true;

	tree->clear();
	TreeItem *root = tree->create_item();
	const FileSystemDirectory *file_system_root = file_system->get_root();

	_create_tree(root, file_system_root, get_uncollapsed_paths());

	tree->ensure_cursor_is_visible();

	updating_tree = false;
	tree->connect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
}

void NavigationPane::_update_subtree(TreeItem *p_parent, const FileSystemDirectory *p_dir, const Vector<String> &p_uncollapsed_paths) {
	ERR_FAIL_NULL(file_system);
	ERR_FAIL_NULL(p_parent);
	ERR_FAIL_NULL(p_dir);

	Dictionary d = p_parent->get_metadata(0);
	String path = d["path"];
	if (path != p_dir->get_path()) {
		return;
	}

	if (tree->has_connections("item_collapsed")) {
		tree->disconnect("item_collapsed", callable_mp(this, &NavigationPane::_tree_item_collapsed));
	}
	updating_tree = true;

	p_parent->clear_children();

	// Create items for all subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		FileSystemDirectory *subdir = p_dir->get_subdir(i);
		_create_tree(p_parent, subdir, p_uncollapsed_paths);
	}

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

void NavigationPane::_tree_activate_file() {
	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	if (is_dir) {
		callable_mp(selected, &TreeItem::set_collapsed).call_deferred(!selected->is_collapsed());
	} else {
		emit_signal(SNAME("item_activated"), path, is_dir);
	}
}

void NavigationPane::_tree_multi_selected(Object *p_item, int p_column, bool p_selected) {
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
		uncollapsed_paths.push_back(path);
	} else {
		uncollapsed_paths.erase(path);
	}

	_data_changed();
}

void NavigationPane::_build_empty_menu() {
	context_menu->clear();

	// TODO
	// context_menu->add_file_item(FileContextMenu::FILE_MENU_EXPAND_TO_CURRENT);
}

void NavigationPane::_build_file_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY_PATH);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_SHOW_IN_EXPLORER);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void NavigationPane::_build_folder_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
	// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB); // TODO: create new tab in central area
	// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY_PATH);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW);
	context_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
	FileContextMenu *new_menu = memnew(FileContextMenu);
	new_menu->set_file_system(file_system);
	new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
	new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
	new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
	new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("File")));
	context_menu->set_item_submenu_node(-1, new_menu);
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &NavigationPane::_context_menu_id_pressed));

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_SHOW_IN_EXPLORER);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void NavigationPane::_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	tree->deselect_all();

	// TODO
	// Vector<String> targets;
	// targets.push_back(get_path());
	// context_menu->set_targets(targets);

	// _build_empty_menu();

	// context_menu->set_position(get_screen_position() + p_pos);
	// context_menu->reset_size();
	// context_menu->popup();
}

void NavigationPane::_item_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *cursor_item = tree->get_selected();
	if (!cursor_item) {
		return;
	}

	Vector<String> targets;
	TreeItem *selected = tree->get_root();
	selected = tree->get_next_selected(selected); // TODO: push cursor_item at first
	while (selected) {
		if (selected->is_visible_in_tree()) {
			Dictionary d = selected->get_metadata(0);
			targets.push_back(d["path"]);
		}
		selected = tree->get_next_selected(selected);
	}
	context_menu->set_targets(targets);

	if (targets.size() > 1) {
		// TODO: handle multi selected
	} else {
		Dictionary d = cursor_item->get_metadata(0);
		if (d["is_dir"]) {
			_build_folder_menu();
		} else {
			_build_file_menu();
		}
	}

	context_menu->set_position(get_screen_position() + p_pos);
	context_menu->reset_size();
	context_menu->popup();
}

void NavigationPane::_context_menu_id_pressed(int p_option) {
	// TODO
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
	if (item && dir) {
		callable_mp(this, &NavigationPane::_update_subtree).call_deferred(item, dir, get_uncollapsed_paths());
	} else {
		callable_mp(this, &NavigationPane::_update_tree).call_deferred();
	}
}

void NavigationPane::_load_uncollapsed_paths() {
	file_system->scan(uncollapsed_paths);
	// TODO: scan all subdir
}

Vector<String> NavigationPane::get_selected_paths() const {
	// Build a list of selected items with the active one at the first position.
	Vector<String> selected_strings;

	// TreeItem *cursor_item = tree->get_selected();
	// if (cursor_item && (p_include_unselected_cursor || cursor_item->is_selected(0)) && cursor_item != favorites_item) {
	// 	selected_strings.push_back(cursor_item->get_metadata(0));
	// }

	// TreeItem *selected = tree->get_root();
	// selected = tree->get_next_selected(selected);
	// while (selected) {
	// 	if (selected != cursor_item && selected != favorites_item && selected->is_visible_in_tree()) {
	// 		selected_strings.push_back(selected->get_metadata(0));
	// 	}
	// 	selected = tree->get_next_selected(selected);
	// }

	// if (remove_self_inclusion) {
	// 	selected_strings = _remove_self_included_paths(selected_strings);
	// }
	return selected_strings;
}

Vector<String> NavigationPane::get_uncollapsed_paths() const {
	// Vector<String> paths;
	// TreeItem *root = tree->get_root();
	// if (root) {
	// 	// BFS to find all uncollapsed paths of the file system directory.
	// 	TreeItem *file_system_subtree = root->get_first_child();
	// 	if (file_system_subtree) {
	// 		List<TreeItem *> queue;
	// 		queue.push_back(file_system_subtree);

	// 		while (!queue.is_empty()) {
	// 			TreeItem *ti = queue.back()->get();
	// 			queue.pop_back();
	// 			if (!ti->is_collapsed() && ti->get_child_count() > 0) {
	// 				Dictionary d = ti->get_metadata(0);
	// 				paths.push_back(d["path"]);
	// 			}
	// 			for (int i = 0; i < ti->get_child_count(); i++) {
	// 				queue.push_back(ti->get_child(i));
	// 			}
	// 		}
	// 	}
	// }
	// return paths;
	return uncollapsed_paths;
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
	if (!uncollapsed_paths.has(root_path)) {
		uncollapsed_paths.push_back(root_path);
	}

	callable_mp(this, &NavigationPane::_update_tree).call_deferred();
}

void NavigationPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("item_activated", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
	ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
}

Dictionary NavigationPane::get_config_data() {
	Dictionary d;
	d["uncollapsed_paths"] = get_uncollapsed_paths();
	return d;
}

void NavigationPane::apply_config_data(const Dictionary &p_dict) {
	ERR_FAIL_NULL(file_system);

	if (p_dict.has("uncollapsed_paths")) {
		uncollapsed_paths = p_dict.get("uncollapsed_paths", get_uncollapsed_paths());
		callable_mp(this, &NavigationPane::_load_uncollapsed_paths).call_deferred();
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
	tree->connect("item_activated", callable_mp(this, &NavigationPane::_tree_activate_file));
	tree->connect("multi_selected", callable_mp(this, &NavigationPane::_tree_multi_selected));
	tree->connect("item_mouse_selected", callable_mp(this, &NavigationPane::_item_clicked)); // multi_selected already contains left mouse button select
	tree->connect("empty_clicked", callable_mp(this, &NavigationPane::_empty_clicked));
	// TODO: edit
	// tree->connect("item_edited", callable_mp(this, &NavigationPane::_item_edited));

	context_menu = memnew(FileContextMenu);
	add_child(context_menu);
	context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &NavigationPane::_context_menu_id_pressed));
}

NavigationPane::~NavigationPane() {
	file_system = nullptr;
}
