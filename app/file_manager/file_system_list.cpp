#include "file_system_list.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/texture_rect.h"

#include "scene/scene_string_names.h"

#include "app/core/io/file_system_access.h"

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

void FileSystemList::_update_dir() {
	dir->set_text(_get_path());
}

void FileSystemList::_add_item(const FileInfo &p_fi, bool p_is_dir) {
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder"));
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File"));

		icon = p_is_dir ? folder : file;
	}
	item_list->add_item(p_fi.name, icon);
	// item_list->set_item_icon(-1, icon);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = p_is_dir;
	item_list->set_item_metadata(-1, d);
}

void FileSystemList::_update_file_list() {
	item_list->clear();

	if (preview->get_texture().is_valid()) {
		preview->show();
	}

	String cdir = _get_path();

	List<FileInfo> subdirs;
	List<FileInfo> files;
	Error err = FileSystemAccess::list_file_infos(cdir, subdirs, files);
	if (err != OK) {
		return;
	}

	// show_hidden_files;

	// list dirs
	for (const FileInfo &subdir : subdirs) {
		_add_item(subdir, true);
	}

	// list files
	for (const FileInfo &file : files) {
		_add_item(file);
	}
}

void FileSystemList::_update_up_button() {
	if (_get_path() != COMPUTER_PATH) {
		dir_up->set_disabled(false);
	} else {
		dir_up->set_disabled(true);
	}
}

void FileSystemList::_update_history_button() {
	dir_prev->set_disabled(local_history_pos == 0);
	dir_next->set_disabled(local_history_pos == local_history.size() - 1);
}

Vector<String> FileSystemList::_get_selected() const {
	Vector<int> selected_ids = item_list->get_selected_items();
	Vector<String> selected;
	selected.resize(selected_ids.size());

	String *selected_write = selected.ptrw();
	int i = 0;
	for (const int id : selected_ids) {
		Dictionary d = item_list->get_item_metadata(id);
		selected_write[i] = d["path"];
		i++;
	}
	return selected;
}

void FileSystemList::_item_menu_id_pressed(int p_option) {
	FileSystemControl::_item_menu_id_pressed(p_option);
}

void FileSystemList::_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	item_list->deselect_all();
	popup_menu(p_pos, MENU_MODE_EMPTY);
}

void FileSystemList::_item_clicked(int p_item, const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	Dictionary d = item_list->get_item_metadata(p_item);

	if (d["is_dir"]) {
		popup_menu(p_pos, MENU_MODE_FOLDER);
	} else {
		popup_menu(p_pos, MENU_MODE_FILE);
	}
}

void FileSystemList::_item_dc_selected(int p_item) {
	int current = p_item;
	if (current < 0 || current >= item_list->get_item_count()) {
		return;
	}

	Dictionary d = item_list->get_item_metadata(current);

	if (d["is_dir"]) {
		change_path(d["path"]);
	} else {
		// TODO: open_file()/run_file()
		const String file = d["path"]; // _get_path().path_join(d["name"]);
		print_line("run: " + file);
		OS::get_singleton()->shell_open(file);
	}
}

void FileSystemList::_select_history(int p_idx) {
	local_history_pos = p_idx;
	_go_history();
}

void FileSystemList::_dir_submitted(const String &p_path) {
	// print_line(_get_path() + ", dir submit: " + p_path);
	String new_path = p_path;
	// TODO
	// #ifdef WINDOWS_ENABLED
	// 	if (drives->is_visible() && !new_path.is_network_share_path() && new_path.is_absolute_path() && new_path.find(":/") == -1 && new_path.find(":\\") == -1) {
	// 		// Non network path without X:/ prefix on Windows, add drive letter.
	// 		new_path = drives->get_item_text(drives->get_selected()).path_join(new_path);
	// 	}
	// #endif
	if (!change_path(new_path)) {
		_update_dir();
	}
}

void FileSystemList::_push_history() {
	// TODO: remove histories from _push_history()
	String new_path = _get_path();
	int current_history_size = local_history.size();
	if (current_history_size > 0 && new_path == local_history[current_history_size - 1]) {
		// Set history_pos to the end of local_history
		if (local_history_pos < current_history_size - 1) {
			local_history_pos = current_history_size - 1;
			histories->select(local_history_pos);

			_update_history_button();
		}
		return;
	}

	// Note: LOCAL_HISTORY_MAX not include current path(the last path)
	if (current_history_size > LOCAL_HISTORY_MAX) {
		// Set history_pos to the end of local_history
		local_history_pos = current_history_size - 1;
		local_history.remove_at(0);
		histories->remove_item(0);
		histories->select(-1); // clear selection
	} else {
		// Set history_pos to the end of local_history
		local_history_pos = current_history_size; // (current_history_size - 1) + 1
	}
	local_history.push_back(new_path);
	histories->add_item(new_path);

	histories->select(local_history_pos);
	// print_line(vformat("his %d, sel %d, %s", histories->get_item_count(), local_history_pos, histories->get_item_text(local_history_pos)));

	_update_history_button();
}

