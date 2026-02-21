#include "file_pane.h"

#include "app/app_modules/file_management/gui/address_bar.h"
#include "app/app_modules/file_management/gui/file_context_menu.h"
#include "app/app_modules/file_management/gui/file_system_tree.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/popup_menu.h"

#include "app/gui/app_control.h"
#include "app/settings/app_settings.h"
#include "app/themes/app_scale.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

Control *FileSystemItemList::make_custom_tooltip(const String &p_text) const {
	int idx = get_item_at_position(get_local_mouse_position());
	if (idx == -1) {
		return nullptr;
	}
	// TODO
	return nullptr; // FileSystemDock::get_singleton()->create_tooltip_for_path(get_item_metadata(idx));
}

void FileSystemItemList::_line_editor_submit(const String &p_text) {
	if (popup_edit_committed) {
		return; // Already processed by _text_editor_popup_modal_close
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	popup_edit_committed = true; // End edit popup processing.
	popup_editor->hide();

	emit_signal(SNAME("item_edited"));
	queue_redraw();
}

bool FileSystemItemList::edit_selected() {
	ERR_FAIL_COND_V_MSG(!is_anything_selected(), false, "No item selected.");
	int s = get_current();
	ERR_FAIL_COND_V_MSG(s < 0, false, "No current item selected.");
	ensure_current_is_visible();

	Rect2 rect;
	Rect2 popup_rect;
	Vector2 ofs;

	Vector2 icon_size = get_fixed_icon_size() * get_icon_scale();

	// Handles the different icon modes (TOP/LEFT).
	switch (get_icon_mode()) {
		case ItemList::ICON_MODE_LEFT:
			rect = get_item_rect(s, true);
			if (get_v_scroll_bar()->is_visible()) {
				rect.position.y -= get_v_scroll_bar()->get_value();
			}
			if (get_h_scroll_bar()->is_visible()) {
				rect.position.x -= get_h_scroll_bar()->get_value();
			}
			ofs = Vector2(0, Math::floor((MAX(line_editor->get_minimum_size().height, rect.size.height) - rect.size.height) / 2));
			popup_rect.position = rect.position - ofs;
			popup_rect.size = rect.size;

			// Adjust for icon position and size.
			popup_rect.size.x -= MAX(theme_cache.h_separation, 0) / 2 + icon_size.x;
			popup_rect.position.x += MAX(theme_cache.h_separation, 0) / 2 + icon_size.x;
			break;
		case ItemList::ICON_MODE_TOP:
			rect = get_item_rect(s, false);
			if (get_v_scroll_bar()->is_visible()) {
				rect.position.y -= get_v_scroll_bar()->get_value();
			}
			if (get_h_scroll_bar()->is_visible()) {
				rect.position.x -= get_h_scroll_bar()->get_value();
			}
			popup_rect.position = rect.position;
			popup_rect.size = rect.size;

			// Adjust for icon position and size.
			popup_rect.size.y -= MAX(theme_cache.v_separation, 0) / 2 + theme_cache.icon_margin + icon_size.y;
			popup_rect.position.y += MAX(theme_cache.v_separation, 0) / 2 + theme_cache.icon_margin + icon_size.y;
			break;
	}
	if (is_layout_rtl()) {
		popup_rect.position.x = get_size().width - popup_rect.position.x - popup_rect.size.x;
	}
	popup_rect.position += get_screen_position();

	popup_editor->set_position(popup_rect.position);
	popup_editor->set_size(popup_rect.size);

	String name = get_item_text(s);
	line_editor->set_text(name);
	line_editor->select(0, name.rfind_char('.'));

	popup_edit_committed = false; // Start edit popup processing.
	popup_editor->popup();
	popup_editor->child_controls_changed();
	line_editor->grab_focus();
	return true;
}

String FileSystemItemList::get_edit_text() {
	return line_editor->get_text();
}

void FileSystemItemList::_text_editor_popup_modal_close() {
	if (popup_edit_committed) {
		return; // Already processed by _text_editor_popup_modal_close
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	_line_editor_submit(line_editor->get_text());
}

void FileSystemItemList::_bind_methods() {
	ADD_SIGNAL(MethodInfo("item_edited"));
}

FileSystemItemList::FileSystemItemList() {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);

	popup_editor = memnew(Popup);
	add_child(popup_editor);

	popup_editor_vb = memnew(VBoxContainer);
	popup_editor_vb->add_theme_constant_override("separation", 0);
	popup_editor_vb->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	popup_editor->add_child(popup_editor_vb);

	line_editor = memnew(LineEdit);
	line_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	popup_editor_vb->add_child(line_editor);
	line_editor->connect(SceneStringName(text_submitted), callable_mp(this, &FileSystemItemList::_line_editor_submit));
	popup_editor->connect("popup_hide", callable_mp(this, &FileSystemItemList::_text_editor_popup_modal_close));
}

String FilePane::_get_pane_title() const {
	return "Folder";
}

Ref<Texture2D> FilePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("Folder"));
}

