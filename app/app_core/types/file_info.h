#pragma once

#include "core/string/string_name.h"
#include "core/templates/list.h"

#include "scene/resources/texture.h"

#define FOLDER_TYPE "folder"

enum class FileSortOption {
	FILE_SORT_NAME = 0,
	FILE_SORT_NAME_REVERSE = 1,
	FILE_SORT_TYPE = 2,
	FILE_SORT_TYPE_REVERSE = 3,
	FILE_SORT_MODIFIED_TIME = 4,
	FILE_SORT_MODIFIED_TIME_REVERSE = 5,
	FILE_SORT_MAX = 6,
};

struct FileInfo {
	String name;
	String path;

	// String icon_path;
	Ref<Texture2D> icon;

	StringName type = "unknown";
	int64_t size = 0;

	uint64_t creation_time = 0;
	uint64_t modified_time = 0; // last_write_time

	bool hidden = false;

	FileInfo &operator=(const FileInfo &p_file_info) = default;

	bool operator<(const FileInfo &p_fi) const {
		return FileNoCaseComparator()(name, p_fi.name);
	}
};

struct FileInfoTypeComparator {
	bool operator()(const FileInfo &p_a, const FileInfo &p_b) const {
		return FileNoCaseComparator()(p_a.name.get_extension() + p_a.type + p_a.name.get_basename(), p_b.name.get_extension() + p_b.type + p_b.name.get_basename());
	}
};

struct FileInfoModifiedTimeComparator {
	bool operator()(const FileInfo &p_a, const FileInfo &p_b) const {
		return p_a.modified_time > p_b.modified_time;
	}
};

void sort_file_info_list(List<FileInfo> &r_file_list, FileSortOption p_file_sort_option);
