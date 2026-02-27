#include "file_system.h"

#include "app/app_core/io/file_system_access.h"
#include "core/io/dir_access.h"

#include "app/settings/app_settings.h"

#include <ctime>

const FileInfo &FileSystemDirectory::get_info() const {
	return info;
}

String FileSystemDirectory::get_name() const {
	return info.name;
}

String FileSystemDirectory::get_path() const {
	return info.path;
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
	return info.icon;
}

StringName FileSystemDirectory::get_type() const {
	return info.type;
}

uint64_t FileSystemDirectory::get_size() const {
	return info.size;
}

uint64_t FileSystemDirectory::get_creation_time() const {
	return info.creation_time;
}

uint64_t FileSystemDirectory::get_modified_time() const {
	return info.modified_time;
}

bool FileSystemDirectory::is_hidden() const {
	return info.hidden;
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
		String path = dir->get_path();
#ifdef WINDOWS_ENABLED
		if (path == p_path || path.to_lower() == p_path.to_lower()) {
			// TODO: Sync the name case to the actual filesystem name.
#else
		if (path == p_path) {
#endif
			return dir;
		}
	}

	return nullptr;
}

FileSystemDirectory *FileSystemDirectory::get_subdir_by_name(const String &p_name) const {
	for (FileSystemDirectory *dir : subdirs) {
		String name = dir->get_name();
#ifdef WINDOWS_ENABLED
		if (name == p_name || name.to_lower() == p_name.to_lower()) {
			// TODO: Sync the name case to the actual filesystem name.
#else
		if (name == p_name) {
#endif
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
		// TODO: Sync the name case to the actual filesystem name.
		if (fi->path == p_path) {
			return fi;
		}
	}

	return nullptr;
}

const FileInfo *FileSystemDirectory::get_file_by_name(const String &p_name) const {
	for (FileInfo *fi : files) {
		// TODO: Sync the name case to the actual filesystem name.
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

void FileSystemDirectory::setup(FileSystemDirectory *p_parent, const FileInfo &p_info) {
	ERR_FAIL_NULL(p_parent);
	ERR_FAIL_COND(!FileSystemAccess::is_dir_type(p_info));

	parent = p_parent;
	info = p_info;
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

// String FileSystem::fix_path(const String &p_path) {
// 	String path;
// 	path = p_path.simplify_path();
// 	// path = p_path.replace_char('/', '\\'); // TODO
// 	return path;
// }

String FileSystem::parse_time(uint64_t p_timestamp, bool p_use_local_time) {
	String time = "";

	time_t t = (time_t)p_timestamp;
	struct tm lt;
#ifdef WINDOWS_ENABLED
	if (p_use_local_time) {
		localtime_s(&lt, &t);
	} else {
		gmtime_s(&lt, &t);
	}
#else
	if (p_use_local_time) {
		localtime_r(&t, &lt);
	} else {
		gmtime_r(&t, &lt);
	}
#endif
	time = vformat("%04d/%02d/%02d %02d:%02d",
			(int)(1900 + lt.tm_year),
			(int)(lt.tm_mon + 1),
			(int)(lt.tm_mday),
			(int)(lt.tm_hour),
			(int)(lt.tm_min));

	return time;
}

String FileSystem::parse_size(uint64_t p_bytes) {
	String size = "";

	// size = vformat("%d B", p_bytes);
	String bytes = vformat("%d", p_bytes);
	int len = bytes.length();
	for (int i = 0; i < len; i++) {
		if (i > 0 && (len - i) % 3 == 0) {
			size += ",";
		}
		size += bytes[i];
	}
	size += " B";

	return size;
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

FileSystemDirectory *FileSystem::_create_dir(const String &p_path) {
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

	Vector<String> paths;

	bool created = false;
	bool found = false;
	FileSystemDirectory *dir = file_system_root;
	for (int i = 0; i < names.size(); i++) {
		String name = names[i];
		FileSystemDirectory *subdir = dir->get_subdir_by_name(name);
		// print_line("find: ", name, subdir, dir->get_path());
		if (subdir) {
			found = true;
			dir = subdir;
			continue;
		}

		String subdir_path;
		if (dir == file_system_root) {
			subdir_path = name;
		} else {
			subdir_path = dir->get_path().path_join(name);
		}
		FileInfo file_info;
		Error err = FileSystemAccess::get_file_info(subdir_path, file_info);
		// print_line("fi: ", subdir_path, err);
		if (err != OK) {
			created = false;
			found = false;
			break;
		}

		subdir = memnew(FileSystemDirectory);
		subdir->setup(dir, file_info);
		dir->subdirs.push_back(subdir);

		dir->scanned = true;
		paths.push_back(dir->get_path());

		dir = subdir;
		created = true;
	}
	if (!found && !created) {
		return nullptr;
	}

	// print_line("create, scan: ", paths);
	scan(paths, true);

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

	Error err = FAILED;
	bool changed = false;
	if (task.pending) {
		err = OK;
		task.pending = false;
		ERR_FAIL_NULL(task.dir);
	} else {
		FileSystemDirectory *old_dir = get_dir(path);
		if (!old_dir) {
			old_dir = _create_dir(path);
			// print_line("create dir: ", old_dir, path);
			if (!old_dir) {
				scan_tasks.pop_front();
				pending_paths.erase(path);
				ERR_FAIL();
			}
			changed = true;
		} else {
			FileSystemDirectory *parent = old_dir->get_parent();
			task.update_dir = memnew(FileSystemDirectory);
			if (!parent || !task.update_dir) {
				scan_tasks.pop_front();
				pending_paths.erase(path);
				ERR_FAIL();
			}
			// print_line("create update dir: ", task.update_dir);
		}
		task.dir = old_dir;

		err = FileSystemAccess::list_dir_begin(path);
		// print_line("list begin: ", err);
	}

	if (err == OK) {
		FileSystemDirectory *dir = task.update_dir ? task.update_dir : task.dir;
		int items_processed = 0;
		while (true) {
			FileInfo file_info;
			err = FileSystemAccess::get_next(file_info);
			if (err != OK) {
				break;
			}
			// print_line("next: ", file_info.name);

			if (file_info.name != "." && file_info.name != "..") {
				if (FileSystemAccess::is_dir_type(file_info)) {
					FileSystemDirectory *subdir = memnew(FileSystemDirectory);
					subdir->setup(dir, file_info);
					dir->subdirs.push_back(subdir);
				} else {
					FileInfo *fi = memnew(FileInfo);
					*fi = file_info;
					dir->files.push_back(fi);
				}
				// print_line("files: ", dir->subdirs.size(), dir->files.size());

				if (dir == task.dir) {
					changed = true;
				}
			}

			items_processed++;
			if (items_processed >= max_items_per_step) {
				task.pending = true;
				break;
			}
		}

		if (!task.pending) {
			FileSystemAccess::list_dir_end();

			dir->scanned = true;
			if (dir == task.update_dir) {
				changed = true;

				_replace_dir(task.dir, task.update_dir);
			}
		}
	}

	// print_line("end: ", changed, task.pending, err);
	if (!task.pending) {
		scan_tasks.pop_front();
		pending_paths.erase(path);
	}

	if (changed) {
		emit_signal(SNAME("file_system_changed"), path);
	}
}

void FileSystem::_scan_root() {
	ERR_FAIL_NULL(file_system_root);

	List<FileInfo> drives;
	Error err = FileSystemAccess::list_drives(drives);
	if (err != OK) {
		return;
	}

	FileSystemDirectory *dir = file_system_root;
	if (dir->is_scanned()) {
// #define TEST_CODE
#ifdef TEST_CODE
		if (drives.size() == dir->subdirs.size()) {
			FileInfo file_info;
			file_info.name = "Z:";
			file_info.path = file_info.name;
			file_info.icon = FileSystemAccess::get_icon(COMPUTER_PATH);
			file_info.type = DRIVE_TYPE;
			drives.push_back(file_info);
		}
#endif

		// FileSystemDirectory *update_dir = memnew(FileSystemDirectory);

		// Update dir->subdirs.
		Vector<FileSystemDirectory *> remove_dirs;
		for (FileSystemDirectory *subdir : dir->subdirs) {
			bool found = false;
			String path = subdir->get_path();

			for (List<FileInfo>::Element *E = drives.front(); E;) {
				List<FileInfo>::Element *N = E->next();
				if (E->get().path == path) {
					// for (FileSystemDirectory *dir : subdir->subdirs) {
					// 	dir->parent = update_dir;
					// 	update_dir->subdirs.push_back(dir);
					// }

					drives.erase(E);
					found = true;
					break;
				}
				E = N;
			}
			// print_line("found subdir: ", found, path);
			if (!found) {
				remove_dirs.push_back(subdir);
			}
		}

		for (FileSystemDirectory *subdir : remove_dirs) {
			dir->subdirs.erase(subdir);
			memdelete(subdir);
		}

		// dir->subdirs.clear();
		// memdelete(dir);

		// file_system_root = update_dir;
		// dir = file_system_root;

		// Note: Then add the remaining drives to the dir.
	}

	for (const FileInfo &file_info : drives) {
		FileSystemDirectory *subdir = memnew(FileSystemDirectory);
		subdir->setup(dir, file_info);
		dir->subdirs.push_back(subdir);
	}

	dir->scanned = true;
	emit_signal(SNAME("file_system_changed"), dir->get_path());
}

void FileSystem::_replace_dir(FileSystemDirectory *p_old, FileSystemDirectory *p_new) {
	FileSystemDirectory *old_dir = p_old;
	FileSystemDirectory *new_dir = p_new;
	// print_line("replace: ", old_dir, new_dir);

	FileSystemDirectory *parent = old_dir->get_parent();
	new_dir->setup(parent, old_dir->get_info()); // TODO: Update info

	// Move subdir->subdirs and subdir->files.
	Vector<FileSystemDirectory *> to_dirs = new_dir->subdirs;
	for (int i = 0; i < old_dir->subdirs.size(); i++) {
		bool found = false;
		FileSystemDirectory *subdir = old_dir->subdirs[i];
		String path = subdir->get_path();

		// print_line("move sub subdirs: ", i, path, to_dirs.size());
		for (int j = 0; j < to_dirs.size(); j++) {
			FileSystemDirectory *to_subdir = to_dirs[j];
			String to_path = to_subdir->get_path();
			// print_line("compare path: ", path, to_path);

#ifdef WINDOWS_ENABLED
			if (to_path == path || to_path.to_lower() == path.to_lower()) {
				// TODO: Sync the name case to the actual filesystem name.
				// if (to_path != path) {
				// 	print_line("fix path: ", path, to_path);
				// }
#else
			if (to_path == path) {
#endif
				// print_line("found subdirs: ", j);

				for (FileSystemDirectory *dir : subdir->subdirs) {
					dir->parent = to_subdir;
					to_subdir->subdirs.push_back(dir);
				}
				if (!subdir->files.is_empty()) {
					to_subdir->files = subdir->files;
				}

				// Clear the vector to avoid deleting the subdirs or files
				subdir->subdirs.clear();
				subdir->files.clear();

				to_dirs.remove_at(j);
				found = true;
				break;
			}
		}
	}

	// Replace in parent.
	for (int i = 0; i < parent->subdirs.size(); i++) {
		if (parent->subdirs[i] == old_dir) {
			// print_line("update dir set: ", i, new_dir);
			parent->subdirs.set(i, new_dir);
			break;
		}
	}
	memdelete(old_dir);
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
	for (String path : p_paths) {
		path = path.simplify_path();
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
		scan_tasks.push_back(task);
	}
}

void FileSystem::_bind_methods() {
	ADD_SIGNAL(MethodInfo("file_system_changed", PropertyInfo(Variant::STRING, "path")));
}

FileSystem::FileSystem() {
	file_system_root = memnew(FileSystemDirectory);

	FileInfo &file_info = file_system_root->info;
	file_info.name = COMPUTER_PATH;
	file_info.path = COMPUTER_PATH;
	file_info.icon = FileSystemAccess::get_icon(COMPUTER_PATH);
	file_info.type = ROOT_TYPE;

	APP_SHORTCUT("file_management/copy_path", RTR("Copy Path"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::C);

	APP_SHORTCUT("file_management/show_in_explorer", RTR("Show in System Explorer"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::R);
	APP_SHORTCUT("file_management/open_in_external_program", RTR("Open in External Program"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::E);
	APP_SHORTCUT("file_management/open_in_terminal", RTR("Open in Terminal"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::T);

	APP_SHORTCUT("file_management/undo", RTR("Undo"), KeyModifierMask::CMD_OR_CTRL | Key::Z);
	APP_SHORTCUT("file_management/redo", RTR("Redo"), KeyModifierMask::CMD_OR_CTRL | Key::Y);

	APP_SHORTCUT("file_management/new_folder", RTR("New Folder..."), Key::NONE);
	APP_SHORTCUT("file_management/new_textfile", RTR("New TextFile..."), Key::NONE);

	APP_SHORTCUT("file_management/cut", RTR("Cut"), KeyModifierMask::CMD_OR_CTRL | Key::X);
	APP_SHORTCUT("file_management/copy", RTR("Copy"), KeyModifierMask::CMD_OR_CTRL | Key::C);
	APP_SHORTCUT("file_management/paste", RTR("Paste"), KeyModifierMask::CMD_OR_CTRL | Key::V);

	APP_SHORTCUT("file_management/rename", RTR("Rename..."), Key::F2);
	APP_SHORTCUT("file_management/delete", RTR("Delete"), Key::KEY_DELETE);

	set_process(true);
	scan(COMPUTER_PATH);
}

FileSystem::~FileSystem() {
	if (file_system_root) {
		memdelete(file_system_root);
	}
	file_system_root = nullptr;
}
