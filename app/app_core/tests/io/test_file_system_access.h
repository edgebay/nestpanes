#pragma once

#include "app/app_core/io/file_system_access.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#ifdef WINDOWS_ENABLED
#include "app/app_core/platform/windows/file_system_access_windows.h"
#endif

namespace TestFileSystemAccess {

static inline String get_data_path() {
	String data_path = "../app/app_core/tests/data";
	return TestUtils::get_executable_dir().path_join(data_path);
}

static inline String get_test_path() {
	return get_data_path().path_join("file_system_access_test");
}

TEST_CASE("[App][FileSystemAccess] Path existence with test data") {
	String test_root = get_data_path();
	String valid_dir = test_root.path_join("file_system_access_test");
	String valid_file = test_root.path_join("file_system_access_test/file_system_access_test.txt");
	String invalid_path = test_root.path_join("nonexistent.xyz");

	CHECK(FileSystemAccess::path_exists(test_root) == true);
	CHECK(FileSystemAccess::dir_exists(test_root) == true);
	CHECK(FileSystemAccess::file_exists(valid_file) == true);
	CHECK(FileSystemAccess::dir_exists(valid_dir) == true);

	CHECK(FileSystemAccess::path_exists(invalid_path) == false);
	CHECK(FileSystemAccess::file_exists(invalid_path) == false);
	CHECK(FileSystemAccess::dir_exists(invalid_path) == false);
}

TEST_CASE("[App][FileSystemAccess] Directory operations") {
	Error err = FAILED;
	String work_dir = get_test_path();

	String dir_root = work_dir.path_join("level1");
	CHECK(FileSystemAccess::dir_exists(dir_root) == false);
	err = FileSystemAccess::make_dir(dir_root);
	CHECK(err == OK);
	CHECK(FileSystemAccess::dir_exists(dir_root) == true);

	err = FileSystemAccess::remove(dir_root);
	CHECK(err == OK);
	CHECK(FileSystemAccess::dir_exists(dir_root) == false);

	String dir_recursive = dir_root.path_join("level2");
	// CHECK(FileSystemAccess::dir_exists(dir_root) == false);
	CHECK(FileSystemAccess::dir_exists(dir_recursive) == false);
	err = FileSystemAccess::make_dir_recursive(dir_recursive);
	CHECK(err == OK);
	CHECK(FileSystemAccess::dir_exists(dir_recursive) == true);

	err = FileSystemAccess::remove(dir_recursive);
	err = FileSystemAccess::remove(dir_root);
}

TEST_CASE("[App][FileSystemAccess] File operations") {
	Error err = FAILED;
	String work_dir = get_test_path();

	String txt = "file.txt";
	String txt_path = work_dir.path_join(txt);
	CHECK(FileSystemAccess::file_exists(txt_path) == false);
	err = FileSystemAccess::create_file(work_dir, txt);
	CHECK(err == OK);
	CHECK(FileSystemAccess::file_exists(txt_path) == true);

	String md = "file.md";
	String md_path = work_dir.path_join(md);
	CHECK(FileSystemAccess::file_exists(md_path) == false);
	err = FileSystemAccess::rename(txt_path, md_path);
	CHECK(err == OK);
	CHECK(FileSystemAccess::file_exists(md_path) == true);
	CHECK(FileSystemAccess::file_exists(txt_path) == false);

	err = FileSystemAccess::remove(md_path);
	CHECK(err == OK);
	CHECK(FileSystemAccess::file_exists(md_path) == false);
}

#if 0 // paste() cannot be tested.
TEST_CASE("[App][FileSystemAccess] Clipboard operations (cut/copy/paste)") {
	String work_dir = get_test_path();
	String src_dir = work_dir.path_join("source");
	String dst_dir = work_dir.path_join("dest");

	FileSystemAccess::make_dir_recursive(src_dir);
	FileSystemAccess::make_dir_recursive(dst_dir);
	CHECK(FileSystemAccess::dir_exists(src_dir) == true);
	CHECK(FileSystemAccess::dir_exists(dst_dir) == true);

	SUBCASE("[App][FileSystemAccess] File") {
		String test_file = "clip_me.txt";
		String src_file = src_dir.path_join(test_file);
		String dst_file = dst_dir.path_join(test_file);

		FileSystemAccess::create_file(src_dir, test_file);
		CHECK(FileSystemAccess::file_exists(src_file) == true);
		CHECK(FileSystemAccess::file_exists(dst_file) == false);

		Vector<String> files;
		files.push_back(src_file);
		// CHECK(FileSystemAccess::can_paste() == false);	// TODO
		CHECK(FileSystemAccess::cut(files) == true);
		CHECK(FileSystemAccess::can_paste() == true);

		CHECK(FileSystemAccess::paste(dst_dir) == true);
		CHECK(FileSystemAccess::file_exists(src_file) == false);
		CHECK(FileSystemAccess::file_exists(dst_file) == true);

		files.clear();
		files.push_back(dst_file);
		CHECK(FileSystemAccess::copy(files) == true);

		CHECK(FileSystemAccess::paste(src_dir) == true);
		CHECK(FileSystemAccess::file_exists(src_file) == true);
		CHECK(FileSystemAccess::file_exists(dst_file) == true);

		FileSystemAccess::remove(src_file);
		FileSystemAccess::remove(dst_file);
	}

	SUBCASE("[App][FileSystemAccess] Folder") {
		String test_folder = "folder";
		String src_folder = src_dir.path_join(test_folder);
		String dst_folder = dst_dir.path_join(test_folder);

		FileSystemAccess::make_dir_recursive(test_folder);

		Vector<String> files;
		files.push_back(src_folder);
		// CHECK(FileSystemAccess::can_paste() == false);	// TODO
		CHECK(FileSystemAccess::cut(files) == true);
		CHECK(FileSystemAccess::can_paste() == true);

		CHECK(FileSystemAccess::paste(dst_dir) == true);
		CHECK(FileSystemAccess::file_exists(src_folder) == false);
		CHECK(FileSystemAccess::file_exists(dst_folder) == true);

		files.clear();
		files.push_back(dst_folder);
		CHECK(FileSystemAccess::copy(files) == true);

		CHECK(FileSystemAccess::paste(src_dir) == true);
		CHECK(FileSystemAccess::file_exists(src_folder) == true);
		CHECK(FileSystemAccess::file_exists(dst_folder) == true);

		FileSystemAccess::remove(src_folder);
		FileSystemAccess::remove(dst_folder);
	}

	FileSystemAccess::remove(src_dir);
	FileSystemAccess::remove(dst_dir);
}
#endif

} // namespace TestFileSystemAccess