void FileSystemList::_go_history() {
	_set_path(local_history[local_history_pos]);
	histories->select(local_history_pos);

	_update_dir();
	_update_file_list();
	_update_up_button();
	_update_history_button();
}

void FileSystemList::_go_back() {
	if (local_history_pos <= 0) {
		return;
	}

	local_history_pos--;
	_go_history();
	// print_line(vformat("his pos %d, %s", local_history_pos, local_history[local_history_pos]));
}

void FileSystemList::_go_forward() {
	if (local_history_pos >= local_history.size() - 1) {
		return;
	}

	local_history_pos++;
	_go_history();
}

void FileSystemList::_go_up() {
	String cdir = _get_path().trim_suffix("/");

	// go up Computer
	List<FileInfo> drives;
	Error err = FileSystemAccess::list_drives(drives);
	if (err == OK) {
		for (const FileInfo &drive : drives) {
			if (cdir == drive.name) {
				change_path(COMPUTER_PATH);
				return;
			}
		}
	}

	change_path(cdir.get_base_dir().trim_suffix("/"));
}

void FileSystemList::_initialize_ui() {
	vbox = memnew(VBoxContainer);
	add_child(vbox);
	vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	pathhb = memnew(HBoxContainer);
	vbox->add_child(pathhb);

	dir_prev = memnew(Button);
	dir_prev->set_theme_type_variation(SceneStringName(FlatButton));
	dir_prev->set_tooltip_text(RTR("Go to previous folder."));
	dir_prev->set_disabled(true);
	dir_prev->set_focus_mode(FOCUS_NONE);
	dir_next = memnew(Button);
	dir_next->set_theme_type_variation(SceneStringName(FlatButton));
	dir_next->set_tooltip_text(RTR("Go to next folder."));
	dir_next->set_disabled(true);
	dir_next->set_focus_mode(FOCUS_NONE);
	dir_up = memnew(Button);
	dir_up->set_theme_type_variation(SceneStringName(FlatButton));
	dir_up->set_tooltip_text(RTR("Go to parent folder."));
	dir_up->set_disabled(true);
	dir_up->set_focus_mode(FOCUS_NONE);
	refresh = memnew(Button);
	refresh->set_theme_type_variation(SceneStringName(FlatButton));
	refresh->set_tooltip_text(RTR("Refresh files."));
	refresh->set_focus_mode(FOCUS_NONE);

	dir_prev->connect(SceneStringName(pressed), callable_mp(this, &FileSystemList::_go_back));
	dir_next->connect(SceneStringName(pressed), callable_mp(this, &FileSystemList::_go_forward));
	dir_up->connect(SceneStringName(pressed), callable_mp(this, &FileSystemList::_go_up));
	refresh->connect(SceneStringName(pressed), callable_mp(this, &FileSystemList::_update_file_list));

	pathhb->add_child(dir_prev);
	pathhb->add_child(dir_next);
	pathhb->add_child(dir_up);
	pathhb->add_child(refresh);

	dir = memnew(LineEdit);
	dir->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	dir->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pathhb->add_child(dir);

	histories = memnew(OptionButton);
	histories->connect(SceneStringName(item_selected), callable_mp(this, &FileSystemList::_select_history));
	// histories->set_focus_mode(FOCUS_NONE);	// TODO
	pathhb->add_child(histories);

	body_hsplit = memnew(HSplitContainer);
	body_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(body_hsplit);

	item_list = memnew(FileSystemItemList);
	item_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	item_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_list->set_select_mode(ItemList::SELECT_MULTI);
	item_list->set_allow_rmb_select(true);
	item_list->set_custom_minimum_size(Size2(100, 100));

	item_list->get_v_scroll_bar()->set_value(0);
	item_list->set_icon_mode(ItemList::ICON_MODE_LEFT);
	item_list->set_max_columns(1);
	item_list->set_max_text_lines(1);
	item_list->set_fixed_column_width(0);
	item_list->set_fixed_icon_size(Size2());

	body_hsplit->add_child(item_list);

	preview = memnew(TextureRect);
	body_hsplit->add_child(preview);

	// item_list->connect("item_clicked", callable_mp(this, &FileSystemList::_item_list_item_rmb_clicked));
	// item_list->connect("empty_clicked", callable_mp(this, &FileSystemList::_item_list_empty_clicked));
	item_list->connect("item_clicked", callable_mp(this, &FileSystemList::_item_clicked));
	item_list->connect("empty_clicked", callable_mp(this, &FileSystemList::_empty_clicked));
	// item_list->connect(SceneStringName(gui_input), callable_mp(this, &FileSystemList::_item_list_gui_input));
	// item_list->connect(SceneStringName(item_selected), callable_mp(this, &FileSystemList::_item_selected), CONNECT_DEFERRED);
	// item_list->connect("multi_selected", callable_mp(this, &FileSystemList::_multi_selected), CONNECT_DEFERRED);
	item_list->connect("item_activated", callable_mp(this, &FileSystemList::_item_dc_selected).bind());
	// item_list->connect("empty_clicked", callable_mp(this, &FileSystemList::_items_clear_selection));
	item_list->connect("item_edited", callable_mp(this, &FileSystemList::_item_edited));

	dir->connect(SceneStringName(text_submitted), callable_mp(this, &FileSystemList::_dir_submitted));
}

