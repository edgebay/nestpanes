#include "file_system_tree.h"

#include "app/app_modules/file_management/gui/file_context_menu.h"

#include "app/gui/app_control.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

static const int default_column_count = 5;
static FileSystemTree::ColumnSetting default_column_settings[default_column_count] = {
	{ 0, true, TTRC("Name"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 3 },
	{ 1, true, TTRC("Modified"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 2 },
	{ 2, false, TTRC("Created"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 2 },
	{ 3, true, TTRC("Type"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
	{ 4, true, TTRC("Size"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
};

void FileSystemTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			// p_cast->theme_cache.hovered = p_cast->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);

			// if (i == 0 && select_mode == SELECT_ROW) {
			// 	if (p_item->cells[0].selected || is_row_hovered) {
			// 		const Rect2 content_rect = _get_content_rect();
			// 		Rect2i row_rect = Rect2i(Point2i(content_rect.position.x, item_rect.position.y), Size2i(content_rect.size.x, item_rect.size.y));
			// 		row_rect = convert_rtl_rect(row_rect);

			// 		if (p_item->cells[0].selected) {
			// 			if (is_row_hovered) {
			// 				if (has_focus(true)) {
			// 					theme_cache.hovered_selected_focus->draw(ci, row_rect);
			// 				} else {
			// 					theme_cache.hovered_selected->draw(ci, row_rect);
			// 				}
			// 			} else {
			// 				if (has_focus(true)) {
			// 					theme_cache.selected_focus->draw(ci, row_rect);
			// 				} else {
			// 					theme_cache.selected->draw(ci, row_rect);
			// 				}
			// 			}
			// 		} else if (!drop_mode_flags) {
			// 			if (is_cell_button_hovered) {
			// 				theme_cache.hovered_dimmed->draw(ci, row_rect);
			// 			} else {
			// 				theme_cache.hovered->draw(ci, row_rect);
			// 			}
			// 		}
			// 	}
			// }
		} break;
	}
}

void FileSystemTree::_update_display_mode() {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_hide_root(true);
	set_allow_rmb_select(true);
	set_allow_reselect(true);

	uint32_t column_count = 1;
	if (display_mode == DISPLAY_MODE_TREE) {
		column_count = 1;

		set_select_mode(Tree::SELECT_MULTI);

		set_theme_type_variation("Tree");
		set_hide_folding(false);
		set_columns(column_count);
		set_column_titles_visible(false);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		column_count = 4;

		// set_select_mode(Tree::SELECT_MULTI);
		set_select_mode(Tree::SELECT_ROW);

		set_theme_type_variation("TreeTable");
		set_hide_folding(true);
		set_columns(column_count);
		set_column_titles_visible(true);
	}

	ERR_FAIL_COND(column_count >= column_settings.size());

	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size() && column < column_count; i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		set_column_title(column, setting.title);
		set_column_title_alignment(column, setting.alignment);
		set_column_clip_content(column, setting.clip);
		if (setting.expand_ratio < 0) {
			set_column_expand(column, false);
		} else {
			set_column_expand(column, true);
			set_column_expand_ratio(column, setting.expand_ratio);
		}
		column++;
	}
}

TreeItem *FileSystemTree::_add_tree_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = p_fi.type == FOLDER_TYPE;
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	TreeItem *item = create_item(p_parent, p_index);
	String path = p_fi.path;
	item->set_text(0, p_fi.name);
	item->set_icon(0, icon);
	item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);

	// TODO
	// if (da->is_link(path)) {
	// 	item->set_icon_overlay(0, get_app_theme_icon(SNAME("LinkOverlay")));
	// 	// TRANSLATORS: This is a tooltip for a file that is a symbolic link to another file.
	// 	item->set_tooltip_text(0, vformat(RTR("Link to: %s"), da->read_link(path)));
	// }

	const int icon_size = get_theme_constant(SNAME("class_icon_size"));
	item->set_icon_max_width(0, icon_size);

	// TODO
	// Color parent_bg_color = p_parent->get_custom_bg_color(0);
	// if (has_custom_color) {
	// 	item->set_custom_bg_color(0, parent_bg_color.darkened(ITEM_BG_DARK_SCALE));
	// } else if (parent_bg_color != Color()) {
	// 	item->set_custom_bg_color(0, parent_bg_color);
	// }

	item->set_selectable(0, true);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = path;
	d["is_dir"] = is_dir;
	item->set_metadata(0, d);

	return item;
}

TreeItem *FileSystemTree::_add_list_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = p_fi.type == FOLDER_TYPE;
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	TreeItem *item = create_item(p_parent, p_index);
	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size(); i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		// TODO: Does translation affect it?
		if (setting.title == "Name") {
			item->set_text(column, p_fi.name);
			item->set_icon(column, icon);
		} else if (setting.title == "Modified") {
			String modified_time = itos(p_fi.modified_time); // TODO
			item->set_text(column, modified_time);
		} else if (setting.title == "Created") {
			String creation_time = itos(p_fi.creation_time); // TODO
			item->set_text(column, creation_time);
		} else if (setting.title == "Type") {
			item->set_text(column, p_fi.type); // TODO
		} else if (setting.title == "Size") {
			String size = itos(p_fi.size); // TODO
			item->set_text(column, size);
		}
		// item->set_editable(column, true);
		// item->set_selectable(column, true);

		column++;
	}

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = is_dir;
	item->set_metadata(0, d);

	return item;
}

