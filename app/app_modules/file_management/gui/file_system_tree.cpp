#include "file_system_tree.h"

#include "app/app_modules/file_management/gui/file_context_menu.h"

// #include "app/app_string_names.h"
#include "app/gui/app_control.h"
// #include "app/themes/app_scale.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

// #define DRAG_THRESHOLD (8 * EDSCALE)

static const int default_column_count = 5;
static FileSystemTree::ColumnSetting default_column_settings[default_column_count] = {
	{ FileSystemTree::COLUMN_TYPE_NAME, 0, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 3 },
	{ FileSystemTree::COLUMN_TYPE_MODIFIED, 1, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
	{ FileSystemTree::COLUMN_TYPE_CREATED, 2, false, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
	{ FileSystemTree::COLUMN_TYPE_TYPE, 3, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
	{ FileSystemTree::COLUMN_TYPE_SIZE, 4, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
};

void FileSystemTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			// _draw_selection();

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
			// case NOTIFICATION_TRANSLATION_CHANGED:
			// case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
			// case NOTIFICATION_THEME_CHANGED: {
			// 	_update_display_mode();
			// } break;
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

	ERR_FAIL_COND(column_count > column_settings.size());

	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size() && column < column_count; i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		String title = "";
		switch (setting.type) {
			case COLUMN_TYPE_NAME:
				title = RTR("Name");
				break;
			case COLUMN_TYPE_MODIFIED:
				title = RTR("Modified");
				break;
			case COLUMN_TYPE_CREATED:
				title = RTR("Created");
				break;
			case COLUMN_TYPE_TYPE:
				title = RTR("Type");
				break;
			case COLUMN_TYPE_SIZE:
				title = RTR("Size");
				break;
		}

		set_column_title(column, title);
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

	bool is_dir = FileSystemAccess::is_dir_type(p_fi);
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

	item->set_editable(0, true);
	item->set_selectable(0, true);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = path;
	d["is_dir"] = is_dir; // TODO: Remove, use type
	d["type"] = p_fi.type;
	item->set_metadata(0, d);

	return item;
}

TreeItem *FileSystemTree::_add_list_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = FileSystemAccess::is_dir_type(p_fi);
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

		switch (setting.type) {
			case COLUMN_TYPE_NAME: {
				item->set_text(column, p_fi.name);
				item->set_icon(column, icon);
				item->set_editable(column, true);
			} break;
			case COLUMN_TYPE_MODIFIED: {
				String modified_time = "";
				if (p_fi.modified_time > 0) {
					modified_time = FileSystem::parse_time(p_fi.modified_time);
				}
				item->set_text(column, modified_time);
				item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_CREATED: {
				String creation_time = "";
				if (p_fi.creation_time > 0) {
					creation_time = FileSystem::parse_time(p_fi.creation_time);
				}
				item->set_text(column, creation_time);
				item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_TYPE: {
				String type = p_fi.type;
				if (FileSystemAccess::is_root_type(p_fi)) {
					type = "";
				} else if (FileSystemAccess::is_drive_type(p_fi)) {
					type = ""; // RTR("Disk"); // TODO: Distinguish local disk and network drive.
				} else if (FileSystemAccess::is_dir_type(p_fi)) {
					type = RTR("Folder");
				} else {
					type = type.to_upper();
				}
				item->set_text(column, type);
				item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_SIZE: {
				String size = "";
				if (!FileSystemAccess::is_dir_type(p_fi)) {
					size = FileSystem::parse_size(p_fi.size);
				}
				item->set_text(column, size);
				item->set_text_alignment(column, HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
				item->set_editable(column, false);
			} break;
		}
		// item->set_selectable(column, true);

		column++;
	}

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = is_dir; // TODO: Remove, use type
	d["type"] = p_fi.type;
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

	if (display_mode == DISPLAY_MODE_TREE) {
		// TODO
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_EXPAND_TO_CURRENT);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

		context_menu->add_separator();

		context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW);
		// context_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
		FileContextMenu *new_menu = memnew(FileContextMenu);
		new_menu->set_file_system(context_menu->get_file_system());
		new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
		// new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
		new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		// new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("File")));
		context_menu->set_item_submenu_node(-1, new_menu);
		new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

		context_menu->add_separator();

		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
	}
}

void FileSystemTree::_build_file_menu() {
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

void FileSystemTree::_build_folder_menu() {
	context_menu->clear();

	if (display_mode == DISPLAY_MODE_TREE) {
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB); // TODO: create new tab in central area
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

		// context_menu->add_separator();

		// context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
		context_menu->add_item(RTR("New TextFile..."), FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		context_menu->add_item(RTR("New Folder..."), FileContextMenu::FILE_MENU_NEW_FOLDER);

		context_menu->add_separator();
	} else if (display_mode == DISPLAY_MODE_LIST) {
		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

		context_menu->add_separator();
	}

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

void FileSystemTree::_build_selected_items_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

	context_menu->add_separator();

	// context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void FileSystemTree::_on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button) {
	ERR_FAIL_NULL(context_menu);

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *cursor_item = get_selected();
	if (!cursor_item) {
		return;
	}

	Vector<String> targets = get_selected_paths();
	context_menu->set_targets(targets);

	if (targets.size() > 1) {
		_build_selected_items_menu();
	} else {
		Dictionary d = cursor_item->get_metadata(0);
		String type = d["type"];
		if (FileSystemAccess::is_root_type(type)) {
			return;
		} else if (FileSystemAccess::is_drive_type(type)) {
			return;
		} else if (FileSystemAccess::is_dir_type(type)) {
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

	deselect_all();

	// TODO
	if (display_mode == DISPLAY_MODE_TREE) {
		return;
	}

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *root_item = get_root();
	ERR_FAIL_NULL(root_item);

	String path = root_item->get_metadata(0);
	if (path.is_empty() || path == COMPUTER_PATH) { // TODO: use FileSystemAccess::is_root_type(type)
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
	ERR_FAIL_COND(col_index < 0);

	Dictionary d = ti->get_metadata(0);
	String from = d["path"];
	String new_name = ti->get_text(col_index).strip_edges();
	if (_rename_operation_confirm(from, new_name)) {
		// deselect_all(); // TODO
		String path = from.get_base_dir();
		to_select = path.path_join(new_name);

		context_menu->get_file_system()->scan(path, true);
	} else {
		ti->set_text(0, d["name"]);
	}
}

bool FileSystemTree::_rename_operation_confirm(const String &p_from, const String &p_new_name) {
	bool rename_error = false;

	if (p_new_name.length() == 0) {
		// TODO: hint
		// print_line(RTR("No name provided."));
		rename_error = true;
	} else if (p_new_name.contains_char('/') || p_new_name.contains_char('\\') || p_new_name.contains_char(':')) {
		// print_line(RTR("Name contains invalid characters."));
		rename_error = true;
	} else if (p_new_name[0] == '.') {
		// print_line(RTR("This filename begins with a dot rendering the file invisible to the app.\nIf you want to rename it anyway, use your operating system's file manager."));
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
		if (item) { // TODO: Note select_mode
			item->select(0);
			if (edit_selected()) {
				Dictionary d = item->get_metadata(0);
				String path = d["path"];
				String name = path.get_file();
				if (FileSystemAccess::file_exists(path)) {
					set_editor_selection(0, name.rfind_char('.'));
				} else {
					set_editor_selection(0, name.length());
				}
			}
		}
		rename_item = false;
	}
}

bool FileSystemTree::_process_id_pressed(int p_option, const Vector<String> &p_selected) {
	bool is_handled = true;

	switch (p_option) {
		case FileContextMenu::FILE_MENU_OPEN: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}

			// TODO: Note the display_mode
			emit_signal(SNAME("item_activated"));
		} break;

		case FileContextMenu::FILE_MENU_PASTE: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (FileSystemAccess::file_exists(path)) {
				path = path.get_base_dir();
			}

			if (!FileSystemAccess::dir_exists(path)) {
				break;
			}

			bool ret = false;
			bool is_cut = false;
			Vector<String> clipboard_paths;
			ret = FileSystemAccess::get_clipboard_paths(clipboard_paths, is_cut);
			if (!ret || clipboard_paths.is_empty()) {
				break;
			}

			Vector<String> dest_paths;
			ret = FileSystemAccess::paste(path, dest_paths);
			// print_line("paste ret: ", ret, is_cut, dest_paths);
			if (ret && context_menu->get_file_system()) {
				context_menu->get_file_system()->scan(path, true);

				if (is_cut) {
					// Update the source directory for cut operations.
					Vector<String> dirs;
					for (const String &clipboard_path : clipboard_paths) {
						String base_dir = clipboard_path.get_base_dir();
						if (!dirs.has(base_dir)) {
							dirs.push_back(base_dir);
						}
					}
					// print_line("dirs: ", dirs.size(), dirs);
					context_menu->get_file_system()->scan(dirs, true);
				}

				if (!dest_paths.is_empty()) {
					// deselect_all(); // TODO
					// TODO: selected list
					to_select = dest_paths[0];
				}
			}
		} break;

		case FileContextMenu::FILE_MENU_RENAME: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (path.is_empty()) {
				break;
			} else if (FileSystemAccess::dir_exists(path)) {
				TreeItem *item = get_selected();
				if (item) {
					item->select(0);
					grab_focus(!has_focus(true));
					bool result = edit_selected(); // TODO: Note select_mode
					String name = path.get_file();
					set_editor_selection(0, name.length());
				}
			} else if (FileSystemAccess::file_exists(path)) {
				TreeItem *item = get_selected();
				if (item) {
					item->select(0);
					grab_focus(!has_focus(true));
					bool result = edit_selected(); // TODO: Note select_mode
					String name = path.get_file();
					set_editor_selection(0, name.rfind_char('.'));
				}
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_FOLDER: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new folder");
			// TODO
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
				// deselect_all(); // TODO
				to_select = path;
				rename_item = true;

				context_menu->get_file_system()->scan(dir, true);
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_TEXTFILE: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			// print_line("new text dir: ", dir);
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new file");
			String name = new_name + ".txt";
			String path = dir.path_join(name);
			// print_line("path: ", new_name, path, FileSystemAccess::file_exists(path));
			int i = 1;
			while (FileSystemAccess::file_exists(path)) {
				name = vformat("%s (%d).txt", new_name, i);
				path = dir.path_join(name);
				// print_line("i: ", i, name, path);
				i++;
			}
			Error err = FileSystemAccess::create_file(dir, name);
			// print_line("create file: ", err);
			if (err == OK) {
				// deselect_all(); // TODO
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

// void FileSystemTree::_draw_selection() {
// 	print_line("draw: ", drag_type, drag_from, box_selecting_to);
// 	if (drag_type == DRAG_BOX_SELECTION) {
// 		// Draw the dragging box
// 		Point2 bsfrom = drag_from;
// 		Point2 bsto = box_selecting_to;

// 		draw_rect(
// 				Rect2(bsfrom, bsto - bsfrom),
// 				get_theme_color(SNAME("box_selection_fill_color"), AppStringName(App)));

// 		draw_rect(
// 				Rect2(bsfrom, bsto - bsfrom),
// 				get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
// 				false,
// 				Math::round(EDSCALE));
// 	}
// }

Vector<TreeItem *> FileSystemTree::_get_selected_items() {
	Vector<TreeItem *> items;

	TreeItem *cursor_item = get_selected();
	if (cursor_item) {
		items.push_back(cursor_item);
	}

	TreeItem *selected = get_root();
	selected = get_next_selected(selected);
	while (selected) {
		if (selected != cursor_item && selected->is_visible_in_tree()) {
			items.push_back(selected);
		}
		selected = get_next_selected(selected);
	}

	return items;
}

// bool FileSystemTree::_gui_input_select(const Ref<InputEvent> &p_event) {
// 	ERR_FAIL_COND_V(display_mode != DISPLAY_MODE_LIST, false);

// 	Ref<InputEventMouseButton> b = p_event;
// 	Ref<InputEventMouseMotion> m = p_event;
// 	Ref<InputEventKey> k = p_event;

// 	// if (drag_type == DRAG_NONE || (drag_type == DRAG_BOX_SELECTION && b.is_valid() && !b->is_pressed())) {
// 	// 	Point2 click;
// 	// 	bool can_select = b.is_valid() && b->get_button_index() == MouseButton::LEFT;
// 	// 	if (can_select) {
// 	// 		click = b->get_position();
// 	// 		// Allow selecting on release when performed very small box selection (necessary when Shift is pressed, see below).
// 	// 		can_select = b->is_pressed() || (drag_type == DRAG_BOX_SELECTION && click.distance_to(drag_from) <= DRAG_THRESHOLD);
// 	// 	}
// 	// 	if (can_select) {
// 	// 		TreeItem *item = get_item_at_position(click);

// 	// 		if (b->is_pressed()) {
// 	// 			// Shift or Ctrl also allows forcing box selection when item was clicked.
// 	// 			if (item == nullptr || ((b->is_shift_pressed() || b->is_command_or_control_pressed()))) {
// 	// 				// Start a box selection.
// 	// 				if (!(b->is_shift_pressed() || b->is_command_or_control_pressed())) {
// 	// 					// Clear the selection if not additive.
// 	// 					deselect_all();
// 	// 					queue_redraw();
// 	// 				};
// 	// 				// for (TreeItem *item : _get_selected_items()) {
// 	// 				// 	selected_list.push_back(item);
// 	// 				// }

// 	// 				drag_from = click;
// 	// 				drag_type = DRAG_BOX_SELECTION;
// 	// 				box_selecting_to = drag_from;
// 	// 				// prev_selecting_to = box_selecting_to;
// 	// 				prev_hovered_item = nullptr;
// 	// 				return true;
// 	// 			}
// 	// 		} else {
// 	// 			drag_type = DRAG_NONE;
// 	// 			// Select the item.
// 	// 			// if (item) {
// 	// 			// 	item->select(0);
// 	// 			// }
// 	// 			// return true;
// 	// 			return false; // Handle it outside?
// 	// 		}
// 	// 	}
// 	// }

// 	if (drag_type == DRAG_NONE) {
// 		if (b.is_valid() && b->get_button_index() == MouseButton::LEFT) {
// 			print_line("b: ", b->is_pressed());
// 			if (b->is_pressed()) {
// 				drag_from = b->get_position();
// 				detecting_box_selection = true;
// 			} else {
// 				detecting_box_selection = false;
// 			}
// 			return false; // Handle it outside
// 		}

// 		if (detecting_box_selection && m.is_valid()) {
// 			Point2 click = m->get_position();
// 			print_line("click: ", click, drag_from, click.distance_to(drag_from), DRAG_THRESHOLD);
// 			if (click.distance_to(drag_from) > DRAG_THRESHOLD) {
// 				// Start a box selection.
// 				if (!(m->is_shift_pressed() || m->is_command_or_control_pressed())) {
// 					// Clear the selection if not additive.
// 					deselect_all();
// 					queue_redraw();
// 				};

// 				// drag_from = click;
// 				drag_type = DRAG_BOX_SELECTION;
// 				box_selecting_to = drag_from;
// 				// prev_selecting_to = box_selecting_to;
// 				prev_hovered_item = nullptr;
// 				starting_item = get_item_at_position(drag_from);
// 				return true;
// 			}
// 		}
// 	} else if (drag_type == DRAG_BOX_SELECTION) {
// 		// End box selection.
// 		if (b.is_valid() && !b->is_pressed() && b->get_button_index() == MouseButton::LEFT) {
// 			drag_type = DRAG_NONE;
// 			detecting_box_selection = false;
// 			queue_redraw();
// 			return true;
// 		}

// 		// Cancel box selection.	// TODO: context menu?
// 		if (b.is_valid() && b->is_pressed() && b->get_button_index() == MouseButton::RIGHT) {
// 			drag_type = DRAG_NONE;
// 			detecting_box_selection = false;
// 			queue_redraw();
// 			return true;
// 		}

// 		// Update box selection.
// 		if (m.is_valid()) {
// 			Point2 selecting_to = m->get_position();

// 			// bool move_up = selecting_to.y < drag_from.y;
// 			// TreeItem *hovered_item = get_item_at_position(selecting_to);

// 			// // if (!starting_item && hovered_item) {
// 			// // 	starting_item = hovered_item;
// 			// // }

// 			// // TreeItem *item = starting_item;
// 			// TreeItem *item = get_root();
// 			// while (item) {
// 			// 	Rect2 rect = get_item_rect(item);

// 			// 	// if (!item->is_selected(0) || (m->is_shift_pressed() || m->is_command_or_control_pressed())) {
// 			// 	// 	item->select(0);
// 			// 	// 	// } else {
// 			// 	// 	// 	item->deselect(0);
// 			// 	// }

// 			// 	if (move_up) {
// 			// 		item = item->get_prev();
// 			// 	} else {
// 			// 		item = item->get_next();
// 			// 	}
// 			// }

// 			// // print_line("item: ", selecting_to, hovered_item, prev_hovered_item);
// 			// // if (hovered_item && hovered_item != prev_hovered_item) {
// 			// // 	// TreeItem *target_item = nullptr;
// 			// // 	// if (prev_hovered_item) {
// 			// // 	// 	if (move_up) {
// 			// // 	// 		target_item = prev_hovered_item->get_prev();
// 			// // 	// 	} else {
// 			// // 	// 		target_item = prev_hovered_item->get_next();
// 			// // 	// 	}
// 			// // 	// }

// 			// // 	if (!hovered_item->is_selected(0) || (m->is_shift_pressed() || m->is_command_or_control_pressed())) {
// 			// // 		hovered_item->select(0);
// 			// // 	} else {
// 			// // 		hovered_item->deselect(0);
// 			// // 	}
// 			// // 	prev_hovered_item = hovered_item;
// 			// // }

// 			// prev_selecting_to = box_selecting_to;
// 			box_selecting_to = selecting_to;

// 			// Rect2 box = Rect2(drag_from, box_selecting_to - drag_from);

// 			queue_redraw();
// 			return true;
// 		}
// 	}

// 	return false;
// }

void FileSystemTree::gui_input(const Ref<InputEvent> &p_event) {
	if (display_mode == DISPLAY_MODE_TREE) {
		Tree::gui_input(p_event);
		return;
	}

	// bool accepted = true;

	// if (_gui_input_select(p_event)) {
	// 	// print_line("Selection");
	// } else {
	// 	// print_line("Not accepted");
	// 	accepted = false;
	// }

	// if (accepted) {
	// 	accept_event();
	// } else {
	Tree::gui_input(p_event);
	// }
}

void FileSystemTree::set_display_mode(DisplayMode p_display_mode) {
	if (display_mode == p_display_mode) {
		return;
	}

	display_mode = p_display_mode;
	_update_display_mode();
}

FileSystemTree::DisplayMode FileSystemTree::get_display_mode() const {
	return display_mode;
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
	if (context_menu) {
		context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

		if (!is_connected(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw))) {
			// CanvasItem::_redraw_callback()
			connect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (!is_connected("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited))) {
			connect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (!is_connected("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected))) {
			connect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected)); // multi_selected already contains left mouse button select
		}
		if (!is_connected("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked))) {
			connect("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked));
		}
	} else {
		if (is_connected(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw))) {
			disconnect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (is_connected("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited))) {
			disconnect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (is_connected("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected))) {
			disconnect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected));
		}
		if (is_connected("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked))) {
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

Vector<String> FileSystemTree::get_selected_paths() {
	Vector<String> selected_paths;

	for (const TreeItem *item : _get_selected_items()) {
		Dictionary d = item->get_metadata(0);
		selected_paths.push_back(d["path"]);
	}

	return selected_paths;
}

Vector<String> FileSystemTree::get_uncollapsed_paths() const {
	Vector<String> paths;
	TreeItem *root_item = get_root();
	if (root_item) {
		// BFS to find all uncollapsed paths of the file system directory.
		TreeItem *file_system_subtree = root_item->get_first_child();
		if (file_system_subtree) {
			List<TreeItem *> queue;
			queue.push_back(file_system_subtree);

			while (!queue.is_empty()) {
				TreeItem *ti = queue.back()->get();
				queue.pop_back();
				// if (!ti->is_collapsed() && ti->get_child_count() > 0) {
				if (!ti->is_collapsed()) {
					Dictionary d = ti->get_metadata(0);
					if (d["is_dir"]) {
						paths.push_back(d["path"]);
					}
				}
				for (int i = 0; i < ti->get_child_count(); i++) {
					queue.push_back(ti->get_child(i));
				}
			}
		}
	}
	return paths;
}

TreeItem *FileSystemTree::add_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	TreeItem *item = nullptr;
	if (display_mode == DISPLAY_MODE_TREE) {
		item = _add_tree_item(p_fi, p_parent, p_index);
		if (item && !to_select.is_empty() && to_select == p_fi.path) {
			TreeItem *root_item = get_root();
			TreeItem *parent = item->get_parent();
			while (parent && parent != root_item) {
				if (parent->is_collapsed()) {
					parent->set_collapsed(false);

					// Dictionary d = parent->get_metadata(0);
					// print_line("set uncollapsed: ", d["path"]);
				}
				parent = parent->get_parent();
			}
			item->select(0);
			// item->set_as_cursor(0);
			to_select = "";
		}
	} else if (display_mode == DISPLAY_MODE_LIST) {
		item = _add_list_item(p_fi, p_parent, p_index);
		if (item && !to_select.is_empty() && to_select == p_fi.path) {
			// item->select_row();	// TODO
			item->select(0);
			// item->set_as_cursor(0);
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
