#pragma once

#include "app/app_core/io/file_system_access.h"

class FileSystemAccessWindows : public FileSystemAccess {
private:
	uint64_t drives_mask = 0;
	String current_path = "";

	void _update_drives_icon();

	static HashSet<String> invalid_files;

protected:
	virtual Ref<Texture2D> _get_this_pc_icon() const;

	virtual Ref<Texture2D> _get_icon(const String &p_file_path, bool p_is_dir = false, bool p_is_hidden = false) const override;
	virtual Error _get_file_info(const String &p_file_path, FileInfo &r_info) const override;
	virtual Error _list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME) const override;
	virtual Error _list_drives(List<FileInfo> &r_drives) const override;

	virtual Error _make_dir(const String &p_dir) override;

	virtual bool _file_exists(const String &p_file) const override;
	virtual bool _dir_exists(const String &p_dir) const override;

	virtual bool _open_in_terminal(const String &p_path) override;

	virtual bool _cut(const Vector<String> &p_files) override;
	virtual bool _copy(const Vector<String> &p_files) override;
	virtual bool _paste(const String &p_dir) override;

	virtual Error _rename(const String &p_path, const String &p_new_path) override;
	virtual Error _remove(const String &p_path) override;

	virtual bool _new_file(const String &p_dir, const String &p_filename) override;

public:
	static bool is_path_invalid(const String &p_path);

	virtual Error change_path(const String &p_dir) override; ///< can be relative or absolute, return false on success
	virtual String get_current_path() const override; ///< return current dir location

	static void initialize();
	static void finalize();

	FileSystemAccessWindows();
	~FileSystemAccessWindows();
};