void FileSystemTree::_on_item_activated() {
	// TreeItem *selected = get_selected();
	// if (!selected) {
	// 	return;
	// }

	// Dictionary d = selected->get_metadata(0);
	// String path = d["path"];
	// bool is_dir = d["is_dir"];

	// if (is_dir) {
	// 	callable_mp(selected, &TreeItem::set_collapsed).call_deferred(!selected->is_collapsed());
	// } else {
	// 	emit_signal(SNAME("item_activated"), path, is_dir);
	// }
}

void FileSystemTree::_on_multi_selected(Object *p_item, int p_column, bool p_selected) {
}

// TODO: menu item
void FileSystemTree::_build_empty_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW);
	context_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
	FileContextMenu *new_menu = memnew(FileContextMenu);
	new_menu->set_file_system(context_menu->get_file_system());
	new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
	new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
	new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
	new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("File")));
	context_menu->set_item_submenu_node(-1, new_menu);
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
}

void FileSystemTree::_build_file_menu() {
	context_menu->clear();

	// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);

	// context_menu->add_separator();

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

void FileSystemTree::_build_folder_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB);
	// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY_PATH);

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

void FileSystemTree::_on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button) {
	ERR_FAIL_NULL(context_menu);

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	Vector<String> targets;
	TreeItem *cursor_item = get_selected();
	if (!cursor_item) {
		return;
	} else {
		Dictionary d = cursor_item->get_metadata(0);
		targets.push_back(d["path"]);
	}

	TreeItem *selected = get_root();
	selected = get_next_selected(selected);
	while (selected) {
		if (selected != cursor_item && selected->is_visible_in_tree()) {
			Dictionary d = selected->get_metadata(0);
			targets.push_back(d["path"]);
		}
		selected = get_next_selected(selected);
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

	if (context_menu->get_item_count() > 0) {
		context_menu->set_position(get_screen_position() + p_pos);
		context_menu->reset_size();
		context_menu->popup();
	}
}

void FileSystemTree::_on_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	ERR_FAIL_NULL(context_menu);

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	deselect_all();

	TreeItem *root_item = get_root();
	ERR_FAIL_NULL(root_item);

	String path = root_item->get_metadata(0);
	if (path.is_empty()) {
		return;
	}

	Vector<String> targets;
	targets.push_back(path);
	context_menu->set_targets(targets);

	_build_empty_menu();

	if (context_menu->get_item_count() > 0) {
		context_menu->set_position(get_screen_position() + p_pos);
		context_menu->reset_size();
		context_menu->popup();
	}
}

