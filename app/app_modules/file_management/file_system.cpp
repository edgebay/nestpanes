#include "file_system.h"

#include "app/app_core/io/file_system_access.h"
#include "core/io/dir_access.h"

#include "app/settings/app_settings.h"

String FileSystemDirectory::get_name() const {
	return name;
}

String FileSystemDirectory::get_path() const {
	return path;
}

// TODO
// String FileSystemDirectory::get_directory_path() const {
// 	String directory_path = name;
// 	const FileSystemDirectory *dir = parent;
// 	while (dir) {
// 		String dir_name = dir->get_name();
// 		if (dir_name == COMPUTER_PATH) {
// 			break;
// 		} else if (dir_name == COMPUTER_PATH) { // TODO: drive
// 		}
// 		directory_path = dir->get_name().path_join(directory_path);
// 		dir = dir->get_parent();
// 	}
// 	return directory_path;
// }

Ref<Texture2D> FileSystemDirectory::get_icon() const {
	return icon;
}

bool FileSystemDirectory::is_hidden() const {
	return hidden;
}

bool FileSystemDirectory::is_scanned() const {
	return scanned;
}

FileSystemDirectory *FileSystemDirectory::get_parent() const {
	return parent;
}

int FileSystemDirectory::get_subdir_count() const {
	return subdirs.size();
}

FileSystemDirectory *FileSystemDirectory::get_subdir(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, subdirs.size(), nullptr);
	return subdirs[p_idx];
}

FileSystemDirectory *FileSystemDirectory::get_subdir_by_path(const String &p_path) const {
	for (FileSystemDirectory *dir : subdirs) {
		if (dir->get_path() == p_path) {
			return dir;
		}
	}

	return nullptr;
}

FileSystemDirectory *FileSystemDirectory::get_subdir_by_name(const String &p_name) const {
	for (FileSystemDirectory *dir : subdirs) {
		if (dir->get_name() == p_name) {
			return dir;
		}
	}

	return nullptr;
}

int FileSystemDirectory::get_file_count() const {
	return files.size();
}

const FileInfo *FileSystemDirectory::get_file(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, files.size(), nullptr);
	return files[p_idx];
}

const FileInfo *FileSystemDirectory::get_file_by_path(const String &p_path) const {
	for (FileInfo *fi : files) {
		if (fi->path == p_path) {
			return fi;
		}
	}

	return nullptr;
}

const FileInfo *FileSystemDirectory::get_file_by_name(const String &p_name) const {
	for (FileInfo *fi : files) {
		if (fi->name == p_name) {
			return fi;
		}
	}

	return nullptr;
}

void FileSystemDirectory::clear() {
	for (FileInfo *fi : files) {
		memdelete(fi);
	}
	files.clear();

	for (FileSystemDirectory *dir : subdirs) {
		memdelete(dir);
	}
	subdirs.clear();

	scanned = false;
}

void FileSystemDirectory::setup(FileSystemDirectory *p_parent, const String &p_name, const String &p_path, const Ref<Texture2D> &p_icon, bool p_hidden) {
	parent = p_parent;
	name = p_name;
	path = p_path;
	icon = p_icon;
	hidden = p_hidden;
}

FileSystemDirectory::FileSystemDirectory() {
}

FileSystemDirectory::~FileSystemDirectory() {
	clear();
}

/// FileSystem
bool FileSystem::is_valid_path(const String &p_path) {
	return p_path == COMPUTER_PATH || FileSystemAccess::path_exists(p_path);
}

bool FileSystem::is_valid_dir_path(const String &p_path) {
	return p_path == COMPUTER_PATH || FileSystemAccess::dir_exists(p_path);
}

FileSystemDirectory *FileSystem::get_root() const {
	return file_system_root;
}