void FilePane::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if ((k.is_valid() && k->is_pressed() && !k->is_echo()) || Object::cast_to<InputEventShortcut>(*p_event)) {
		bool is_handled = true;
		int option_id = -1;
		bool add_selected = false;
		Vector<String> targets;

		if (APP_IS_SHORTCUT("file_management/copy_path", p_event)) {
			option_id = FileContextMenu::FILE_MENU_COPY_PATH;
			add_selected = true;

		} else if (APP_IS_SHORTCUT("file_management/show_in_explorer", p_event)) {
			option_id = FileContextMenu::FILE_MENU_SHOW_IN_EXPLORER;
			add_selected = true;
		} else if (APP_IS_SHORTCUT("file_management/open_in_external_program", p_event)) {
			option_id = FileContextMenu::FILE_MENU_OPEN_EXTERNAL;
			add_selected = true;
		} else if (APP_IS_SHORTCUT("file_management/open_in_terminal", p_event)) {
			option_id = FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL;
			add_selected = true;

		} else if (APP_IS_SHORTCUT("file_management/undo", p_event)) {
			option_id = FileContextMenu::FILE_MENU_UNDO;
		} else if (APP_IS_SHORTCUT("file_management/redo", p_event)) {
			option_id = FileContextMenu::FILE_MENU_REDO;

		} else if (APP_IS_SHORTCUT("file_management/new_folder", p_event)) {
			option_id = FileContextMenu::FILE_MENU_NEW_FOLDER;
			targets.push_back(current_path);
		} else if (APP_IS_SHORTCUT("file_management/new_textfile", p_event)) {
			option_id = FileContextMenu::FILE_MENU_NEW_TEXTFILE;
			targets.push_back(current_path);

		} else if (APP_IS_SHORTCUT("file_management/cut", p_event)) {
			option_id = FileContextMenu::FILE_MENU_CUT;
			add_selected = true;
		} else if (APP_IS_SHORTCUT("file_management/copy", p_event)) {
			option_id = FileContextMenu::FILE_MENU_COPY;
			add_selected = true;
		} else if (APP_IS_SHORTCUT("file_management/paste", p_event)) {
			option_id = FileContextMenu::FILE_MENU_PASTE;
			targets.push_back(current_path);

		} else if (APP_IS_SHORTCUT("file_management/rename", p_event)) {
			option_id = FileContextMenu::FILE_MENU_RENAME;
			add_selected = true;

		} else if (APP_IS_SHORTCUT("file_management/delete", p_event)) {
			option_id = FileContextMenu::FILE_MENU_REMOVE;
			add_selected = true;

		} else if (APP_IS_SHORTCUT("file_management/focus_path", p_event)) {
			address_bar->edit();

		} else {
			is_handled = false;
		}

		if (option_id >= 0 && add_selected) {
			if (item_list->is_anything_selected()) {
				Vector<int> selected_items = item_list->get_selected_items();
				for (int i : selected_items) {
					Dictionary d = item_list->get_item_metadata(i);
					targets.push_back(d["path"]);
				}
			} else {
				option_id = -1;
			}
		}

		if (option_id >= 0) {
			callable_mp(this, &FilePane::_process_shortcut_input).call_deferred(option_id, targets);
		}

		if (is_handled) {
			get_tree()->get_root()->set_input_as_handled();
		}
	}
}

