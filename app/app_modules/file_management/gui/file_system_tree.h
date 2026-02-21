#pragma once

#include "scene/gui/tree.h"

#include "app/app_core/types/file_info.h"

class FileContextMenu;

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

	FileContextMenu *context_menu = nullptr;

	// TODO
	// bool show_hidden_files = false;

	String to_select = "";
	bool rename_item = false;

	void _update_display_mode();

	TreeItem *_add_tree_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);
	TreeItem *_add_list_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

	void _on_item_activated();
	void _on_multi_selected(Object *p_item, int p_column, bool p_selected);

	void _build_empty_menu();
	void _build_file_menu();
	void _build_folder_menu();
	void _on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button);
	void _on_empty_clicked(const Vector2 &p_pos, MouseButton p_button);

	void _on_item_edited();
	bool _rename_operation_confirm(const String &p_from, const String &p_new_name);
	void _on_draw();

	bool _process_id_pressed(int p_option, const Vector<String> &p_selected);
	void _context_menu_id_pressed(int p_option);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_display_mode(DisplayMode p_display_mode);
	DisplayMode get_display_mode() const { return display_mode; }

	void set_column_settings(const Array &p_column_settings);
	Array get_column_settings() const;

	void set_context_menu(FileContextMenu *p_menu);
	FileContextMenu *get_context_menu() const;
	void process_menu_id(int p_option, const Vector<String> &p_selected);

	TreeItem *add_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

	FileSystemTree();
	~FileSystemTree();
};
