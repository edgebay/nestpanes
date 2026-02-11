#pragma once

#include "scene/gui/popup_menu.h"

class FileSystem;
class PaneBase;

class FileContextMenu : public PopupMenu {
	GDCLASS(FileContextMenu, PopupMenu);

public:
	enum FileMenu {
		// TODO: file list
		// FILE_MENU_VIEW,
		// FILE_MENU_VIEW_LIST,
		// FILE_MENU_VIEW_DETAIL,

		// FILE_MENU_SORT,

		FILE_MENU_OPEN,
		FILE_MENU_OPEN_IN_NEW_TAB,
		FILE_MENU_OPEN_IN_NEW_WINDOW,
		FILE_MENU_OPEN_EXTERNAL,
		FILE_MENU_OPEN_IN_TERMINAL,

		FILE_MENU_SHOW_IN_EXPLORER,

		FILE_MENU_COPY_PATH,

		// TODO: file tree
		// FILE_MENU_EXPAND,
		// FILE_MENU_COLLAPSE,
		// FILE_MENU_EXPAND_ALL,
		// FILE_MENU_COLLAPSE_ALL,

		// FILE_MENU_EXPAND_TO_CURRENT,

		FILE_MENU_UNDO,
		FILE_MENU_REDO,

		FILE_MENU_CUT,
		FILE_MENU_COPY,
		FILE_MENU_PASTE,

		FILE_MENU_REMOVE,
		FILE_MENU_DELETE = FILE_MENU_REMOVE,
		FILE_MENU_RENAME,

		// set_item_submenu_node
		FILE_MENU_NEW,
		FILE_MENU_NEW_FOLDER,
		FILE_MENU_NEW_TEXTFILE,

		FILE_MENU_MAX = 1024,
	};

private:
	Vector<String> targets;

	FileSystem *file_system = nullptr;

	void _id_pressed(int p_index);

	void _on_new_file_pane(PaneBase *p_pane, const String &p_path);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void add_file_item(int p_id);

	void add_custom_item(const String &p_label, int p_id_offset, Key p_accel = Key::NONE);
	void add_custom_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id_offset, Key p_accel = Key::NONE);
	void remove_custom_item(int p_id_offset);
	int calculate_custom_id(int p_id_offset);
	int calculate_original_id(int p_custom_id);

	void set_targets(const Vector<String> &p_targets);
	Vector<String> get_targets() const;
	void clear_targets();

	void set_file_system(FileSystem *p_file_system);

	FileContextMenu();
	~FileContextMenu();
};
