#include "file_system_control.h"

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

FileSystemControl::FileSystemControl() {
	_set_path(COMPUTER_PATH);
}

FileSystemControl::~FileSystemControl() {
}
