#include "file_system_tree.h"

#include "core/io/config_file.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tree.h"

#include "scene/scene_string_names.h"

const FileInfo *FileSystemTreeDirectory::get_info() const {
	return &info;
}

int FileSystemTreeDirectory::get_subdir_count() const {
	return subdirs.size();
}

FileSystemTreeDirectory *FileSystemTreeDirectory::get_subdir(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, subdirs.size(), nullptr);
	return subdirs[p_idx];
}

int FileSystemTreeDirectory::get_file_count() const {
	return files.size();
}

FileInfo *FileSystemTreeDirectory::get_file(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, files.size(), nullptr);
	return files[p_idx];
}

FileSystemTreeDirectory *FileSystemTreeDirectory::get_parent() {
	return parent;
}

FileSystemTreeDirectory::FileSystemTreeDirectory() {
	parent = nullptr;
}

FileSystemTreeDirectory::~FileSystemTreeDirectory() {
	for (FileInfo *fi : files) {
		memdelete(fi);
	}

	for (FileSystemTreeDirectory *dir : subdirs) {
		memdelete(dir);
	}
}

void FileSystemTree::_item_edited() {
	TreeItem *ti = tree->get_edited();
	int col_index = tree->get_edited_column();
	String new_name = ti->get_text(col_index).strip_edges();

	Dictionary d = ti->get_metadata(0);
	if (_rename_operation_confirm(new_name)) {
		String new_path = ((String)d["path"]).get_base_dir().path_join(new_name);
		d["name"] = new_name;
		d["path"] = new_path;
		ti->set_metadata(0, d);

		ti->set_icon(0, FileSystemAccess::get_icon(new_path, d["is_dir"]));
	} else {
		ti->set_text(0, d["name"]);
	}
}

void FileSystemTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_update_tree(Vector<String>(), true);
		} break;

		case NOTIFICATION_PROCESS: {
			while (collapsed_changed_items.front()) {
				_scan_and_create_tree_items(collapsed_changed_items.front()->get());

				collapsed_changed_items.pop_front();
			}
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			_update_tree(get_uncollapsed_paths());
		} break;
	}
}

void FileSystemTree::_update_tree(const Vector<String> &p_uncollapsed_paths, bool p_uncollapse_root, bool p_scroll_to_selected) {
	updating_tree = true;

	tree->clear();
	TreeItem *root = tree->create_item();

	Vector<String> uncollapsed_paths = p_uncollapsed_paths;
	if (p_uncollapse_root) {
		uncollapsed_paths.push_back(filesystem->get_info()->name);
	}

	_create_tree(root, filesystem, uncollapsed_paths);

	if (p_scroll_to_selected) {
		tree->ensure_cursor_is_visible();
	}

	updating_tree = false;
}

void FileSystemTree::_create_tree(TreeItem *p_parent, const FileSystemTreeDirectory *p_dir, const Vector<String> &p_uncollapsed_paths) {
	// Create a tree item for the subdirectory.
	TreeItem *subdirectory_item = tree->create_item(p_parent);
	const FileInfo *fi = p_dir->get_info();
	String dname = fi->name; // dir name
	String lpath = fi->path; // dir path
	// print_line("create tree: " + lpath);

	subdirectory_item->set_text(0, dname);
	subdirectory_item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);
	// subdirectory_item->set_icon(0, get_app_theme_icon(SNAME("Folder")));
	// subdirectory_item->set_icon(0, fsa->get_file_icon_from_system(p_path, fsa->dir_exists(p_path)));
	subdirectory_item->set_icon(0, fi->icon);
	// if (da->is_link(lpath)) {
	// 	subdirectory_item->set_icon_overlay(0, get_app_theme_icon(SNAME("LinkOverlay")));
	// 	subdirectory_item->set_tooltip_text(0, vformat(RTR("Link to: %s"), da->read_link(lpath)));
	// }
	subdirectory_item->set_selectable(0, true);

	Dictionary d;
	d["name"] = dname;
	d["path"] = lpath;
	d["is_dir"] = true;
	d["data"] = p_dir;
	subdirectory_item->set_metadata(0, d);

	if (selected_path == lpath || (selected_path.get_base_dir() == lpath)) {
		subdirectory_item->select(0);
		// Keep select an item when re-created a tree
		// To prevent crashing when nothing is selected.
		subdirectory_item->set_as_cursor(0);
	}

	// if (p_unfold_path && current_path.begins_with(lpath) && current_path != lpath) {
	// 	subdirectory_item->set_collapsed(false);
	// } else {
	subdirectory_item->set_collapsed(!p_uncollapsed_paths.has(lpath));
	// }

	// Create items for all subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_create_tree(subdirectory_item, p_dir->get_subdir(i), p_uncollapsed_paths);
	}

	// Build the tree.
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		_create_tree_item(subdirectory_item, p_dir->get_file(i));
		// for (const FileInfo *file_info : p_dir->files) {
		// 	_create_tree_item(subdirectory_item, file_info);
	}
}