void FilePane::_process_shortcut_input(int p_option, const Vector<String> &p_selected) {
	if (!_process_id_pressed(p_option, p_selected)) {
		context_menu->set_targets(p_selected);
		context_menu->file_option(p_option);
	}
}

void FilePane::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			_update_icons();
			_update_ui();
		} break;
	}
}

void FilePane::_update_icons() {
	Ref<Texture2D> parent_folder = get_app_theme_icon(SNAME("ArrowUp"));
	Ref<Texture2D> forward_folder = get_app_theme_icon(SNAME("Forward"));
	Ref<Texture2D> back_folder = get_app_theme_icon(SNAME("Back"));
	Ref<Texture2D> reload = get_app_theme_icon(SNAME("Reload"));

	// Update icons.
	if (is_layout_rtl()) {
		dir_prev->set_button_icon(forward_folder);
		dir_next->set_button_icon(back_folder);
	} else {
		dir_prev->set_button_icon(back_folder);
		dir_next->set_button_icon(forward_folder);
	}
	dir_up->set_button_icon(parent_folder);

	refresh->set_button_icon(reload);
}

void FilePane::_update_ui() {
	ERR_FAIL_NULL(file_system);

	// Return the parent directory if current_path does not exist.
	String path = current_path;
	while (!file_system->is_valid_dir_path(path)) {
		String base = path.get_base_dir();
		if (base == path) {
			if (!file_system->is_valid_dir_path(base)) {
				path = file_system->get_root()->get_path();
				break;
			}
		}
		path = base;
	}
	if (path != current_path) {
		print_line("update path");
		// TODO: hint
		set_path(path);
		return;
	}

	FileSystemDirectory *dir = file_system->get_dir(current_path);
	if (!dir) {
		file_system->scan(current_path);
		return;
	}

	_update_ui_nocheck(dir);
}

void FilePane::_update_ui_nocheck(FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(p_dir);

	set_title(p_dir->get_name());
	set_icon(p_dir->get_icon());

	dir_prev->set_disabled(local_history_pos <= 0);
	dir_next->set_disabled(local_history_pos == local_history.size() - 1);
	if (current_path != file_system->get_root()->get_path()) {
		dir_up->set_disabled(false);
	} else {
		dir_up->set_disabled(true);
	}

	address_bar->set_text(current_path);
	// TODO
	// for (local_history) histories->add_item(history);
	// histories->select(local_history_pos);

	// _update_item_list(p_dir);
	_update_files(p_dir);
}

void FilePane::_add_item(const FileInfo &p_fi, bool p_is_dir) {
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder"));
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File"));

		icon = p_is_dir ? folder : file;
	}
	int item_id = item_list->add_item(p_fi.name, icon);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = p_is_dir;
	item_list->set_item_metadata(item_id, d);

	if (!to_select.is_empty() && to_select == p_fi.path) {
		item_list->set_current(item_id);
		item_list->select(item_id);
		to_select = "";
	}
}

void FilePane::_update_item_list(FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(p_dir);

	item_list->clear();

	// list dirs
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		FileSystemDirectory *subdir = p_dir->get_subdir(i);
		_add_item(subdir->get_info(), true);
	}

	// list files
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		const FileInfo &fi = *(p_dir->get_file(i));
		_add_item(fi, false);
	}
}