void FileSystemTree::_on_item_edited() {
	ERR_FAIL_NULL(context_menu);

	TreeItem *ti = get_edited();
	int col_index = get_edited_column();

	Dictionary d = ti->get_metadata(0);
	String from = d["path"];
	String new_name = ti->get_text(col_index).strip_edges();
	if (_rename_operation_confirm(from, new_name)) {
		String path = from.get_base_dir();
		to_select = path.path_join(new_name);

		context_menu->get_file_system()->scan(path, true);
	} else {
		ti->set_text(0, from);
	}
}

bool FileSystemTree::_rename_operation_confirm(const String &p_from, const String &p_new_name) {
	bool rename_error = false;

	if (p_new_name.length() == 0) {
		// TODO: hint
		// print_line(TTRC("No name provided."));
		rename_error = true;
	} else if (p_new_name.contains_char('/') || p_new_name.contains_char('\\') || p_new_name.contains_char(':')) {
		// print_line(TTRC("Name contains invalid characters."));
		rename_error = true;
	} else if (p_new_name[0] == '.') {
		// print_line(TTRC("This filename begins with a dot rendering the file invisible to the editor.\nIf you want to rename it anyway, use your operating system's file manager."));
		rename_error = true;
	}

	if (rename_error) {
		return false;
	}

	String from = p_from;
	String to = from.get_base_dir().path_join(p_new_name);
	if (to == from) {
		return false;
	}
	return FileSystemAccess::rename(p_from, to) == OK;
}

void FileSystemTree::_on_draw() {
	if (rename_item) {
		TreeItem *item = get_selected();
		if (item && edit_selected(true)) { // TODO: Note select_mode
			Dictionary d = item->get_metadata(0);
			String path = d["path"];
			String name = path.get_file();
			if (FileSystemAccess::file_exists(path)) {
				set_editor_selection(0, name.rfind_char('.'));
			} else {
				set_editor_selection(0, name.length());
			}
		}
		rename_item = false;
	}
}