FileSystemDirectory *FileSystem::get_dir(const String &p_path) const {
	if (p_path == file_system_root->get_path()) {
		return file_system_root;
	}

	String path = p_path.replace_char('\\', '/');
	if (path.ends_with("/")) {
		path = path.substr(0, path.length() - 1);
	}

	Vector<String> names = path.split("/");

	bool found = false;
	FileSystemDirectory *dir = file_system_root;
	for (int i = 0; i < names.size(); i++) {
		dir = dir->get_subdir_by_name(names[i]);
		if (!dir) {
			found = false;
			break;
		}
		found = true;
	}

	return found ? dir : nullptr;
}

const FileInfo *FileSystem::get_file(const String &p_path) const {
	String path = p_path.replace_char('\\', '/');

	Vector<String> names = p_path.split("/");
	if (names.size() < 2) {
		return nullptr;
	}

	String file = names[names.size() - 1];
	names.resize(names.size() - 1);

	bool found = false;
	const FileSystemDirectory *dir = file_system_root;
	for (int i = 0; i < names.size(); i++) {
		dir = dir->get_subdir_by_name(names[i]);
		if (!dir) {
			found = false;
			break;
		}
		found = true;
	}

	return found ? dir->get_file_by_name(file) : nullptr;
}

FileSystemDirectory *FileSystem::_create_dir(const String &p_path) const {
	if (p_path == file_system_root->get_path()) {
		return file_system_root;
	}

	if (!FileSystemAccess::dir_exists(p_path)) {
		return nullptr;
	}

	String path = p_path.replace_char('\\', '/');
	if (path.ends_with("/")) {
		path = path.substr(0, path.length() - 1);
	}

	Vector<String> names = path.split("/");

	bool created = false;
	bool found = false;
	FileSystemDirectory *dir = file_system_root;
	for (int i = 0; i < names.size(); i++) {
		String name = names[i];
		FileSystemDirectory *subdir = dir->get_subdir_by_name(name);
		if (subdir) {
			found = true;
			dir = subdir;
			continue;
		}

		String subdir_path = dir->get_path().path_join(name);
		FileInfo file_info;
		Error err = FileSystemAccess::get_file_info(subdir_path, file_info);
		if (err != OK) {
			created = false;
			found = false;
			break;
		}

		subdir = memnew(FileSystemDirectory);
		subdir->setup(dir, file_info.name, file_info.path, file_info.icon, file_info.is_hidden);
		dir->subdirs.push_back(subdir);

		dir = subdir;
		created = true;
	}
	if (!found && !created) {
		return nullptr;
	}

	return dir;
}

void FileSystem::_scan() {
	if (scan_tasks.is_empty()) {
		return;
	}

	ScanTask &task = scan_tasks.front()->get();
	String path = task.path;
	// print_line("scan: ", path);
	if (path == file_system_root->get_path()) {
		_scan_root();
		scan_tasks.pop_front();
		pending_paths.erase(path);
		return;
	}

	if (!task.dir) {
		task.dir = _create_dir(path);
		if (!task.dir) {
			scan_tasks.pop_front();
			pending_paths.erase(path);
			return;
		}
	}
	ERR_FAIL_NULL(task.dir);

	FileSystemDirectory *dir = task.dir;
	Error err = FAILED;
	bool changed = false;
	if (task.pending) {
		err = OK;
		task.pending = false;
	} else {
		if (dir->is_scanned()) {
			dir->clear();
			changed = true;
		}

		err = FileSystemAccess::list_dir_begin(path);
	}

	if (err == OK) {
		int items_processed = 0;
		while (true) {
			FileInfo file_info;
			err = FileSystemAccess::get_next(file_info);
			if (err != OK) {
				break;
			}

			if (file_info.name != "." && file_info.name != "..") {
				if (file_info.type == FOLDER_TYPE) {
					FileSystemDirectory *subdir = memnew(FileSystemDirectory);
					subdir->setup(dir, file_info.name, file_info.path, file_info.icon, file_info.is_hidden);
					dir->subdirs.push_back(subdir);
				} else {
					FileInfo *fi = memnew(FileInfo);
					*fi = file_info;
					dir->files.push_back(fi);
				}
				changed = true;
			}

			items_processed++;
			if (items_processed >= max_items_per_step) {
				task.pending = true;
				break;
			}
		}

		if (!task.pending) {
			FileSystemAccess::list_dir_end();
		}
	}

	if (!task.pending) {
		dir->scanned = true;

		scan_tasks.pop_front();
		pending_paths.erase(path);
	}

	if (changed) {
		emit_signal(SNAME("file_system_changed"), dir);
	}
}