void FileSystemTree::_create_tree_item(TreeItem *p_parent, const FileInfo *p_file_info) {
	const int icon_size = get_app_theme_constant(SNAME("class_icon_size"));
	TreeItem *file_item = tree->create_item(p_parent);
	const String file_metadata = p_file_info->path;
	file_item->set_text(0, p_file_info->name);
	file_item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);
	file_item->set_icon(0, p_file_info->icon);
	// if (da->is_link(file_metadata)) {
	// 	file_item->set_icon_overlay(0, get_app_theme_icon(SNAME("LinkOverlay")));
	// 	// TRANSLATORS: This is a tooltip for a file that is a symbolic link to another file.
	// 	file_item->set_tooltip_text(0, vformat(RTR("Link to: %s"), da->read_link(file_metadata)));
	// }
	file_item->set_icon_max_width(0, icon_size);
	// Color parent_bg_color = p_parent->get_custom_bg_color(0);
	// if (has_custom_color) {
	// 	file_item->set_custom_bg_color(0, parent_bg_color.darkened(ITEM_BG_DARK_SCALE));
	// } else if (parent_bg_color != Color()) {
	// 	file_item->set_custom_bg_color(0, parent_bg_color);
	// }
	file_item->set_selectable(0, true);
	Dictionary d;
	d["name"] = p_file_info->name;
	d["path"] = file_metadata;
	d["is_dir"] = false;
	file_item->set_metadata(0, d);
	// if (!p_select_in_favorites && current_path == file_metadata) {
	if (home_path == file_metadata) {
		file_item->select(0);
		file_item->set_as_cursor(0);
	}
}

void FileSystemTree::_select_file(const String &p_path, bool p_navigate) {
	// String fpath = p_path;
	// if (fpath.ends_with("/")) {
	// 	// Ignore a directory.
	// } else if (fpath != "Favorites") {
	// 	if (FileAccess::exists(fpath + ".import")) {
	// 		Ref<ConfigFile> config;
	// 		config.instantiate();
	// 		Error err = config->load(fpath + ".import");
	// 		if (err == OK) {
	// 			if (config->has_section_key("remap", "importer")) {
	// 				String importer = config->get_value("remap", "importer");
	// 				if (importer == "keep" || importer == "skip") {
	// 					EditorNode::get_singleton()->show_warning(TTR("Importing has been disabled for this file, so it can't be opened for editing."));
	// 					return;
	// 				}
	// 			}
	// 		}
	// 	}

	// 	String resource_type = ResourceLoader::get_resource_type(fpath);

	// 	if (resource_type == "PackedScene" || resource_type == "AnimationLibrary") {
	// 		bool is_imported = false;

	// 		{
	// 			List<String> importer_exts;
	// 			ResourceImporterScene::get_scene_importer_extensions(&importer_exts);
	// 			String extension = fpath.get_extension();
	// 			for (const String &E : importer_exts) {
	// 				if (extension.nocasecmp_to(E) == 0) {
	// 					is_imported = true;
	// 					break;
	// 				}
	// 			}
	// 		}

	// 		if (is_imported) {
	// 			SceneImportSettingsDialog::get_singleton()->open_settings(p_path, resource_type);
	// 		} else if (resource_type == "PackedScene") {
	// 			EditorNode::get_singleton()->open_request(fpath);
	// 		} else {
	// 			EditorNode::get_singleton()->load_resource(fpath);
	// 		}
	// 	} else if (ResourceLoader::is_imported(fpath)) {
	// 		// If the importer has advanced settings, show them.
	// 		int order;
	// 		bool can_threads;
	// 		String name;
	// 		Error err = ResourceFormatImporter::get_singleton()->get_import_order_threads_and_importer(fpath, order, can_threads, name);
	// 		bool used_advanced_settings = false;
	// 		if (err == OK) {
	// 			Ref<ResourceImporter> importer = ResourceFormatImporter::get_singleton()->get_importer_by_name(name);
	// 			if (importer.is_valid() && importer->has_advanced_options()) {
	// 				importer->show_advanced_options(fpath);
	// 				used_advanced_settings = true;
	// 			}
	// 		}

	// 		if (!used_advanced_settings) {
	// 			EditorNode::get_singleton()->load_resource(fpath);
	// 		}

	// 	} else {
	// 		EditorNode::get_singleton()->load_resource(fpath);
	// 	}
	// }
	// if (p_navigate) {
	// 	_navigate_to_path(fpath, p_select_in_favorites);
	// }
}

