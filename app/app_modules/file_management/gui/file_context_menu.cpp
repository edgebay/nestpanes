#include "file_context_menu.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

#include "app/app_modules/file_management/gui/file_pane.h"
#include "app/gui/app_tab_container.h"
#include "app/gui/layout_manager.h"

void FileContextMenu::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			connect(SceneStringName(id_pressed), callable_mp(this, &FileContextMenu::_id_pressed));
		} break;
	}
}

void FileContextMenu::_id_pressed(int p_index) {
	print_line("ctx menu pressed: ", p_index, targets);
	if (p_index >= FILE_MENU_MAX) {
		emit_signal(SNAME("custom_id_pressed"), p_index);
		return;
	}

	switch (p_index) {
			// case FILE_MENU_OPEN: { // Handle it in the parent.
			// } break;

		case FILE_MENU_OPEN_IN_NEW_TAB: {
			String path = targets.is_empty() ? "" : targets[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}

			Control *c = Object::cast_to<Control>(get_parent());
			AppTabContainer *tab_container = AppTabContainer::get_control_parent_tab_container(c);
			if (!tab_container) {
				break;
			}

			if (FileSystemAccess::dir_exists(path)) {
				LayoutManager::get_singleton()->create_new_tab(FilePane::get_class_static(),
						tab_container,
						callable_mp(this, &FileContextMenu::_on_new_file_pane).bind(path));
			} else if (FileSystemAccess::file_exists(path)) {
				// TODO: txt file
			}
		} break;

		case FILE_MENU_OPEN_IN_NEW_WINDOW: {
			List<String> args;

			// TODO: args
			// args.push_back("--path");
			// args.push_back(path);

			Error err = OS::get_singleton()->create_instance(args);
			if (err != OK) {
				ERR_PRINT(vformat("Failed to start an app instance, error code %d.", err));
				return;
			}
		} break;

		case FILE_MENU_OPEN_EXTERNAL: {
			String path = targets.is_empty() ? "" : targets[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}

			OS::get_singleton()->shell_open(path);
		} break;

		case FILE_MENU_OPEN_IN_TERMINAL: {
			String fpath = targets.is_empty() ? "" : targets[0];
			if (!FileSystemAccess::path_exists(fpath)) {
				break;
			}

			Vector<String> terminal_emulators;
			const String terminal_emulator_setting = ""; // TODO: EDITOR_GET("filesystem/external_programs/terminal_emulator");
			if (terminal_emulator_setting.is_empty()) {
				// Figure out a default terminal emulator to use.
#if defined(WINDOWS_ENABLED)
				// Default to PowerShell as done by Windows 10 and later.
				terminal_emulators.push_back("powershell");
#endif
			} else {
				// Use the user-specified terminal.
				terminal_emulators.push_back(terminal_emulator_setting);
			}

			String terminal_emulator_flags = ""; // TODO: EDITOR_GET("filesystem/external_programs/terminal_emulator_flags");
			String arguments = terminal_emulator_flags;
			if (arguments.is_empty()) {
				// NOTE: This default value is ignored further below if the terminal executable is `powershell` or `cmd`,
				// due to these terminals requiring nonstandard syntax to start in a specified folder.
				arguments = "{directory}";
			}

			// On Windows and macOS, the first (and only) terminal emulator in the list is always available.
			String chosen_terminal_emulator = terminal_emulators[0];

			List<String> terminal_emulator_args; // Required for `execute()`, as it doesn't accept `Vector<String>`.
			bool append_default_args = true;

#ifdef WINDOWS_ENABLED
			// Prepend default arguments based on the terminal emulator name.
			// Use `String.get_basename().to_lower()` to handle Windows' case-insensitive paths
			// with optional file extensions for executables in `PATH`.
			if (chosen_terminal_emulator.get_basename().to_lower() == "powershell") {
				terminal_emulator_args.push_back("-noexit");
				terminal_emulator_args.push_back("-command");
				terminal_emulator_args.push_back("cd '{directory}'");
				append_default_args = false;
			} else if (chosen_terminal_emulator.get_basename().to_lower() == "cmd") {
				terminal_emulator_args.push_back("/K");
				terminal_emulator_args.push_back("cd /d {directory}");
				append_default_args = false;
			}
#endif

			Vector<String> arguments_array = arguments.split(" ");
			for (const String &argument : arguments_array) {
				if (!append_default_args && argument == "{directory}") {
					// Prevent appending a `{directory}` placeholder twice when using powershell or cmd.
					// This allows users to enter the path to cmd or PowerShell in the custom terminal emulator path,
					// and make it work without having to enter custom arguments.
					continue;
				}
				terminal_emulator_args.push_back(argument);
			}

			const bool is_directory = FileSystemAccess::dir_exists(fpath);
			for (String &terminal_emulator_arg : terminal_emulator_args) {
				if (is_directory) {
					terminal_emulator_arg = terminal_emulator_arg.replace("{directory}", fpath);
				} else {
					terminal_emulator_arg = terminal_emulator_arg.replace("{directory}", fpath.get_base_dir());
				}
			}

			if (OS::get_singleton()->is_stdout_verbose()) {
				// Print full command line to help with troubleshooting.
				String command_string = chosen_terminal_emulator;
				for (const String &arg : terminal_emulator_args) {
					command_string += " " + arg;
				}
				print_line("Opening terminal emulator:", command_string);
			}

			const Error err = OS::get_singleton()->create_process(chosen_terminal_emulator, terminal_emulator_args, nullptr, true);
			if (err != OK) {
				String args_string;
				for (const String &terminal_emulator_arg : terminal_emulator_args) {
					args_string += terminal_emulator_arg;
				}
				// TODO: ERR_PRINT_ED(vformat(RTR("Couldn't run external terminal program (error code %d): %s %s\nCheck `filesystem/external_programs/terminal_emulator` and `filesystem/external_programs/terminal_emulator_flags` in the Editor Settings."), err, chosen_terminal_emulator, args_string));
				ERR_PRINT_ED(vformat(RTR("Couldn't run external terminal program (error code %d): %s %s."), err, chosen_terminal_emulator, args_string));
			}
		} break;

		case FILE_MENU_SHOW_IN_EXPLORER: {
			String path = targets.is_empty() ? "" : targets[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}
			OS::get_singleton()->shell_show_in_file_manager(path, true);
		} break;

		case FILE_MENU_COPY_PATH: {
			String path = targets.is_empty() ? "" : targets[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}
			DisplayServer::get_singleton()->clipboard_set(path);
		} break;

		case FILE_MENU_UNDO: {
		} break;

		case FILE_MENU_REDO: {
		} break;

		case FILE_MENU_CUT: {
			if (targets.is_empty()) {
				break;
			}
			FileSystemAccess::cut(targets);
		} break;

		case FILE_MENU_COPY: {
			if (targets.is_empty()) {
				break;
			}
			FileSystemAccess::copy(targets);
		} break;

		case FILE_MENU_PASTE: { // Handle it in the parent.
		} break;

		case FILE_MENU_REMOVE: {
			// FILE_MENU_DELETE
			HashSet<String> dirs;
			for (const auto &path : targets) {
				Error err = OS::get_singleton()->move_to_trash(path);
				if (err == OK) {
					String dir = path.get_base_dir();
					if (!dirs.has(dir)) {
						dirs.insert(dir);
					}
				} else {
					print_line("Cannot remove: " + path); // TODO: hint
				}
			}

			if (file_system) {
				for (const String &dir : dirs) {
					file_system->scan(dir, true);
				}
			}
		} break;

			// case FILE_MENU_RENAME: { // Handle it in the parent.
			// } break;

			// case FILE_MENU_NEW: {
			// } break;

			// case FILE_MENU_NEW_FOLDER: { // Handle it in the parent.
			// } break;

			// case FILE_MENU_NEW_TEXTFILE: { // Handle it in the parent.
			// } break;
	}
}