void FileSystem::_scan_root() {
	ERR_FAIL_NULL(file_system_root);

	FileSystemDirectory *dir = file_system_root;
	if (dir->is_scanned()) {
		dir->clear();
	}

	List<FileInfo> drives;
	Error err = FileSystemAccess::list_drives(drives);
	if (err != OK) {
		return;
	}

	for (const FileInfo &file_info : drives) {
		FileSystemDirectory *subdir = memnew(FileSystemDirectory);
		subdir->setup(dir, file_info.name, file_info.path, file_info.icon, file_info.is_hidden);
		dir->subdirs.push_back(subdir);
	}

	dir->scanned = true;
	emit_signal(SNAME("file_system_changed"), dir);
}

void FileSystem::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			_scan();
		} break;
	}
}

bool FileSystem::is_scanning() const {
	return !scan_tasks.is_empty();
}

void FileSystem::scan(const String &p_path, bool p_update) {
	scan(Vector<String>({ p_path }), p_update);
}

void FileSystem::scan(const Vector<String> &p_paths, bool p_update) {
	for (const String &path : p_paths) {
		if (!is_valid_dir_path(path)) {
			continue;
		}

		if (pending_paths.has(path)) {
			continue;
		}

		FileSystemDirectory *dir = get_dir(path);
		if (!p_update && dir && dir->is_scanned()) {
			continue;
		}

		pending_paths.insert(path);

		ScanTask task;
		task.path = path;
		task.dir = dir;
		scan_tasks.push_back(task);
	}
}

void FileSystem::_bind_methods() {
	ADD_SIGNAL(MethodInfo("file_system_changed", PropertyInfo(Variant::OBJECT, "dir")));
}

FileSystem::FileSystem() {
	file_system_root = memnew(FileSystemDirectory);
	file_system_root->setup(nullptr, COMPUTER_PATH, COMPUTER_PATH, FileSystemAccess::get_icon(COMPUTER_PATH), false);
	scan(COMPUTER_PATH);

	APP_SHORTCUT("file_management/copy_path", TTRC("Copy Path"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::C);

	APP_SHORTCUT("file_management/show_in_explorer", TTRC("Open in File Manager"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::R);
	APP_SHORTCUT("file_management/open_in_external_program", TTRC("Open in External Program"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::E);
	APP_SHORTCUT("file_management/open_in_terminal", TTRC("Open in Terminal"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::T);

	APP_SHORTCUT("file_management/undo", TTRC("Undo"), KeyModifierMask::CMD_OR_CTRL | Key::Z);
	APP_SHORTCUT("file_management/redo", TTRC("Redo"), KeyModifierMask::CMD_OR_CTRL | Key::Y);

	APP_SHORTCUT("file_management/new_folder", TTRC("New Folder..."), Key::NONE);
	APP_SHORTCUT("file_management/new_textfile", TTRC("New TextFile..."), Key::NONE);

	APP_SHORTCUT("file_management/cut", TTRC("Cut"), KeyModifierMask::CMD_OR_CTRL | Key::X);
	APP_SHORTCUT("file_management/copy", TTRC("Copy"), KeyModifierMask::CMD_OR_CTRL | Key::C);
	APP_SHORTCUT("file_management/paste", TTRC("Paste"), KeyModifierMask::CMD_OR_CTRL | Key::V);

	APP_SHORTCUT("file_management/rename", TTRC("Rename..."), Key::F2);
	APP_SHORTCUT("file_management/delete", TTRC("Delete"), Key::KEY_DELETE);

	set_process(true);
}

FileSystem::~FileSystem() {
	if (file_system_root) {
		memdelete(file_system_root);
	}
	file_system_root = nullptr;
}
