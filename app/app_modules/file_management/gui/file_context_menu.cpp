#include "file_context_menu.h"

#include "app/app_core/io/file_system_access.h"

void FileContextMenu::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			connect("index_pressed", callable_mp(this, &FileContextMenu::_pressed));
		} break;
	}
}

void FileContextMenu::_pressed(int p_index) {
	if (p_index >= FILE_MENU_MAX) {
		emit_signal(SNAME("custom_id_pressed"), p_index);
		return;
	}

	if (targets.is_empty()) {
		return;
	}

	switch (p_index) {
		case FILE_MENU_OPEN: {
		} break;

		case FILE_MENU_OPEN_IN_NEW_TAB: {
		} break;

		case FILE_MENU_OPEN_IN_NEW_WINDOW: {
		} break;

		case FILE_MENU_OPEN_EXTERNAL: {
		} break;

		case FILE_MENU_OPEN_IN_TERMINAL: {
		} break;

		case FILE_MENU_SHOW_IN_EXPLORER: {
		} break;

		case FILE_MENU_COPY_PATH: {
		} break;

		case FILE_MENU_UNDO: {
		} break;

		case FILE_MENU_REDO: {
		} break;

		case FILE_MENU_CUT: {
			FileSystemAccess::cut(targets);
		} break;

		case FILE_MENU_COPY: {
			FileSystemAccess::copy(targets);
		} break;

		case FILE_MENU_PASTE: {
			String path;
			path = targets[0];
			if (FileSystemAccess::file_exists(path)) {
				path = path.get_base_dir();
			}

			if (!path.is_empty()) {
				FileSystemAccess::paste(path);
			}
		} break;

		case FILE_MENU_REMOVE: {
			// FILE_MENU_DELETE
			// Remove the selected files.
			for (const auto &path : targets) {
				Error err = OS::get_singleton()->move_to_trash(path);
				if (err != OK) {
					print_line("Cannot remove: " + path); // TODO: hint
				}
			}
		} break;

		case FILE_MENU_RENAME: {
			// TODO
			// print_line(targets[0]);

			// // Set to_rename variable for callback execution.
			// to_rename.path = targets[0];
			// to_rename.is_file = FileSystemAccess::file_exists(to_rename.path);
			// edit_selected(to_rename);
		} break;

		case FILE_MENU_NEW: {
		} break;

		case FILE_MENU_NEW_FOLDER: {
		} break;

		case FILE_MENU_NEW_TEXTFILE: {
		} break;
	}
}

void FileContextMenu::add_file_item(int p_id) {
	switch (p_id) {
		case FILE_MENU_OPEN: {
			add_item(RTR("Open"), p_id, Key::O);
		} break;

		case FILE_MENU_OPEN_IN_NEW_TAB: {
			add_item(RTR("Open in New Ttab"), p_id, Key::B);
		} break;

		case FILE_MENU_OPEN_IN_NEW_WINDOW: {
			add_item(RTR("Open in New Window"), p_id, Key::E);
		} break;

		case FILE_MENU_OPEN_EXTERNAL: {
			// TODO
			// add_item(RTR("Open"), p_id, Key::O);
		} break;

		case FILE_MENU_OPEN_IN_TERMINAL: {
			add_item(RTR("Open in Terminal"), p_id); // Key::T
		} break;

		case FILE_MENU_SHOW_IN_EXPLORER: {
			add_item(RTR("Show in system explorer"), p_id);
		} break;

		case FILE_MENU_COPY_PATH: {
			add_item(RTR("Copy Path"), p_id, Key::A);
		} break;

		case FILE_MENU_UNDO: {
			add_item(RTR("Undo"), p_id, Key::U);
			// TODO
			// Ref<Shortcut> shortcut = APP_SHORTCUT("file_context/undo", RTR("Undo"), KeyModifierMask::CTRL + Key::Z);
			// set_item_shortcut(-1, shortcut);
		} break;

		case FILE_MENU_REDO: {
			add_item(RTR("Redo"), p_id, Key::R);
			// TODO
			// Ref<Shortcut> shortcut = APP_SHORTCUT("file_context/redo", RTR("Redo"), KeyModifierMask::CTRL + Key::Y);
			// set_item_shortcut(-1, shortcut);
		} break;

		case FILE_MENU_CUT: {
			add_item(RTR("Cut"), p_id, Key::T);
		} break;

		case FILE_MENU_COPY: {
			add_item(RTR("Copy"), p_id, Key::C);
		} break;

		case FILE_MENU_PASTE: {
			add_item(RTR("Paste"), p_id, Key::P);
		} break;

		case FILE_MENU_REMOVE: {
			// FILE_MENU_DELETE
			add_item(RTR("Delete"), p_id, Key::D);
		} break;

		case FILE_MENU_RENAME: {
			add_item(RTR("Rename"), p_id, Key::M);
		} break;

		case FILE_MENU_NEW: {
			add_item(RTR("Create New"), p_id, Key::W);
		} break;

		case FILE_MENU_NEW_FOLDER: {
			add_item(RTR("Folder..."), p_id, Key::F);
		} break;

		case FILE_MENU_NEW_TEXTFILE: {
			add_item(RTR("TextFile..."), p_id);
		} break;
	}
}

void FileContextMenu::add_custom_item(const String &p_label, int p_id_offset, Key p_accel) {
	int id = calculate_custom_id(p_id_offset);
	add_item(p_label, id, p_accel);
}

void FileContextMenu::add_custom_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id_offset, Key p_accel) {
	int id = calculate_custom_id(p_id_offset);
	add_icon_item(p_icon, p_label, id, p_accel);
}

void FileContextMenu::remove_custom_item(int p_id_offset) {
	int id = calculate_custom_id(p_id_offset);
	remove_item(id);
}

int FileContextMenu::calculate_custom_id(int p_id_offset) {
	return FILE_MENU_MAX + p_id_offset;
}

int FileContextMenu::calculate_original_id(int p_custom_id) {
	return (p_custom_id >= FILE_MENU_MAX) ? (p_custom_id - FILE_MENU_MAX) : -1;
}

void FileContextMenu::set_targets(const Vector<String> &p_targets) {
	targets.clear();
	for (const String &target : p_targets) {
		targets.push_back(target);
	}
}

Vector<String> FileContextMenu::get_targets() const {
	return targets;
}

void FileContextMenu::clear_targets() {
	targets.clear();
}

void FileContextMenu::_bind_methods() {
	ADD_SIGNAL(MethodInfo("custom_id_pressed", PropertyInfo(Variant::INT, "id")));
}

FileContextMenu::FileContextMenu() {
}

FileContextMenu::~FileContextMenu() {
}