void FileContextMenu::_on_new_file_pane(PaneBase *p_pane, const String &p_path) {
	FilePane *pane = Object::cast_to<FilePane>(p_pane);
	if (pane) {
		pane->set_path(p_path);
	}
}

void FileContextMenu::add_file_item(int p_id) {
	switch (p_id) {
		case FILE_MENU_OPEN: {
			add_item(RTR("Open"), p_id); // , Key::O);
		} break;

		case FILE_MENU_OPEN_IN_NEW_TAB: {
			add_item(RTR("Open in New Tab"), p_id); // , Key::B);
		} break;

		case FILE_MENU_OPEN_IN_NEW_WINDOW: {
			add_item(RTR("Open in New Window"), p_id); // , Key::E);
		} break;

		case FILE_MENU_OPEN_EXTERNAL: {
			// TODO
			// add_item(RTR("Open"), p_id); // , Key::O);
		} break;

		case FILE_MENU_OPEN_IN_TERMINAL: {
			add_item(RTR("Open in Terminal"), p_id); // Key::T
		} break;

		case FILE_MENU_SHOW_IN_EXPLORER: {
			add_item(RTR("Show in System Explorer"), p_id);
		} break;

		case FILE_MENU_COPY_PATH: {
			add_item(RTR("Copy Path"), p_id); // , Key::A);
		} break;

		case FILE_MENU_UNDO: {
			add_item(RTR("Undo"), p_id); // , Key::U);
			// TODO
			// Ref<Shortcut> shortcut = APP_SHORTCUT("file_context/undo", RTR("Undo"), KeyModifierMask::CTRL + Key::Z);
			// set_item_shortcut(-1, shortcut);
		} break;

		case FILE_MENU_REDO: {
			add_item(RTR("Redo"), p_id); // , Key::R);
			// TODO
			// Ref<Shortcut> shortcut = APP_SHORTCUT("file_context/redo", RTR("Redo"), KeyModifierMask::CTRL + Key::Y);
			// set_item_shortcut(-1, shortcut);
		} break;

		case FILE_MENU_CUT: {
			add_item(RTR("Cut"), p_id); // , Key::T);
		} break;

		case FILE_MENU_COPY: {
			add_item(RTR("Copy"), p_id); // , Key::C);
		} break;

		case FILE_MENU_PASTE: {
			add_item(RTR("Paste"), p_id); // , Key::P);
			if (!FileSystemAccess::can_paste()) {
				set_item_disabled(-1, true);
			}
		} break;

		case FILE_MENU_REMOVE: {
			// FILE_MENU_DELETE
			add_item(RTR("Delete"), p_id); // , Key::D);
		} break;

		case FILE_MENU_RENAME: {
			add_item(RTR("Rename"), p_id); // , Key::M);
		} break;

		case FILE_MENU_NEW: {
			add_item(RTR("Create New"), p_id); // , Key::W);
		} break;

		case FILE_MENU_NEW_FOLDER: {
			add_item(RTR("Folder..."), p_id); // , Key::F);
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

void FileContextMenu::file_option(int p_option) {
	_id_pressed(p_option);
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

void FileContextMenu::set_file_system(FileSystem *p_file_system) {
	file_system = p_file_system;
}

FileSystem *FileContextMenu::get_file_system() const {
	return file_system;
}

void FileContextMenu::_bind_methods() {
	ADD_SIGNAL(MethodInfo("custom_id_pressed", PropertyInfo(Variant::INT, "id")));
}

FileContextMenu::FileContextMenu() {
}

FileContextMenu::~FileContextMenu() {
}