void FilePane::_add_item(const FileInfo &p_fi) {
	bool is_dir = p_fi.type == FOLDER_TYPE;
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder"));
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File"));

		icon = is_dir ? folder : file;
	}

	TreeItem *root = tree->get_root();
	ERR_FAIL_NULL(root);

	TreeItem *item = tree->create_item(root);
	item->set_text(0, p_fi.name);
	// item->set_editable(0, true);
	item->set_icon(0, p_fi.icon);

	String modified_time = itos(p_fi.modified_time);
	item->set_text(1, modified_time);
	// item->set_selectable(1, true);

	// item->set_cell_mode(2, TreeItem::CELL_MODE_CHECK);
	// item->set_editable(2, true);
	item->set_text(2, p_fi.type);
	// item->set_checked(2, info.is_singleton);

	String size = itos(p_fi.size);
	item->set_text(3, size);
	// item->add_button(3, get_editor_theme_icon(SNAME("Load")), BUTTON_OPEN);
	// item->add_button(3, get_editor_theme_icon(SNAME("MoveUp")), BUTTON_MOVE_UP);
	// item->add_button(3, get_editor_theme_icon(SNAME("MoveDown")), BUTTON_MOVE_DOWN);
	// item->add_button(3, get_editor_theme_icon(SNAME("Remove")), BUTTON_DELETE);
	// item->set_selectable(3, false);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = is_dir;
	item->set_metadata(0, d);

	if (!to_select.is_empty() && to_select == p_fi.path) {
		// item->select_row();	// TODO
		item->select(0);
		item->set_as_cursor(0);
		to_select = "";
	}
}

void FilePane::_update_files(FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(p_dir);

	tree->clear();
	TreeItem *root = tree->create_item();

	// list dirs
	int dir_count = p_dir->get_subdir_count();
	for (int i = 0; i < dir_count; i++) {
		FileSystemDirectory *subdir = p_dir->get_subdir(i);
		_add_item(subdir->get_info());
	}

	// list files
	int file_count = p_dir->get_file_count();
	for (int i = 0; i < file_count; i++) {
		const FileInfo &fi = *(p_dir->get_file(i));
		_add_item(fi);
	}

	item_count->set_text(itos(dir_count + file_count));
}

// TODO: menu item
void FilePane::_build_empty_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

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
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FilePane::_context_menu_id_pressed));

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
}

void FilePane::_build_file_menu() {
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

void FilePane::_build_folder_menu() {
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

void FilePane::_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	item_list->deselect_all();

	Vector<String> targets;
	targets.push_back(current_path);
	context_menu->set_targets(targets);

	_build_empty_menu();

	// get_position() is the offset within the parent.
	context_menu->set_position(get_screen_position() + get_position() + p_pos);
	context_menu->reset_size();
	context_menu->popup();
}

void FilePane::_item_clicked(int p_item, const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	if (!item_list->is_anything_selected()) {
		return;
	}

	Vector<String> targets;
	Vector<int> selected_items = item_list->get_selected_items();
	for (int i : selected_items) {
		Dictionary d = item_list->get_item_metadata(i);
		targets.push_back(d["path"]);
	}
	context_menu->set_targets(targets);

	if (targets.size() > 1) {
		// TODO: handle multi selected
	} else {
		Dictionary d = item_list->get_item_metadata(p_item);
		if (d["is_dir"]) {
			_build_folder_menu();
		} else {
			_build_file_menu();
		}
	}

	// get_position() is the offset within the parent.
	context_menu->set_position(get_screen_position() + get_position() + p_pos);
	context_menu->reset_size();
	context_menu->popup();
}

// void FilePane::_multi_selected(int p_index, bool p_selected) {
// 	print_line("multi selected: ", p_index, p_selected);
// }

void FilePane::_item_dc_selected(int p_item) {
	int current = p_item;
	if (current < 0 || current >= item_list->get_item_count()) {
		return;
	}

	Dictionary d = item_list->get_item_metadata(current);

	if (d["is_dir"]) {
		set_path(d["path"]);
	} else {
		// TODO: signal

		// TODO: open_file()/run_file()
		const String file = d["path"];
		OS::get_singleton()->shell_open(file);
	}
}

void FilePane::_on_address_submitted(const String &p_path) {
	if (!file_system->is_valid_dir_path(p_path)) {
		return;
	}

	set_path(p_path);
}

void FilePane::_select_history(int p_idx) {
	local_history_pos = p_idx;
	_go_history();
}

void FilePane::_go_history() {
	ERR_FAIL_COND(local_history_pos < 0 || local_history_pos >= local_history.size());
	set_path(local_history[local_history_pos], false);
}

void FilePane::_go_back() {
	dir_prev->release_focus();

	if (local_history_pos <= 0) {
		return;
	}

	local_history_pos--;
	_go_history();
}

void FilePane::_go_forward() {
	dir_next->release_focus();

	if (local_history_pos >= local_history.size() - 1) {
		return;
	}

	local_history_pos++;
	_go_history();
}

void FilePane::_go_up() {
	dir_up->release_focus();

	FileSystemDirectory *dir = file_system->get_dir(current_path);
	if (!dir || dir == file_system->get_root()) {
		return;
	}

	dir = dir->get_parent();
	ERR_FAIL_NULL(dir);
	set_path(dir->get_path());
}

void FilePane::_refresh() {
	refresh->release_focus();

	file_system->scan(current_path, true);
}

bool FilePane::_process_id_pressed(int p_option, const Vector<String> &p_selected) {
	bool is_handled = true;

	switch (p_option) {
		case FileContextMenu::FILE_MENU_OPEN: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::dir_exists(path)) {
				break;
			}

			set_path(path);
		} break;

		case FileContextMenu::FILE_MENU_RENAME: {
			item_list->edit_selected();
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
				file_system->scan(dir, true);
				to_select = path;
				rename_item = true;
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
				file_system->scan(dir, true);
				to_select = path;
				rename_item = true;
			}
		} break;

		default: {
			is_handled = false;
		}
	}

	return is_handled;
}

