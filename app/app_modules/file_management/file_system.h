#pragma once

#include "core/object/object.h"

#include "app/app_core/types/file_info.h"

class FileSystemDirectory : public Object {
	GDCLASS(FileSystemDirectory, Object);

private:
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

	void scan(bool p_scan_subdirs = false);
	void clear();

	void setup(FileSystemDirectory *p_parent,
			const String &p_name,
			const String &p_path,
			const Ref<Texture2D> &p_icon,
			bool p_hidden = false,
			bool p_scan = false);

public:
	String get_name() const;
	String get_path() const;
	// String get_directory_path() const;
	Ref<Texture2D> get_icon() const;
	bool is_hidden() const;
	bool is_scanned() const;

	FileSystemDirectory *get_parent() const;

	int get_subdir_count() const;
	FileSystemDirectory *get_subdir(int p_idx) const;
	FileSystemDirectory *get_subdir(const String &p_name) const;
	int get_file_count() const;
	const FileInfo *get_file(int p_idx) const;
	const FileInfo *get_file(const String &p_name) const;

	FileSystemDirectory();
	~FileSystemDirectory();
};

class FileSystem : public Object {
	GDCLASS(FileSystem, Object);

	_THREAD_SAFE_CLASS_

private:
	FileSystemDirectory *file_system_root = nullptr;

	void _update(FileSystemDirectory *p_dir);

protected:
	static void _bind_methods();

public:
	FileSystemDirectory *get_root() const;
	FileSystemDirectory *get_dir(const String &p_path) const;
	const FileInfo *get_file(const String &p_path) const;

	void update(const String &p_path);
	void update(FileSystemDirectory *p_dir);
	void update_file_system();

	FileSystem();
};
