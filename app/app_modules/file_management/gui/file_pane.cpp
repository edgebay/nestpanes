#include "file_pane.h"

#include "app/app_modules/file_management/file_system_list.h" // TODO: FileSystemItemList
#include "app/app_modules/file_management/gui/address_bar.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"

#include "app/gui/app_control.h"
#include "app/themes/app_scale.h"

#include "app/app_modules/file_management/file_system.h"

String FilePane::_get_pane_title() const {
	return "Folder";
}

Ref<Texture2D> FilePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("Folder"));
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

	print_line("update ui: ", current_path);

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

	_update_item_list();

	print_line("update ui end");
}

void FilePane::_add_item(const FileInfo &p_fi, bool p_is_dir) {
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder"));
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File"));

		icon = p_is_dir ? folder : file;
	}
	item_list->add_item(p_fi.name, icon);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = p_is_dir;
	item_list->set_item_metadata(-1, d);
}

void FilePane::_update_item_list() {
	print_line("update item list");

	item_list->clear();

	FileSystemDirectory *dir = file_system->get_dir(current_path);
	if (!dir) {
		return;
	}

	// list dirs
	for (int i = 0; i < dir->get_subdir_count(); i++) {
		FileSystemDirectory *subdir = dir->get_subdir(i);
		FileInfo fi;
		fi.type = FOLDER_TYPE;
		fi.name = subdir->get_name();
		fi.path = subdir->get_path();
		fi.icon = subdir->get_icon();
		fi.is_hidden = subdir->is_hidden();
		_add_item(fi, true);
	}

	// list files
	for (int i = 0; i < dir->get_file_count(); i++) {
		_add_item(*dir->get_file(i));
	}
}

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
		const String file = d["path"]; // _get_path().path_join(d["name"]);
		print_line("run: " + file);
		OS::get_singleton()->shell_open(file);
	}
}

void FilePane::_on_address_submitted(const String &p_path) {
	if (!file_system->is_valid_path(p_path)) {
		return;
	}

	set_path(p_path);
}

void FilePane::_select_history(int p_idx) {
	local_history_pos = p_idx;
	_go_history();
}

void FilePane::_go_history() {
	set_path(local_history[local_history_pos], false);
}

void FilePane::_go_back() {
	if (local_history_pos <= 0) {
		return;
	}

	local_history_pos--;
	_go_history();
}

void FilePane::_go_forward() {
	if (local_history_pos >= local_history.size() - 1) {
		return;
	}

	local_history_pos++;
	_go_history();
}

void FilePane::_go_up() {
	FileSystemDirectory *dir = file_system->get_dir(current_path);
	if (!dir || dir == file_system->get_root()) {
		return;
	}

	dir = dir->get_parent();
	if (!dir->is_scanned()) {
		file_system->update(dir);
	}
	_set_path(dir);
}

void FilePane::_on_file_system_changed(FileSystemDirectory *p_dir) {
	if (current_path != p_dir->get_path()) {
		return;
	}

	callable_mp(this, &FilePane::_update_item_list).call_deferred();
}

void FilePane::_set_path(FileSystemDirectory *p_dir, bool p_update_history) {
	current_path = p_dir->get_path();
	emit_signal(SNAME("path_changed"), this);

	set_title(p_dir->get_name());
	set_icon(p_dir->get_icon());

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
			// print_line(vformat("his %d, sel %d, %s", histories->get_item_count(), local_history_pos, histories->get_item_text(local_history_pos)));
		}
	}

	callable_mp(this, &FilePane::_update_ui).call_deferred();
}

void FilePane::set_path(const String &p_path, bool p_update_history) {
	if (current_path == p_path) {
		return;
	}

	FileSystemDirectory *dir = file_system->load_dir(p_path);
	if (!dir) {
		return;
	}

	_set_path(dir, p_update_history);
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

	_set_path(file_system->get_root());
}

void FilePane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("path_changed", PropertyInfo(Variant::OBJECT, "file_pane")));
}

FilePane::FilePane() :
		PaneBase(get_class_static()) {
	set_custom_minimum_size(Size2(320, 320) * APP_SCALE);

	vbox = memnew(VBoxContainer);
	add_child(vbox);
	vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	toolbar = memnew(HBoxContainer);
	vbox->add_child(toolbar);

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

	dir_prev->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_back));
	dir_next->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_forward));
	dir_up->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_go_up));
	refresh->connect(SceneStringName(pressed), callable_mp(this, &FilePane::_update_item_list));

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

	// // item_list->connect("item_clicked", callable_mp(this, &FilePane::_item_list_item_rmb_clicked));
	// // item_list->connect("empty_clicked", callable_mp(this, &FilePane::_item_list_empty_clicked));
	// item_list->connect("item_clicked", callable_mp(this, &FilePane::_item_clicked));
	// item_list->connect("empty_clicked", callable_mp(this, &FilePane::_empty_clicked));
	// // item_list->connect(SceneStringName(gui_input), callable_mp(this, &FilePane::_item_list_gui_input));
	// // item_list->connect(SceneStringName(item_selected), callable_mp(this, &FilePane::_item_selected), CONNECT_DEFERRED);
	// // item_list->connect("multi_selected", callable_mp(this, &FilePane::_multi_selected), CONNECT_DEFERRED);
	item_list->connect("item_activated", callable_mp(this, &FilePane::_item_dc_selected).bind());
	// // item_list->connect("empty_clicked", callable_mp(this, &FilePane::_items_clear_selection));
	// item_list->connect("item_edited", callable_mp(this, &FilePane::_item_edited));

	address_bar->connect(SceneStringName(text_submitted), callable_mp(this, &FilePane::_on_address_submitted));
	address_bar->connect(SceneStringName(item_selected), callable_mp(this, &FilePane::_select_history));
}

FilePane::~FilePane() {
	file_system = nullptr;
}