void FilePane::_context_menu_id_pressed(int p_option) {
	_process_id_pressed(p_option, context_menu->get_targets());
}

void FilePane::_item_edited() {
	Vector<int> selected_ids = item_list->get_selected_items();
	int id = selected_ids[0];
	Dictionary d = item_list->get_item_metadata(id);

	String from = d["path"];
	String new_name = item_list->get_edit_text().strip_edges();
	if (_rename_operation_confirm(from, new_name)) {
		file_system->scan(current_path, true);

		to_select = from.get_base_dir().path_join(new_name);
	} else {
		item_list->set_item_text(id, d["name"]);
	}
}

bool FilePane::_rename_operation_confirm(const String &p_from, const String &p_new_name) {
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

void FilePane::_item_list_draw() {
	if (rename_item) {
		item_list->edit_selected();
		rename_item = false;
	}
}

void FilePane::_on_file_system_changed(FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(p_dir);

	// print_line("fs change: ", p_dir, p_dir->is_scanned(), p_dir->get_path(), current_path);

	// TODO: update ui when deleting the current or parent dir.
	if (current_path != p_dir->get_path()) {
		return;
	}

	callable_mp(this, &FilePane::_update_ui_nocheck).call_deferred(p_dir);
}

void FilePane::_set_path(const String &p_path, bool p_update_history) {
	// print_line("set path: ", p_path, current_path, file_system->is_valid_dir_path(p_path), p_update_history);
	if (current_path == p_path || (!file_system->is_valid_dir_path(p_path))) {
		return;
	}

	current_path = p_path;
	emit_signal(SNAME("path_changed"), this);

	if (p_update_history) {
		int current_history_size = local_history.size();
		if (current_history_size > 0 && current_path == local_history[current_history_size - 1]) {
			// Set history_pos to the end of local_history
			if (local_history_pos < current_history_size - 1) {
				local_history_pos = current_history_size - 1;
			}
		} else {
#define LOCAL_HISTORY_MAX 5 // TODO: config
			// Note: LOCAL_HISTORY_MAX not include current path(the last path)
			if (current_history_size > LOCAL_HISTORY_MAX) {
				// Set history_pos to the end of local_history
				local_history_pos = current_history_size - 1;
				local_history.remove_at(0);
			} else {
				// Set history_pos to the end of local_history
				local_history_pos = current_history_size; // (current_history_size - 1) + 1
			}
			local_history.push_back(current_path);
		}
	}

	FileSystemDirectory *dir = file_system->get_dir(current_path);
	// print_line("path: ", dir, dir ? dir->is_scanned() : "null");
	if (!dir || !dir->is_scanned()) {
		file_system->scan(current_path);
	} else {
		callable_mp(this, &FilePane::_update_ui).call_deferred();
	}
}

void FilePane::set_path(const String &p_path, bool p_update_history) {
	_set_path(p_path, p_update_history);

	_data_changed();
}

String FilePane::get_path() const {
	return current_path;
}

void FilePane::set_file_system(FileSystem *p_file_system) {
	if (file_system == p_file_system) {
		return;
	}

	file_system = p_file_system;
	file_system->connect("file_system_changed", callable_mp(this, &FilePane::_on_file_system_changed));

	context_menu->set_file_system(file_system);

	_set_path(file_system->get_root()->get_path());
}

void FilePane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("path_changed", PropertyInfo(Variant::OBJECT, "file_pane")));
}