void FileSystemList::_update_icons() {
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

void FileSystemList::_update_file_ui() {
	_update_dir();
	_update_file_list();
	_update_up_button();
	// _update_favorites();
	// _update_recent();

	_push_history();
}

void FileSystemList::_set_empty_menu_item(PopupMenu *p_popup) {
	PopupMenu *new_menu = memnew(PopupMenu);
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemList::_item_menu_id_pressed));
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("List"), FILE_MENU_VIEW_LIST);
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("Detail"), FILE_MENU_VIEW_DETAIL);
	p_popup->add_submenu_node_item(RTR("View"), new_menu, FILE_MENU_VIEW);
	p_popup->set_item_icon(new_menu->get_item_index(FILE_MENU_VIEW), get_app_theme_icon(SNAME("Folder"))); // TODO

	new_menu = memnew(PopupMenu);
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemList::_item_menu_id_pressed));
	// new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("List"), FILE_MENU_VIEW_LIST);
	// new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("Detail"), FILE_MENU_VIEW_DETAIL);
	p_popup->add_submenu_node_item(RTR("Sort"), new_menu, FILE_MENU_SORT);
	p_popup->set_item_icon(new_menu->get_item_index(FILE_MENU_SORT), get_app_theme_icon(SNAME("Folder"))); // TODO

	p_popup->add_separator();

	p_popup->add_item(RTR("Paste"), FILE_MENU_PASTE);

	new_menu = memnew(PopupMenu);
	new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemList::_item_menu_id_pressed));
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("Folder..."), FILE_MENU_NEW_FOLDER);
	new_menu->add_icon_item(get_app_theme_icon(SNAME("Folder")), RTR("TextFile..."), FILE_MENU_NEW_TEXTFILE);
	p_popup->add_submenu_node_item(RTR("Create New"), new_menu, FILE_MENU_NEW);
	p_popup->set_item_icon(new_menu->get_item_index(FILE_MENU_NEW), get_app_theme_icon(SNAME("Folder")));

	p_popup->add_separator();

	p_popup->add_item(RTR("Open in terminal"), FILE_MENU_OPEN_IN_TERMINAL);
}

void FileSystemList::_set_file_menu_item(PopupMenu *p_popup) {
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

void FileSystemList::_set_folder_menu_item(PopupMenu *p_popup) {
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
}

void FileSystemList::_item_edited() {
	String new_name = item_list->get_edit_text().strip_edges();

	Vector<int> selected_ids = item_list->get_selected_items();
	int id = selected_ids[0];
	Dictionary d = item_list->get_item_metadata(id);
	if (_rename_operation_confirm(new_name)) {
		String new_path = ((String)d["path"]).get_base_dir().path_join(new_name);
		d["name"] = new_name;
		d["path"] = new_path;
		item_list->set_item_metadata(id, d);

		item_list->set_item_icon(id, FileSystemAccess::get_icon(new_path, d["is_dir"]));
	} else {
		item_list->set_item_text(id, d["name"]);
	}
}

bool FileSystemList::edit_selected(const FileOrFolder &p_selected) {
	return item_list->edit_selected();
}

FileSystemList::FileSystemList() {
	_initialize_ui();
}

FileSystemList::~FileSystemList() {
}
