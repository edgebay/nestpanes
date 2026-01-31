#include "file_system.h"

#include "app/app_core/io/file_system_access.h"

String FileSystemDirectory::get_name() const {
	return name;
}

String FileSystemDirectory::get_path() const {
	return path;
}

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

FileSystemDirectory *FileSystemDirectory::get_subdir(const String &p_name) const {
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

const FileInfo *FileSystemDirectory::get_file(const String &p_name) const {
	for (FileInfo *fi : files) {
		if (fi->name == p_name) {
			return fi;
		}
	}

	return nullptr;
}

void FileSystemDirectory::scan(bool p_scan_subdirs) {
	if (scanned) {
		clear();
	}

	List<FileInfo> dir_list;
	List<FileInfo> file_list;
	Error err = FileSystemAccess::list_file_infos(path, dir_list, file_list);
	if (err != OK) {
		return;
	}

	// Create all items for the files in the subdirectory.
	for (const FileInfo &file_info : dir_list) {
		FileSystemDirectory *subdir = memnew(FileSystemDirectory);
		subdir->setup(this, file_info.name, file_info.path, file_info.icon, file_info.is_hidden, p_scan_subdirs);
		subdirs.push_back(subdir);
	}
	for (const FileInfo &file_info : file_list) {
		FileInfo *fi = memnew(FileInfo);
		*fi = file_info;
		files.push_back(fi);
	}

	scanned = true;
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

void FileSystemDirectory::setup(FileSystemDirectory *p_parent, const String &p_name, const String &p_path, const Ref<Texture2D> &p_icon, bool p_hidden, bool p_scan) {
	parent = p_parent;
	name = p_name;
	path = p_path;
	icon = p_icon;
	hidden = p_hidden;

	if (p_scan) {
		scan();
	}
}

FileSystemDirectory::FileSystemDirectory() {
}

FileSystemDirectory::~FileSystemDirectory() {
	clear();
}

/// FileSystemDirectory
FileSystemDirectory *FileSystem::get_root() const {
	return file_system_root;
}

FileSystemDirectory *FileSystem::get_dir(const String &p_path) const {
	String path = p_path.replace_char('\\', '/');
	if (path.ends_with("/")) {
		path = path.substr(0, path.length() - 1);
	}

	Vector<String> names = path.split("/");

	bool found = false;
	FileSystemDirectory *dir = file_system_root;
	for (int i = 0; i < names.size(); i++) {
		dir = dir->get_subdir(names[i]);
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
		dir = dir->get_subdir(names[i]);
		if (!dir) {
			found = false;
			break;
		}
		found = true;
	}

	return found ? dir->get_file(file) : nullptr;
}

void FileSystem::_update(FileSystemDirectory *p_dir) {
	print_line("fs update: ", p_dir->get_path());
	p_dir->scan();
	emit_signal(SNAME("file_system_changed"), p_dir);
}

void FileSystem::update(const String &p_path) {
	// TODO: Use a list to store the paths and handle them in NOTIFICATION_PROCESS?
	FileSystemDirectory *dir = get_dir(p_path);
	if (dir) {
		callable_mp(this, &FileSystem::_update).call_deferred(dir);
	}
}

void FileSystem::update(FileSystemDirectory *p_dir) {
	callable_mp(this, &FileSystem::_update).call_deferred(p_dir);
}

void FileSystem::update_file_system() {
	// TODO
}

void FileSystem::_bind_methods() {
	ADD_SIGNAL(MethodInfo("file_system_changed", PropertyInfo(Variant::OBJECT, "dir")));
}

FileSystem::FileSystem() {
	file_system_root = memnew(FileSystemDirectory);
	file_system_root->setup(nullptr, COMPUTER_PATH, COMPUTER_PATH, FileSystemAccess::get_icon(COMPUTER_PATH), false, true);
}
