#pragma once

#include "app/app_modules/file_management/file_system.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "app/app_core/io/file_system_access.h"

namespace TestFileSystem {

// --- FileSystem static utility tests ---

TEST_CASE("[App][FileSystem] parse_time formats timestamp correctly") {
	// Use a known UTC timestamp: 2025-01-15 09:50:00 UTC = 1736934600
	uint64_t timestamp = 1736934600;
	String result = FileSystem::parse_time(timestamp, false);
	CHECK(result == "2025/01/15 09:50");
}

TEST_CASE("[App][FileSystem] parse_time handles zero timestamp") {
	String result = FileSystem::parse_time(0, false);
	CHECK(result == "1970/01/01 00:00");
}

TEST_CASE("[App][FileSystem] parse_size formats bytes with separators") {
	SUBCASE("Small size") {
		String result = FileSystem::parse_size(0);
		CHECK(result == "0 B");
	}

	SUBCASE("Hundreds") {
		String result = FileSystem::parse_size(999);
		CHECK(result == "999 B");
	}

	SUBCASE("Thousands") {
		String result = FileSystem::parse_size(1234);
		CHECK(result == "1,234 B");
	}

	SUBCASE("Millions") {
		String result = FileSystem::parse_size(1234567);
		CHECK(result == "1,234,567 B");
	}

	SUBCASE("Large size") {
		String result = FileSystem::parse_size(1234567890);
		CHECK(result == "1,234,567,890 B");
	}
}

TEST_CASE("[App][FileSystem] is_root checks COMPUTER_PATH") {
	CHECK(FileSystem::is_root(COMPUTER_PATH) == true);
	CHECK(FileSystem::is_root("C:") == false);
	CHECK(FileSystem::is_root("") == false);
	CHECK(FileSystem::is_root("/") == false);
}

TEST_CASE("[App][FileSystem] is_valid_path recognizes COMPUTER_PATH") {
	CHECK(FileSystem::is_valid_path(COMPUTER_PATH) == true);
}

TEST_CASE("[App][FileSystem] is_valid_dir_path recognizes COMPUTER_PATH") {
	CHECK(FileSystem::is_valid_dir_path(COMPUTER_PATH) == true);
}

// --- FileInfo sorting tests ---

TEST_CASE("[App][FileInfo] Default sorting by name") {
	FileInfo a, b, c;
	a.name = "Charlie";
	b.name = "Alpha";
	c.name = "Bravo";

	List<FileInfo> list;
	list.push_back(a);
	list.push_back(b);
	list.push_back(c);

	sort_file_info_list(list, FileSortOption::FILE_SORT_NAME);

	auto it = list.front();
	CHECK(it->get().name == "Alpha");
	it = it->next();
	CHECK(it->get().name == "Bravo");
	it = it->next();
	CHECK(it->get().name == "Charlie");
}

TEST_CASE("[App][FileInfo] Reverse name sorting") {
	FileInfo a, b, c;
	a.name = "Alpha";
	b.name = "Bravo";
	c.name = "Charlie";

	List<FileInfo> list;
	list.push_back(a);
	list.push_back(b);
	list.push_back(c);

	sort_file_info_list(list, FileSortOption::FILE_SORT_NAME_REVERSE);

	auto it = list.front();
	CHECK(it->get().name == "Charlie");
	it = it->next();
	CHECK(it->get().name == "Bravo");
	it = it->next();
	CHECK(it->get().name == "Alpha");
}

TEST_CASE("[App][FileInfo] Type sorting groups by extension") {
	FileInfo txt, md, cpp;
	txt.name = "readme.txt";
	txt.type = "txt";
	md.name = "notes.md";
	md.type = "md";
	cpp.name = "main.cpp";
	cpp.type = "cpp";

	List<FileInfo> list;
	list.push_back(txt);
	list.push_back(md);
	list.push_back(cpp);

	sort_file_info_list(list, FileSortOption::FILE_SORT_TYPE);

	// Sorted by extension+type+basename.
	auto it = list.front();
	CHECK(it->get().name == "main.cpp");
	it = it->next();
	CHECK(it->get().name == "notes.md");
	it = it->next();
	CHECK(it->get().name == "readme.txt");
}

TEST_CASE("[App][FileInfo] Modified time sorting") {
	FileInfo old_file, new_file;
	old_file.name = "old.txt";
	old_file.modified_time = 1000;
	new_file.name = "new.txt";
	new_file.modified_time = 2000;

	List<FileInfo> list;
	list.push_back(old_file);
	list.push_back(new_file);

	sort_file_info_list(list, FileSortOption::FILE_SORT_MODIFIED_TIME);

	// Newest first.
	auto it = list.front();
	CHECK(it->get().name == "new.txt");
	it = it->next();
	CHECK(it->get().name == "old.txt");
}

TEST_CASE("[App][FileInfo] Modified time reverse sorting") {
	FileInfo old_file, new_file;
	old_file.name = "old.txt";
	old_file.modified_time = 1000;
	new_file.name = "new.txt";
	new_file.modified_time = 2000;

	List<FileInfo> list;
	list.push_back(old_file);
	list.push_back(new_file);

	sort_file_info_list(list, FileSortOption::FILE_SORT_MODIFIED_TIME_REVERSE);

	// Oldest first.
	auto it = list.front();
	CHECK(it->get().name == "old.txt");
	it = it->next();
	CHECK(it->get().name == "new.txt");
}

// --- Type checking utility tests ---

TEST_CASE("[App][FileSystemAccess] Type checking utilities") {
	FileInfo root_info;
	root_info.type = ROOT_TYPE;
	CHECK(FileSystemAccess::is_root_type(root_info) == true);
	CHECK(FileSystemAccess::is_drive_type(root_info) == false);
	CHECK(FileSystemAccess::is_dir_type(root_info) == true); // root is a dir type

	FileInfo drive_info;
	drive_info.type = DRIVE_TYPE;
	CHECK(FileSystemAccess::is_root_type(drive_info) == false);
	CHECK(FileSystemAccess::is_drive_type(drive_info) == true);
	CHECK(FileSystemAccess::is_dir_type(drive_info) == true); // drive is a dir type

	FileInfo folder_info;
	folder_info.type = FOLDER_TYPE;
	CHECK(FileSystemAccess::is_root_type(folder_info) == false);
	CHECK(FileSystemAccess::is_drive_type(folder_info) == false);
	CHECK(FileSystemAccess::is_dir_type(folder_info) == true);

	FileInfo file_info;
	file_info.type = "txt";
	CHECK(FileSystemAccess::is_root_type(file_info) == false);
	CHECK(FileSystemAccess::is_drive_type(file_info) == false);
	CHECK(FileSystemAccess::is_dir_type(file_info) == false);
}

} // namespace TestFileSystem
