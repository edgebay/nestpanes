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
	ClassDB::bind_method(D_METHOD("set_current_path", "current_path"), &FileSystemControl::set_current_path);
	ClassDB::bind_method(D_METHOD("get_current_path"), &FileSystemControl::get_current_path);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "current_path"), "set_current_path", "get_current_path");

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
	// TODO: update ui when visible
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
				if (FileSystemAccess::file_exists(path)) {
					path = path.get_base_dir();
				}
			} else {
				path = _get_path();
			}

			if (!path.is_empty()) {
				FileSystemAccess::paste(path);
			}
		} break;

		case FILE_MENU_RENAME: {
			if (!p_selected.is_empty()) {
				print_line(p_selected[0]);

				// Set to_rename variable for callback execution.
				to_rename.path = p_selected[0];
				to_rename.is_file = FileSystemAccess::file_exists(to_rename.path);
				edit_selected(to_rename);
			}
		} break;

		// FILE_MENU_DELETE
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

bool FileSystemControl::_rename_operation_confirm(const String &p_new_name) {
	String new_name = p_new_name;
	String old_name = to_rename.path.get_file();

	bool rename_error = false;
	if (new_name.length() == 0) {
		print_line(TTRC("No name provided."));
		rename_error = true;
	} else if (new_name.contains_char('/') || new_name.contains_char('\\') || new_name.contains_char(':')) {
		print_line(TTRC("Name contains invalid characters."));
		rename_error = true;
	} else if (new_name[0] == '.') {
		print_line(TTRC("This filename begins with a dot rendering the file invisible to the editor.\nIf you want to rename it anyway, use your operating system's file manager."));
		rename_error = true;
	}

	// TODO
	// // Restore original name.
	// if (rename_error) {
	// 	if (ti) {
	// 		ti->set_text(col_index, old_name);
	// 	}
	// 	return rename_error;
	// }

	String old_path = to_rename.path;
	String new_path = old_path.get_base_dir().path_join(new_name);
	if (old_path == new_path) {
		return false;
	}

	return FileSystemAccess::rename(old_path, new_path) == OK; // TODO: error handle
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
