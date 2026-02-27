#include "file_pane.h"

#include "app/app_modules/file_management/gui/address_bar.h"
#include "app/app_modules/file_management/gui/file_context_menu.h"
#include "app/app_modules/file_management/gui/file_system_tree.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/popup_menu.h"

#include "app/gui/app_control.h"
#include "app/settings/app_settings.h"
#include "app/themes/app_scale.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

String FilePane::_get_pane_title() const {
	return RTR("Folder");
}

Ref<Texture2D> FilePane::_get_pane_icon() const {
	return get_app_theme_icon(SNAME("Folder"));
}

void FilePane::_on_active(bool p_active) {
	PaneBase::_on_active(p_active);
	if (p_active) {
		dir_prev->set_shortcut(ED_GET_SHORTCUT("file_management/prev"));
		dir_next->set_shortcut(ED_GET_SHORTCUT("file_management/next"));
		dir_up->set_shortcut(ED_GET_SHORTCUT("file_management/up"));
		refresh->set_shortcut(ED_GET_SHORTCUT("file_management/refresh"));
	} else {
		dir_prev->set_shortcut(Ref<Shortcut>());
		dir_next->set_shortcut(Ref<Shortcut>());
		dir_up->set_shortcut(Ref<Shortcut>());
		refresh->set_shortcut(Ref<Shortcut>());
	}
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
			targets = tree->get_selected_paths();
			if (targets.is_empty()) {
				option_id = -1;
			}
		}

		if (option_id >= 0) {
			callable_mp(this, &FilePane::_process_shortcut_input).call_deferred(option_id, targets);
		}

		if (is_handled) {
			// accept_event();
			get_tree()->get_root()->set_input_as_handled();
		}
	}
}

void FilePane::_process_shortcut_input(int p_option, const Vector<String> &p_selected) {
	tree->process_menu_id(p_option, p_selected);
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
	// print_line("update ui: ", current_path, dir, dir->get_name(), dir->get_path());
	if (!dir) {
		file_system->scan(current_path, true);
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

	address_bar->set_text(p_dir->get_path());
	// TODO
	// for (local_history) histories->add_item(history);
	// histories->select(local_history_pos);

	_update_files(p_dir);
	_update_status_bar();
}

void FilePane::_add_item(const FileInfo &p_fi) {
	TreeItem *root = tree->get_root();
	ERR_FAIL_NULL(root);

	TreeItem *item = tree->add_item(p_fi, root);
	// print_line("add item: ", item, item->get_text(0));
}

void FilePane::_update_files(FileSystemDirectory *p_dir) {
	ERR_FAIL_NULL(p_dir);

	tree->clear();
	TreeItem *root = tree->create_item();
	root->set_metadata(0, current_path);

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

	// print_line("update files: ", dir_count, file_count);
}

void FilePane::_update_status_bar() {
	int total_count = tree->get_root()->get_child_count();
	String info = vformat(RTRN("%d item", "%d items", total_count), total_count);
	int selected_count = tree->get_selected_paths().size();
	// print_line("status: ", info, selected_count);
	if (selected_count) {
		info = vformat(RTR("%d selected"), selected_count) + " | " + info;
	}
	item_count->set_text(info);
}

void FilePane::_on_item_activated() {
	callable_mp(this, &FilePane::_update_status_bar).call_deferred();

	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	Dictionary d = selected->get_metadata(0);
	String path = d["path"];
	bool is_dir = d["is_dir"];

	if (is_dir) {
		set_path(path);
	} else {
		// TODO: signal
		// emit_signal(SNAME("item_activated"), path, is_dir);

		// TODO: open_file()/run_file()
		const String file = d["path"];
		OS::get_singleton()->shell_open(file);
	}
}

void FilePane::_on_multi_selected(Object *p_item, int p_column, bool p_selected) {
	callable_mp(this, &FilePane::_update_status_bar).call_deferred();

	if (!p_selected) {
		return;
	}

	TreeItem *selected = tree->get_selected();
	if (!selected) {
		return;
	}

	// TODO
	// Dictionary d = selected->get_metadata(0);
	// String path = d["path"];
	// bool is_dir = d["is_dir"];

	// emit_signal(SceneStringName(item_selected), path, is_dir);
}

void FilePane::_on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button) {
	callable_mp(this, &FilePane::_update_status_bar).call_deferred();
}