void FileSystemTree::_tree_activate_file() {
	TreeItem *selected = tree->get_selected();
	if (selected) {
		Dictionary d = selected->get_metadata(0);
		String path = d["path"];
		bool is_dir = d["is_dir"];

		// // TreeItem *parent = selected->get_parent();
		// bool is_folder = file_path.ends_with("/");

		// _select_file(file_path, is_folder);

		emit_signal(SNAME("item_activated"), path, is_dir);
	}
}

void FileSystemTree::_tree_item_selected() {
	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	emit_signal(SceneStringName(item_selected), path, is_dir);
}

void FileSystemTree::_tree_multi_selected(Object *p_item, int p_column, bool p_selected) {
	// Return if we don't select something new.
	if (!p_selected) {
		return;
	}

	// Tree item selected.
	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	emit_signal(SceneStringName(item_selected), path, is_dir);
}

void FileSystemTree::_tree_item_mouse_select(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button == MouseButton::LEFT) {
		_tree_lmb_select(p_pos, p_button);
	} else if (p_button == MouseButton::RIGHT) {
		_tree_rmb_select(p_pos, p_button);
	}
}

void FileSystemTree::_tree_lmb_select(const Vector2 &p_pos, MouseButton p_button) {
}

void FileSystemTree::_tree_rmb_select(const Vector2 &p_pos, MouseButton p_button) {
	// tree->grab_focus();

	// // Right click is pressed in the tree.
	// Vector<String> paths = _tree_get_selected(false);

	// tree_popup->clear();

	// // Popup.
	// if (!paths.is_empty()) {
	// 	tree_popup->reset_size();
	// 	_file_and_folders_fill_popup(tree_popup, paths);
	// 	tree_popup->set_position(tree->get_screen_position() + p_pos);
	// 	tree_popup->reset_size();
	// 	tree_popup->popup();
	// }
}

void FileSystemTree::_tree_item_collapsed(TreeItem *p_item) {
	if (updating_tree) {
		return;
	}
	emit_signal(SNAME("item_collapsed"));

	print_line("item collapsed changed: " + String(p_item->get_metadata(0)));
	if (!p_item->is_collapsed()) {
		Dictionary d = p_item->get_metadata(0);
		FileSystemTreeDirectory *fs = Object::cast_to<FileSystemTreeDirectory>(d["data"]);
		for (FileSystemTreeDirectory *sub_dir : fs->subdirs) {
			if (!sub_dir->scanned) {
				collapsed_changed_items.push_back(p_item);
			}
		}
	}
}

Vector<String> FileSystemTree::_get_selected() const {
	// Build a list of selected items with the active one at the first position.
	Vector<String> selected_strings;

	TreeItem *cursor_item = tree->get_selected();
	if (cursor_item && cursor_item->is_selected(0)) {
		Dictionary d = cursor_item->get_metadata(0);
		selected_strings.push_back(d["path"]);
	}

	TreeItem *selected = tree->get_root();
	selected = tree->get_next_selected(selected);
	while (selected) {
		if (selected != cursor_item && selected->is_visible_in_tree()) {
			Dictionary d = selected->get_metadata(0);
			selected_strings.push_back(d["path"]);
		}
		selected = tree->get_next_selected(selected);
	}

	return selected_strings;
}

void FileSystemTree::_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	tree->deselect_all();
	popup_menu(p_pos, MENU_MODE_EMPTY);
}

