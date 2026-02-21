#pragma once

#include "scene/gui/tree.h"

#include "app/app_core/types/file_info.h"

class FileSystemTree : public Tree {
	GDCLASS(FileSystemTree, Tree);

public:
	enum DisplayMode {
		DISPLAY_MODE_TREE,
		DISPLAY_MODE_LIST,
	};

	struct ColumnSetting {
		int column = -1;
		bool visible = false;
		String title = "";
		HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
		bool clip = false;
		int expand_ratio = -1;
	};

private:
	DisplayMode display_mode = DISPLAY_MODE_TREE;

	HashMap<int, ColumnSetting> column_settings;

	void _update_display_mode();

	TreeItem *_add_tree_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);
	TreeItem *_add_list_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_display_mode(DisplayMode p_display_mode);
	DisplayMode get_display_mode() const { return display_mode; }

	void set_column_settings(const Array &p_column_settings);
	Array get_column_settings() const;

	TreeItem *add_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

	FileSystemTree();
	~FileSystemTree();
};