Dictionary FilePane::get_config_data() {
	Dictionary d;
	d["path"] = get_path();
	return d;
}

void FilePane::apply_config_data(const Dictionary &p_dict) {
	String path = p_dict.get("path", get_path());
	if (path != get_path() && FileSystemAccess::dir_exists(path)) {
		callable_mp(this, &FilePane::_set_path).call_deferred(path, false);
	}
}

FilePane::FilePane() :
		PaneBase(get_class_static()) {
	set_custom_minimum_size(Size2(320, 240) * APP_SCALE);

	vbox = memnew(VBoxContainer);
	add_child(vbox);
	vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	toolbar = memnew(HBoxContainer);
	vbox->add_child(toolbar);

	dir_prev = memnew(Button);
	dir_prev->set_theme_type_variation(SceneStringName(FlatButton));
	dir_prev->set_tooltip_text(RTR("Previous folder"));
	dir_prev->set_disabled(true);
	dir_next = memnew(Button);
	dir_next->set_theme_type_variation(SceneStringName(FlatButton));
	dir_next->set_tooltip_text(RTR("Next folder"));
	dir_next->set_disabled(true);
	dir_up = memnew(Button);
	dir_up->set_theme_type_variation(SceneStringName(FlatButton));
	dir_up->set_tooltip_text(RTR("Parent folder"));
	dir_up->set_disabled(true);
	refresh = memnew(Button);
	refresh->set_theme_type_variation(SceneStringName(FlatButton));
	refresh->set_tooltip_text(RTR("Refresh"));

	dir_prev->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_back));
	dir_next->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_forward));
	dir_up->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_up));
	refresh->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_refresh));

	toolbar->add_child(dir_prev);
	toolbar->add_child(dir_next);
	toolbar->add_child(dir_up);
	toolbar->add_child(refresh);

	address_bar = memnew(AddressBar);
	address_bar->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	address_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	toolbar->add_child(address_bar);

	item_list = memnew(FileSystemItemList);
	item_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	item_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_list->set_select_mode(ItemList::SELECT_MULTI);
	item_list->set_allow_rmb_select(true);
	item_list->set_custom_minimum_size(Size2(100, 100));

	item_list->get_v_scroll_bar()->set_value(0);
	item_list->set_icon_mode(ItemList::ICON_MODE_LEFT);
	item_list->set_max_columns(1);
	item_list->set_max_text_lines(1);
	item_list->set_fixed_column_width(0);
	item_list->set_fixed_icon_size(Size2());

	vbox->add_child(item_list);

	item_list->connect("item_clicked", callable_mp(this, &FilePane::_item_clicked));
	item_list->connect("empty_clicked", callable_mp(this, &FilePane::_empty_clicked));
	item_list->connect("item_edited", callable_mp(this, &FilePane::_item_edited));
	item_list->connect(SceneStringName(draw), callable_mp(this, &FilePane::_item_list_draw));
	// TODO: preview?
	// item_list->connect(SceneStringName(item_selected), callable_mp(this, &FilePane::_item_selected), CONNECT_DEFERRED);
	// item_list->connect("multi_selected", callable_mp(this, &FilePane::_multi_selected));
	item_list->connect("item_activated", callable_mp(this, &FilePane::_item_dc_selected).bind());

	item_list->hide();

	MarginContainer *mc = memnew(MarginContainer);
	mc->set_v_size_flags(SIZE_EXPAND_FILL);
	// mc->set_theme_type_variation("NoBorderHorizontalBottomWide");
	vbox->add_child(mc);

	tree = memnew(FileSystemTree);
	tree->set_hide_root(true);
	// tree->set_select_mode(Tree::SELECT_MULTI);
	tree->set_select_mode(Tree::SELECT_ROW);
	tree->set_allow_reselect(true);

	// TODO
	// SET_DRAG_FORWARDING_GCD(tree, FilePane);

	tree->set_theme_type_variation("TreeTable");
	tree->set_hide_folding(true);
	tree->set_columns(4);
	tree->set_column_titles_visible(true);

	tree->set_column_title(0, TTRC("Name"));
	tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
	tree->set_column_clip_content(0, true);
	tree->set_column_expand(0, true);
	tree->set_column_expand_ratio(0, 2);

	tree->set_column_title(1, TTRC("Modified Time"));
	tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_LEFT);
	tree->set_column_clip_content(1, true);
	tree->set_column_expand(1, true);
	tree->set_column_expand_ratio(1, 1);

	tree->set_column_title(2, TTRC("Type"));
	tree->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_LEFT);
	tree->set_column_expand(2, false);

	tree->set_column_title(3, TTRC("Size"));
	tree->set_column_title_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
	tree->set_column_expand(3, false);

	// tree->connect("cell_selected", callable_mp(this, &FilePane::_autoload_selected));
	// tree->connect("item_edited", callable_mp(this, &FilePane::_autoload_edited));
	// tree->connect("button_clicked", callable_mp(this, &FilePane::_autoload_button_pressed));
	// tree->connect("item_activated", callable_mp(this, &FilePane::_autoload_activated));

	// mc->add_child(tree, true);
	mc->add_child(tree);

	status_bar = memnew(HBoxContainer);
	vbox->add_child(status_bar);
	status_bar->set_alignment(BoxContainer::ALIGNMENT_END);

	mc = memnew(MarginContainer);
	mc->set_v_size_flags(SIZE_EXPAND_FILL);
	mc->set_theme_type_variation("NoBorderHorizontalBottomWide");
	status_bar->add_child(mc);

	item_count = memnew(Label);
	// status_bar->add_child(item_count);
	mc->add_child(item_count);
	item_count->set_text("0");

	context_menu = memnew(FileContextMenu);
	add_child(context_menu);
	context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FilePane::_context_menu_id_pressed));

	address_bar->connect(SceneStringName(text_submitted), callable_mp(this, &FilePane::_on_address_submitted));
	address_bar->connect(SceneStringName(item_selected), callable_mp(this, &FilePane::_select_history));

	APP_SHORTCUT("file_management/focus_path", TTRC("Focus Path"), KeyModifierMask::CMD_OR_CTRL | Key::L);

	// APP_SHORTCUT("file_management/prev", dir_prev->get_tooltip_text(), KeyModifierMask::ALT | Key::LEFT);
	APP_SHORTCUT_ARRAY("file_management/prev",
			dir_prev->get_tooltip_text(),
			{ int32_t(KeyModifierMask::ALT | Key::LEFT), int32_t(Key::BACK) });
	APP_SHORTCUT("file_management/next", dir_next->get_tooltip_text(), KeyModifierMask::ALT | Key::RIGHT);
	APP_SHORTCUT("file_management/up", dir_up->get_tooltip_text(), KeyModifierMask::ALT | Key::UP);
	APP_SHORTCUT("file_management/refresh", refresh->get_tooltip_text(), Key::F5);

	dir_prev->set_shortcut(ED_GET_SHORTCUT("file_management/prev"));
	dir_next->set_shortcut(ED_GET_SHORTCUT("file_management/next"));
	dir_up->set_shortcut(ED_GET_SHORTCUT("file_management/up"));
	refresh->set_shortcut(ED_GET_SHORTCUT("file_management/refresh"));

	// TODO: Handle shortcut input upon receiving focus.
	set_process_shortcut_input(true);
}

FilePane::~FilePane() {
	file_system = nullptr;
}