void FileSystemTree::_item_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *selected = tree->get_selected();
	if (selected) {
		Dictionary d = selected->get_metadata(0);
		bool is_dir = d["is_dir"];
		if (is_dir) {
			popup_menu(p_pos, MENU_MODE_FOLDER);
		} else {
			popup_menu(p_pos, MENU_MODE_FILE);
		}
	}
}

void FileSystemTree::_scan_dir(FileSystemTreeDirectory *r_dir, const String &p_path, bool p_scan_subdirs) {
	List<FileInfo> subdirs;
	List<FileInfo> files;
	Error err = FileSystemAccess::list_file_infos(p_path, subdirs, files);
	if (err != OK) {
		return;
	}

	// Create all items for the files in the subdirectory.
	for (const FileInfo &file_info : subdirs) {
		FileSystemTreeDirectory *sub_dir = memnew(FileSystemTreeDirectory);
		sub_dir->parent = r_dir;
		sub_dir->info = file_info;
		r_dir->subdirs.push_back(sub_dir);
		if (p_scan_subdirs) {
			_scan_dir(sub_dir, file_info.path, p_scan_subdirs);
		}
	}
	for (const FileInfo &file_info : files) {
		FileInfo *fi = memnew(FileInfo);
		*fi = file_info;
		r_dir->files.push_back(fi);
	}
	r_dir->scanned = true;
}

void FileSystemTree::_scan_and_create_tree_items(TreeItem *p_parent) {
	Dictionary d = p_parent->get_metadata(0);

	// scan and create tree item for sub dir
	FileSystemTreeDirectory *fs = Object::cast_to<FileSystemTreeDirectory>(d["data"]);
	updating_tree = true;
	for (int i = 0; i < p_parent->get_child_count(); i++) {
		TreeItem *item = p_parent->get_child(i);
		for (FileSystemTreeDirectory *sub_dir : fs->subdirs) {
			if (!sub_dir->scanned && sub_dir->info.name == item->get_text(0)) {
				_scan_dir(sub_dir, sub_dir->info.path);
				for (FileSystemTreeDirectory *new_dir : sub_dir->subdirs) {
					_create_tree(item, new_dir);
				}
				for (FileInfo *file : sub_dir->files) {
					_create_tree_item(item, file);
				}
			}
		}
	}
	updating_tree = false;
}

void FileSystemTree::_initialize_filesystem() {
	filesystem = memnew(FileSystemTreeDirectory);
	filesystem->parent = nullptr;
	filesystem->info.name = home_path;
	filesystem->info.path = home_path;
	filesystem->info.icon = FileSystemAccess::get_icon(home_path);
	_scan_dir(filesystem, home_path);
	for (FileSystemTreeDirectory *sub_dir : filesystem->subdirs) {
		_scan_dir(sub_dir, sub_dir->info.path);
	}
}

void FileSystemTree::_update_file_ui() {
}

void FileSystemTree::_set_empty_menu_item(PopupMenu *p_popup) {
	p_popup->add_item(RTR("Expand to current"), FILE_MENU_EXPAND_TO_CURRENT);
}

void FileSystemTree::_set_file_menu_item(PopupMenu *p_popup) {
	p_popup->add_item(RTR("Open"), FILE_MENU_OPEN);

	p_popup->add_separator();

	p_popup->add_item(RTR("Copy path"), FILE_MENU_COPY_PATH);

	p_popup->add_separator();

	p_popup->add_item(RTR("Cut"), FILE_MENU_CUT);
	p_popup->add_item(RTR("Copy"), FILE_MENU_COPY);

	p_popup->add_separator();

	p_popup->add_item(RTR("Delete"), FILE_MENU_DELETE);
	p_popup->add_item(RTR("Rename"), FILE_MENU_RENAME);
}

