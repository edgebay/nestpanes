#pragma once

#include "core/object/object.h"
#include "scene/main/node.h"

#include "app/app_core/types/file_info.h"

class FileSystemDirectory : public Object {
	GDCLASS(FileSystemDirectory, Object);

private:
	// TODO: use FileInfo?
	String name;
	String path;
	Ref<Texture2D> icon;
	bool hidden = false;

	bool scanned = false;

	FileSystemDirectory *parent = nullptr;
	Vector<FileSystemDirectory *> subdirs;
	Vector<FileInfo *> files;

protected:
	friend class FileSystem;

	void clear();

	void setup(FileSystemDirectory *p_parent,
			const String &p_name,
			const String &p_path,
			const Ref<Texture2D> &p_icon,
			bool p_hidden = false);

public:
	String get_name() const;
	String get_path() const;
	// TODO
	// String get_directory_path() const;
	Ref<Texture2D> get_icon() const;
	bool is_hidden() const;
	bool is_scanned() const;

	FileSystemDirectory *get_parent() const;

	int get_subdir_count() const;
	FileSystemDirectory *get_subdir(int p_idx) const;
	FileSystemDirectory *get_subdir_by_path(const String &p_path) const;
	FileSystemDirectory *get_subdir_by_name(const String &p_name) const;
	int get_file_count() const;
	const FileInfo *get_file(int p_idx) const;
	const FileInfo *get_file_by_path(const String &p_path) const;
	const FileInfo *get_file_by_name(const String &p_name) const;

	FileSystemDirectory();
	~FileSystemDirectory();
};

class FileSystem : public Node {
	GDCLASS(FileSystem, Node);

	_THREAD_SAFE_CLASS_

private:
	struct ScanTask {
		String path = "";
		FileSystemDirectory *dir = nullptr;
		bool pending = false;
	};

	List<ScanTask> scan_tasks;
	HashSet<String> pending_paths;
	int max_items_per_step = 100;

	FileSystemDirectory *file_system_root = nullptr;

	FileSystemDirectory *_create_dir(const String &p_path) const;
	void _scan();
	void _scan_root();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	static bool is_valid_path(const String &p_path);
	static bool is_valid_dir_path(const String &p_path);
	// TODO
	// static String normalize_path(const String &p_path);

	FileSystemDirectory *get_root() const;
	FileSystemDirectory *get_dir(const String &p_path) const;
	const FileInfo *get_file(const String &p_path) const;

	bool is_scanning() const;
	void scan(const String &p_path, bool p_update = false);
	void scan(const Vector<String> &p_paths, bool p_update = false);

	FileSystem();
	virtual ~FileSystem();
};