void FilePane::_on_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	callable_mp(this, &FilePane::_update_status_bar).call_deferred();
}

void FilePane::_on_address_submitted(const String &p_path) {
	if (!file_system->is_valid_dir_path(p_path)) {
		return;
	}

	// TODO: Sync the name case to the actual filesystem name.
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

void FilePane::_on_file_system_changed(const String &p_path) {
	// print_line("fs changed: ", p_path, current_path);
	if (current_path != p_path) {
		return;
	}

	callable_mp(this, &FilePane::_update_ui).call_deferred();
}

void FilePane::_set_path(const String &p_path, bool p_update_history) {
	String path = p_path.simplify_path();
	// print_line("set path: ", p_path, path, current_path, file_system->is_valid_dir_path(path), p_update_history);
	if (current_path == path || (!file_system->is_valid_dir_path(path))) {
		return;
	}

	// TODO: Sync the name case to the actual filesystem name.
	current_path = path;
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
	// print_line("path: ", dir, dir ? dir->is_scanned() : false);
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

	MarginContainer *mc = memnew(MarginContainer);
	mc->set_v_size_flags(SIZE_EXPAND_FILL);
	// mc->set_theme_type_variation("NoBorderHorizontalBottomWide");
	vbox->add_child(mc);

	tree = memnew(FileSystemTree);
	tree->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_display_mode(FileSystemTree::DISPLAY_MODE_LIST);

	// TODO
	// SET_DRAG_FORWARDING_GCD(tree, FilePane);

	// TODO: Preview on selection?
	tree->connect("item_activated", callable_mp(this, &FilePane::_on_item_activated));
	tree->connect("multi_selected", callable_mp(this, &FilePane::_on_multi_selected));

	tree->connect("item_mouse_selected", callable_mp(this, &FilePane::_on_item_mouse_selected));
	tree->connect("empty_clicked", callable_mp(this, &FilePane::_on_empty_clicked));

	mc->add_child(tree);

	status_bar = memnew(HBoxContainer);
	vbox->add_child(status_bar);
	status_bar->set_alignment(BoxContainer::ALIGNMENT_END);

	mc = memnew(MarginContainer);
	mc->set_v_size_flags(SIZE_EXPAND_FILL);
	mc->set_theme_type_variation("NoBorderHorizontalBottomWide");
	status_bar->add_child(mc);

	item_count = memnew(Label);
	mc->add_child(item_count);
	item_count->set_text("");

	context_menu = memnew(FileContextMenu);
	add_child(context_menu);
	tree->set_context_menu(context_menu);

	address_bar->connect(SceneStringName(text_submitted), callable_mp(this, &FilePane::_on_address_submitted));
	address_bar->connect(SceneStringName(item_selected), callable_mp(this, &FilePane::_select_history));

	APP_SHORTCUT("file_management/focus_path", RTR("Focus Path"), KeyModifierMask::CMD_OR_CTRL | Key::L);

	// APP_SHORTCUT("file_management/prev", dir_prev->get_tooltip_text(), KeyModifierMask::ALT | Key::LEFT);
	APP_SHORTCUT_ARRAY("file_management/prev",
			dir_prev->get_tooltip_text(),
			{ int32_t(KeyModifierMask::ALT | Key::LEFT), int32_t(Key::BACK) });
	APP_SHORTCUT("file_management/next", dir_next->get_tooltip_text(), KeyModifierMask::ALT | Key::RIGHT);
	APP_SHORTCUT("file_management/up", dir_up->get_tooltip_text(), KeyModifierMask::ALT | Key::UP);
	APP_SHORTCUT("file_management/refresh", refresh->get_tooltip_text(), Key::F5);
}

FilePane::~FilePane() {
	file_system = nullptr;
}
