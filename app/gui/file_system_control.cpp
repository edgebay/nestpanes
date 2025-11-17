#include "file_system_control.h"

#include "scene/gui/popup_menu.h"

#include "app/core/io/file_system_access.h"

void FileSystemControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_icons();
			update_file_ui();
		} break;
	}
}

// Note: use absolute path
bool FileSystemControl::change_path(const String &p_path) {
	bool result = _set_path(p_path);
	if (result) {
		update_file_ui();
	}
	return result;
}

void FileSystemControl::_bind_methods() {
	// ADD_SIGNAL(MethodInfo("path_changed", PropertyInfo(Variant::STRING, "path")));
	ADD_SIGNAL(MethodInfo("path_changed", PropertyInfo(Variant::OBJECT, "fs")));
}

PopupMenu *FileSystemControl::get_menu() const {
	return item_menu;
}

bool FileSystemControl::_set_path(const String &p_path) {
	if (p_path != COMPUTER_PATH && !FileSystemAccess::dir_exists(p_path)) {
		return false;
	}
	current_path = p_path;
	// emit_signal(SNAME("path_changed"), p_path);
	emit_signal(SNAME("path_changed"), this);
	return true;
}

String FileSystemControl::_get_path() const {
	return current_path;
}

void FileSystemControl::_update_file_ui_method() {
	if (!updating_file_ui) {
		return;
	}

	_update_file_ui();

	updating_file_ui = false;
}

void FileSystemControl::update_file_ui() {
	if (!is_visible() || updating_file_ui) {
		return;
	}

	updating_file_ui = true;
	callable_mp(this, &FileSystemControl::_update_file_ui_method).call_deferred();
}

bool FileSystemControl::is_updating_file() {
	return updating_file_ui;
}

void FileSystemControl::_item_menu_id_pressed(int p_option) {
	const Vector<String> &p_selected = _get_selected(); // TODO: p_selected -> selected
	switch (p_option) {
		case FILE_MENU_CUT: {
			FileSystemAccess::cut(p_selected);
		} break;

		case FILE_MENU_COPY: {
			FileSystemAccess::copy(p_selected);
		} break;

		case FILE_MENU_PASTE: {
			String path;
			if (!p_selected.is_empty()) {
				path = p_selected[0];
			} else {
				path = _get_path();
			}

			if (!path.is_empty()) {
				FileSystemAccess::paste(path);
			}
		} break;

		case FILE_MENU_REMOVE: {
			// Remove the selected files.
			for (const auto &path : p_selected) {
				Error err = OS::get_singleton()->move_to_trash(path);
				if (err != OK) {
					print_line("Cannot remove: " + path); // TODO
				}
			}
		}
	}
}

void FileSystemControl::_set_menu_item(PopupMenu *p_popup, MenuMode p_mode) {
	switch (p_mode) {
		case MENU_MODE_EMPTY:
			_set_empty_menu_item(p_popup);
			break;
		case MENU_MODE_FILE:
			_set_file_menu_item(p_popup);
			break;
		case MENU_MODE_FOLDER:
			_set_folder_menu_item(p_popup);
			break;
	}
}

void FileSystemControl::set_current_path(const String &p_path) {
	change_path(p_path);
}

String FileSystemControl::get_current_path() const {
	return _get_path();
}

String FileSystemControl::get_current_dir_name() const {
	return _get_path().get_file();
}

Ref<Texture2D> FileSystemControl::get_current_dir_icon() const {
	if (FileSystemAccess::has_system_icon(_get_path())) {
		return FileSystemAccess::get_system_icon(_get_path());
	} else if (FileSystemAccess::has_system_icon("Folder")) {
		return FileSystemAccess::get_system_icon("Folder");
	}
	return get_app_theme_icon(SNAME("Folder"));
}

void FileSystemControl::popup_menu(const Vector2 &p_pos, MenuMode p_mode) {
	item_menu->clear();

	_set_menu_item(item_menu, p_mode);

	item_menu->set_position(get_screen_position() + p_pos);
	item_menu->reset_size();
	item_menu->popup();
}

FileSystemControl::FileSystemControl() {
	_set_path(COMPUTER_PATH);

	item_menu = memnew(PopupMenu);
	item_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemControl::_item_menu_id_pressed));
	add_child(item_menu, false, INTERNAL_MODE_FRONT);
}

FileSystemControl::~FileSystemControl() {
}
