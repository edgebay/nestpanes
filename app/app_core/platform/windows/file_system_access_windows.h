#pragma once

#include "app/app_core/io/file_system_access.h"

struct FileSystemAccessWindowsPrivate;

class FileSystemAccessWindows : public FileSystemAccess {
private:
	uint64_t drives_mask = 0;

	FileSystemAccessWindowsPrivate *p = nullptr;

	void _update_drives_icon();

	static HashSet<String> invalid_files;

protected:
	virtual Ref<Texture2D> _get_this_pc_icon() const;

	virtual Error _list_dir_begin(const String &p_path) override;
	virtual Error _get_next(FileInfo &r_info) override;
	virtual void _list_dir_end() override;

	virtual Ref<Texture2D> _get_icon(const String &p_file_path, bool p_is_dir = false, bool p_is_hidden = false) const override;
	virtual Error _get_file_info(const String &p_file_path, FileInfo &r_info) override;
#if 0 // TODO: handle DRIVE_TYPE
	virtual Error _list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME) const override;
#endif
	virtual Error _list_drives(List<FileInfo> &r_drives) const override;

	virtual bool _canonicalize_path(const String &p_path, String &r_canonicalized) override;

	virtual bool _path_exists(const String &p_path) const override;

	virtual bool _cut(const Vector<String> &p_files) override;
	virtual bool _copy(const Vector<String> &p_files) override;
	virtual bool _paste(const String &p_dir, Vector<String> &r_dest_paths) override;
	virtual bool _can_paste() override;
	virtual bool _get_clipboard_paths(Vector<String> &r_paths, bool &r_is_cut) override;
	virtual Error _move(bool p_is_copy, const String &p_to_dir, const Vector<String> &p_from_paths, Vector<String> &r_dest_paths) override;

	virtual Error _create_file(const String &p_dir, const String &p_filename) override;

public:
	static bool is_path_invalid(const String &p_path);

	static void initialize();
	static void finalize();

	FileSystemAccessWindows();
	~FileSystemAccessWindows();
};