void FileSystemTree::_set_folder_menu_item(PopupMenu *p_popup) {
	p_popup->add_item(RTR("Expand"), FILE_MENU_EXPAND);
	// p_popup->add_item(RTR("Collapse"), FILE_MENU_COLLAPSE);

	p_popup->add_separator();

	p_popup->add_item(RTR("Open"), FILE_MENU_OPEN);
	p_popup->add_item(RTR("Open in new tab"), FILE_MENU_OPEN_IN_NEW_TAB);
	p_popup->add_item(RTR("Open in new window"), FILE_MENU_OPEN_IN_NEW_WINDOW);
	// p_popup->add_item(RTR("Pin"), FILE_MENU_PIN_TO_QUICK_ACCESS);

	p_popup->add_separator();

	p_popup->add_item(RTR("Open in terminal"), FILE_MENU_OPEN_IN_TERMINAL);

	p_popup->add_separator();

	p_popup->add_item(RTR("Copy path"), FILE_MENU_COPY_PATH);

	p_popup->add_separator();

	p_popup->add_item(RTR("Cut"), FILE_MENU_CUT);
	p_popup->add_item(RTR("Copy"), FILE_MENU_COPY);
	p_popup->add_item(RTR("Paste"), FILE_MENU_PASTE);

	p_popup->add_separator();

	p_popup->add_item(RTR("Delete"), FILE_MENU_DELETE);
	p_popup->add_item(RTR("Rename"), FILE_MENU_RENAME);

	p_popup->add_separator();

	PopupMenu *new_menu = memnew(PopupMenu);
	// new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemControl::_item_menu_id_pressed));
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("Folder..."), FILE_MENU_NEW_FOLDER);
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("TextFile..."), FILE_MENU_NEW_TEXTFILE);
	p_popup->add_submenu_node_item(RTR("Create New"), new_menu, FILE_MENU_NEW);
	p_popup->set_item_icon(new_menu->get_item_index(FILE_MENU_NEW), get_app_theme_icon(SNAME("Folder")));
}

void FileSystemTree::_bind_methods() {
	ADD_SIGNAL(MethodInfo("item_activated", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
	// ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "TreeItem")));
	ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::BOOL, "is_dir")));
	ADD_SIGNAL(MethodInfo("item_collapsed"));
}

bool FileSystemTree::edit_selected(const FileOrFolder &p_selected) {
	bool result = tree->edit_selected(true);

	if (p_selected.is_file) {
		String name = p_selected.path.get_file();
		tree->set_editor_selection(0, name.rfind_char('.'));
	} else {
		String name = p_selected.path.get_file();
		tree->set_editor_selection(0, name.length());
	}

	return result;
}

Vector<String> FileSystemTree::get_selected_paths() const {
	// Build a list of selected items with the active one at the first position.
	Vector<String> selected_strings;

	// TreeItem *cursor_item = tree->get_selected();
	// if (cursor_item) {
	// 	selected_strings.push_back(cursor_item->get_metadata(0));
	// }

	// TreeItem *selected = tree->get_root();
	// selected = tree->get_next_selected(selected);
	// while (selected) {
	// 	if (selected != cursor_item && selected->is_visible_in_tree()) {
	// 		selected_strings.push_back(selected->get_metadata(0));
	// 	}
	// 	selected = tree->get_next_selected(selected);
	// }

	return selected_strings;
}

Vector<String> FileSystemTree::get_uncollapsed_paths() const {
	Vector<String> uncollapsed_paths;
	TreeItem *root = tree->get_root();
	if (root) {
		// BFS to find all uncollapsed paths of the file system directory.
		TreeItem *filesystem_subtree = root->get_first_child();
		if (filesystem_subtree) {
			List<TreeItem *> queue;
			queue.push_back(filesystem_subtree);

			while (!queue.is_empty()) {
				TreeItem *ti = queue.back()->get();
				queue.pop_back();
				if (!ti->is_collapsed() && ti->get_child_count() > 0) {
					Dictionary d = ti->get_metadata(0);
					uncollapsed_paths.push_back(d["path"]);
				}
				for (int i = 0; i < ti->get_child_count(); i++) {
					queue.push_back(ti->get_child(i));
				}
			}
		}
	}
	return uncollapsed_paths;
}

void FileSystemTree::save_layout_to_config(Ref<ConfigFile> p_layout, const String &p_section) const {
	Vector<String> uncollapsed_paths = get_uncollapsed_paths();
	print_line("uncollapsed_paths: " + itos(uncollapsed_paths.size()));
	p_layout->set_value(p_section, "dock_filesystem_uncollapsed_paths", uncollapsed_paths);
}

