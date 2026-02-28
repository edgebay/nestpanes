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

	enum ColumnType {
		COLUMN_TYPE_NAME,
		COLUMN_TYPE_MODIFIED,
		COLUMN_TYPE_CREATED,
		COLUMN_TYPE_TYPE,
		COLUMN_TYPE_SIZE,
	};

	struct ColumnSetting {
		ColumnType type = COLUMN_TYPE_NAME;
		int column = -1;
		bool visible = false;
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

	// // Selection.
	// enum DragType {
	// 	DRAG_NONE,
	// 	DRAG_BOX_SELECTION,
	// 	// DRAG_LEFT,
	// 	// DRAG_TOP_LEFT,
	// 	// DRAG_TOP,
	// 	// DRAG_TOP_RIGHT,
	// 	// DRAG_RIGHT,
	// 	// DRAG_BOTTOM_RIGHT,
	// 	// DRAG_BOTTOM,
	// 	// DRAG_BOTTOM_LEFT,
	// 	// DRAG_ANCHOR_TOP_LEFT,
	// 	// DRAG_ANCHOR_TOP_RIGHT,
	// 	// DRAG_ANCHOR_BOTTOM_RIGHT,
	// 	// DRAG_ANCHOR_BOTTOM_LEFT,
	// 	// DRAG_ANCHOR_ALL,
	// 	// DRAG_QUEUED,
	// 	// DRAG_MOVE,
	// 	// DRAG_MOVE_X,
	// 	// DRAG_MOVE_Y,
	// 	// DRAG_SCALE_X,
	// 	// DRAG_SCALE_Y,
	// 	// DRAG_SCALE_BOTH,
	// 	// DRAG_ROTATE,
	// 	// DRAG_PIVOT,
	// 	// DRAG_TEMP_PIVOT,
	// 	// DRAG_V_GUIDE,
	// 	// DRAG_H_GUIDE,
	// 	// DRAG_DOUBLE_GUIDE,
	// 	// DRAG_KEY_MOVE
	// };
	// bool detecting_box_selection = false;
	// DragType drag_type = DRAG_NONE;
	// Point2 drag_from;
	// Point2 box_selecting_to;
	// Point2 prev_selecting_to;
	// TreeItem *prev_hovered_item;
	// TreeItem *starting_item;

	void _update_display_mode();

	TreeItem *_add_tree_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);
	TreeItem *_add_list_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

	void _on_item_activated();
	void _on_multi_selected(Object *p_item, int p_column, bool p_selected);

	// Context menu.
	void _build_empty_menu();
	void _build_file_menu();
	void _build_folder_menu();
	void _build_selected_items_menu();
	void _on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button);
	void _on_empty_clicked(const Vector2 &p_pos, MouseButton p_button);

	void _on_item_edited();
	bool _rename_operation_confirm(const String &p_from, const String &p_new_name);
	void _on_draw();

	bool _process_id_pressed(int p_option, const Vector<String> &p_selected);
	void _context_menu_id_pressed(int p_option);

	// // Selection.
	// void _draw_selection();

	Vector<TreeItem *> _get_selected_items();
	// bool _gui_input_select(const Ref<InputEvent> &p_event);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

	void set_display_mode(DisplayMode p_display_mode);
	DisplayMode get_display_mode() const;

	void set_column_settings(const Array &p_column_settings);
	Array get_column_settings() const;

	void set_context_menu(FileContextMenu *p_menu);
	FileContextMenu *get_context_menu() const;
	void process_menu_id(int p_option, const Vector<String> &p_selected);

	Vector<String> get_selected_paths();
	Vector<String> get_uncollapsed_paths() const;

	TreeItem *add_item(const FileInfo &p_fi, TreeItem *p_parent = nullptr, int p_index = -1);

	FileSystemTree();
	~FileSystemTree();
};
