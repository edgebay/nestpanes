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
	d["dir"] = p_is_dir;
	d["bundle"] = false; // only macOS
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

void FileSystemList::_item_dc_selected(int p_item) {
	int current = p_item;
	if (current < 0 || current >= item_list->get_item_count()) {
		return;
	}

	Dictionary d = item_list->get_item_metadata(current);

	if (d["dir"]) {
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

	item_list = memnew(ItemList);
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
	// item_list->connect(SceneStringName(gui_input), callable_mp(this, &FileSystemList::_item_list_gui_input));
	// item_list->connect(SceneStringName(item_selected), callable_mp(this, &FileSystemList::_item_selected), CONNECT_DEFERRED);
	// item_list->connect("multi_selected", callable_mp(this, &FileSystemList::_multi_selected), CONNECT_DEFERRED);
	item_list->connect("item_activated", callable_mp(this, &FileSystemList::_item_dc_selected).bind());
	// item_list->connect("empty_clicked", callable_mp(this, &FileSystemList::_items_clear_selection));
	dir->connect(SceneStringName(text_submitted), callable_mp(this, &FileSystemList::_dir_submitted));

	// item_menu = memnew(PopupMenu);
	// // item_menu->set_flag(Window::FLAG_MOUSE_PASSTHROUGH, true);
	// item_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemList::_item_menu_id_pressed));
	// add_child(item_menu);
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

FileSystemList::FileSystemList() {
	_initialize_ui();
}

FileSystemList::~FileSystemList() {
}