void FileSystemTree::load_layout_from_config(Ref<ConfigFile> p_layout, const String &p_section) {
	// Restore collapsed state.
	PackedStringArray uncollapsed_paths;
	if (p_layout->has_section_key(p_section, "dock_filesystem_uncollapsed_paths")) {
		uncollapsed_paths = p_layout->get_value(p_section, "dock_filesystem_uncollapsed_paths");
	}

	if (!uncollapsed_paths.is_empty()) {
		for (int i = 0; i < uncollapsed_paths.size(); i++) {
			String uncollapsed_path = String(uncollapsed_paths[i]);

			// TODO: Scan all paths, including those with unexpanded parent directories.
			for (TreeItem *current = tree->get_root(); current; current = current->get_next_in_tree()) {
				Dictionary d = current->get_metadata(0);
				if (uncollapsed_path == (String)d["path"]) {
					_scan_and_create_tree_items(current);
					updating_tree = true;
					current->set_collapsed(false);
					updating_tree = false;
					break;
				}
			}
		}
		// // _update_tree(uncollapsed_paths);
		// tree->queue_redraw();
	}
}

FileSystemTree::FileSystemTree() {
	set_custom_minimum_size(Size2(180, 20));

	vbox = memnew(VBoxContainer);
	add_child(vbox);
	vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	pathhb = memnew(HBoxContainer);
	vbox->add_child(pathhb);

	home = memnew(Button);
	home->set_theme_type_variation(SceneStringName(FlatButton));
	home->set_tooltip_text(RTR("Go to next folder."));
	home->set_focus_mode(FOCUS_NONE);
	dir_up = memnew(Button);
	dir_up->set_theme_type_variation(SceneStringName(FlatButton));
	dir_up->set_tooltip_text(RTR("Go to parent folder."));
	dir_up->set_disabled(true);
	dir_up->set_focus_mode(FOCUS_NONE);
	refresh = memnew(Button);
	refresh->set_theme_type_variation(SceneStringName(FlatButton));
	refresh->set_tooltip_text(RTR("Refresh files."));
	refresh->set_focus_mode(FOCUS_NONE);

	// home->connect(SceneStringName(pressed), callable_mp(this, &FileManager::_go_forward));
	// dir_up->connect(SceneStringName(pressed), callable_mp(this, &FileManager::_go_up));
	// refresh->connect(SceneStringName(pressed), callable_mp(this, &FileManager::_update_file_list));

	pathhb->add_child(home);
	pathhb->add_child(dir_up);
	pathhb->add_child(refresh);

	dir = memnew(LineEdit);
	dir->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	dir->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pathhb->add_child(dir);

	// TODO: show
	pathhb->hide();

	tree = memnew(Tree);
	vbox->add_child(tree);
	tree->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tree->set_hide_root(true);
	// SET_DRAG_FORWARDING_GCD(tree, FileSystemTree);
	tree->set_allow_rmb_select(true);
	tree->set_allow_reselect(true);
	tree->set_select_mode(Tree::SELECT_MULTI);
	// tree->set_custom_minimum_size(Size2(40 * APP_SCALE, 15 * APP_SCALE));
	tree->set_column_clip_content(0, true);

	tree->connect("item_collapsed", callable_mp(this, &FileSystemTree::_tree_item_collapsed));

	// double-clicking selected.
	tree->connect("item_activated", callable_mp(this, &FileSystemTree::_tree_activate_file));
	tree->connect("item_selected", callable_mp(this, &FileSystemTree::_tree_item_selected));
	tree->connect("multi_selected", callable_mp(this, &FileSystemTree::_tree_multi_selected));
	// tree->connect("item_mouse_selected", callable_mp(this, &FileSystemTree::_tree_item_mouse_select));
	// tree->connect("empty_clicked", callable_mp(this, &FileSystemTree::_tree_empty_click));
	tree->connect("item_mouse_selected", callable_mp(this, &FileSystemTree::_item_clicked));
	tree->connect("empty_clicked", callable_mp(this, &FileSystemTree::_empty_clicked));
	// tree->connect("nothing_selected", callable_mp(this, &FileSystemTree::_tree_empty_selected));
	// tree->connect(SceneStringName(gui_input), callable_mp(this, &FileSystemTree::_tree_gui_input));
	// tree->connect(SceneStringName(mouse_exited), callable_mp(this, &FileSystemTree::_tree_mouse_exited));
	tree->connect("item_edited", callable_mp(this, &FileSystemTree::_item_edited));

	home_path = COMPUTER_PATH;

	_initialize_filesystem();

	set_process(true);
}

FileSystemTree::~FileSystemTree() {
	if (filesystem) {
		memdelete(filesystem);
	}
	filesystem = nullptr;
}