bool FileSystemTree::_process_id_pressed(int p_option, const Vector<String> &p_selected) {
	bool is_handled = true;

	switch (p_option) {
		case FileContextMenu::FILE_MENU_OPEN: {
			// TODO: item activated
			// String path = p_selected.is_empty() ? "" : p_selected[0];
			// if (!FileSystemAccess::dir_exists(path)) {
			// 	break;
			// }

			// set_path(path);
		} break;

		case FileContextMenu::FILE_MENU_RENAME: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (path.is_empty()) {
				break;
			} else if (FileSystemAccess::dir_exists(path)) {
				bool result = edit_selected(true); // TODO: Note select_mode
				String name = path.get_file();
				set_editor_selection(0, name.length());
			} else if (FileSystemAccess::file_exists(path)) {
				bool result = edit_selected(true); // TODO: Note select_mode
				String name = path.get_file();
				set_editor_selection(0, name.rfind_char('.'));
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_FOLDER: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new folder");
			String name = new_name;
			String path = dir.path_join(name);
			int i = 1;
			while (FileSystemAccess::dir_exists(path)) {
				name = vformat("%s (%d)", new_name, i);
				path = dir.path_join(name);
				i++;
			}
			Error err = FileSystemAccess::make_dir(path);
			if (err == OK) {
				to_select = path;
				rename_item = true;

				context_menu->get_file_system()->scan(dir, true);
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_TEXTFILE: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new file");
			String name = new_name + ".txt";
			String path = dir.path_join(name);
			int i = 1;
			while (FileSystemAccess::file_exists(path)) {
				name = vformat("%s (%d).txt", new_name, i);
				path = dir.path_join(name);
				i++;
			}
			Error err = FileSystemAccess::create_file(dir, name);
			if (err == OK) {
				to_select = path;
				rename_item = true;

				context_menu->get_file_system()->scan(dir, true);
			}
		} break;

		default: {
			is_handled = false;
		}
	}

	return is_handled;
}

void FileSystemTree::_context_menu_id_pressed(int p_option) {
	_process_id_pressed(p_option, context_menu->get_targets());
}

void FileSystemTree::set_display_mode(DisplayMode p_display_mode) {
	if (display_mode == p_display_mode) {
		return;
	}

	display_mode = p_display_mode;
	_update_display_mode();
}

void FileSystemTree::set_column_settings(const Array &p_column_settings) {
}

Array FileSystemTree::get_column_settings() const {
	return Array();
}

void FileSystemTree::set_context_menu(FileContextMenu *p_menu) {
	if (context_menu == p_menu) {
		return;
	}

	if (context_menu && context_menu != p_menu) {
		context_menu->disconnect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));
	}
	context_menu = p_menu;

	print_line("set menu: ", context_menu);
	if (context_menu) {
		context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

		if (!has_connections(SceneStringName(draw))) {
			// CanvasItem::_redraw_callback()
			connect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (!has_connections("item_edited")) {
			connect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (!has_connections("item_mouse_selected")) {
			connect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected)); // multi_selected already contains left mouse button select
		}
		if (!has_connections("empty_clicked")) {
			connect("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked));
		}
	} else {
		if (has_connections(SceneStringName(draw))) {
			disconnect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (has_connections("item_edited")) {
			disconnect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (has_connections("item_mouse_selected")) {
			disconnect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected)); // multi_selected already contains left mouse button select
		}
		if (has_connections("empty_clicked")) {
			disconnect("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked));
		}
	}
}

FileContextMenu *FileSystemTree::get_context_menu() const {
	return context_menu;
}

void FileSystemTree::process_menu_id(int p_option, const Vector<String> &p_selected) {
	ERR_FAIL_NULL(context_menu);

	if (!_process_id_pressed(p_option, p_selected)) {
		context_menu->set_targets(p_selected);
		context_menu->file_option(p_option);
	}
}

TreeItem *FileSystemTree::add_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	TreeItem *item = nullptr;
	if (display_mode == DISPLAY_MODE_TREE) {
		item = _add_tree_item(p_fi, p_parent, p_index);
		// TODO: to_select
	} else if (display_mode == DISPLAY_MODE_LIST) {
		item = _add_list_item(p_fi, p_parent, p_index);
		if (item && !to_select.is_empty() && to_select == p_fi.path) {
			// item->select_row();	// TODO
			item->select(0);
			item->set_as_cursor(0);
			to_select = "";
		}
	}

	return item;
}

void FileSystemTree::_bind_methods() {
	ADD_SIGNAL(MethodInfo("row_selected", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "TreeItem"), PropertyInfo(Variant::BOOL, "selected")));

	// ADD_SIGNAL(MethodInfo("multi_selected", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "TreeItem"), PropertyInfo(Variant::INT, "column"), PropertyInfo(Variant::BOOL, "selected")));
	// ADD_SIGNAL(MethodInfo("item_mouse_selected", PropertyInfo(Variant::VECTOR2, "mouse_position"), PropertyInfo(Variant::INT, "mouse_button_index")));
	// ADD_SIGNAL(MethodInfo("empty_clicked", PropertyInfo(Variant::VECTOR2, "click_position"), PropertyInfo(Variant::INT, "mouse_button_index")));
	// ADD_SIGNAL(MethodInfo("item_activated"));
}

FileSystemTree::FileSystemTree() {
	for (int i = 0; i < default_column_count; i++) {
		column_settings[i] = default_column_settings[i];
	}

	// connect("item_activated", callable_mp(this, &FileSystemTree::_on_item_activated));
	// connect("multi_selected", callable_mp(this, &FileSystemTree::_on_multi_selected));

	callable_mp(this, &FileSystemTree::_update_display_mode).call_deferred();
}

FileSystemTree::~FileSystemTree() {
}
