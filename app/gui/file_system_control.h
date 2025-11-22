#pragma once

#include "app/gui/app_control.h"

class PopupMenu;

class ConfigFile;

// class FileSystemMenu : public PopupMenu {
// 	GDCLASS(FileSystemMenu, PopupMenu);

// public:
// 	FileSystemMenu();
// 	~FileSystemMenu();
// }

class FileSystemControl : public AppControl {
	GDCLASS(FileSystemControl, AppControl);

public:
	enum FileMenu {
		FILE_MENU_VIEW,
		FILE_MENU_VIEW_LIST,
		FILE_MENU_VIEW_DETAIL,

		FILE_MENU_SORT,

		FILE_MENU_OPEN,
		FILE_MENU_OPEN_IN_NEW_TAB,
		FILE_MENU_OPEN_IN_NEW_WINDOW,
		FILE_MENU_OPEN_EXTERNAL,
		FILE_MENU_OPEN_IN_TERMINAL,

		// FILE_MENU_PIN_TO_QUICK_ACCESS,

		// FILE_MENU_ADD_FAVORITE,
		// FILE_MENU_REMOVE_FAVORITE,

		FILE_MENU_SHOW_IN_EXPLORER,

		FILE_MENU_COPY_PATH,

		FILE_MENU_EXPAND,
		FILE_MENU_COLLAPSE,
		FILE_MENU_EXPAND_ALL,
		FILE_MENU_COLLAPSE_ALL,

		FILE_MENU_EXPAND_TO_CURRENT,
		// FILE_MENU_EXPAND_TO_CURRENT_FOLDER,

		FILE_MENU_UNDO,
		FILE_MENU_REDO,

		FILE_MENU_CUT,
		FILE_MENU_COPY,
		FILE_MENU_PASTE,

		FILE_MENU_REMOVE,
		FILE_MENU_DELETE = FILE_MENU_REMOVE,
		FILE_MENU_RENAME,

		FILE_MENU_NEW,
		FILE_MENU_NEW_FOLDER,
		FILE_MENU_NEW_TEXTFILE,

		FILE_MENU_MAX,

		CONVERT_BASE_ID = 1000,
	};

	enum MenuMode {
		MENU_MODE_EMPTY,
		MENU_MODE_FILE,
		MENU_MODE_FOLDER,
	};

	class FileOrFolder {
	public:
		String path;
		bool is_file = false;

		FileOrFolder() {}
		FileOrFolder(const String &p_path, bool p_is_file) :
				path(p_path),
				is_file(p_is_file) {}
	};

private:
	FileOrFolder to_rename;

	PopupMenu *item_menu = nullptr;

	String current_path = "";

	bool updating_file_ui = false;

protected:
	virtual void _update_icons() {}
	virtual void _update_file_ui() {}

	virtual bool change_path(const String &p_path);

	static void _bind_methods();

	PopupMenu *get_menu() const;

	bool _set_path(const String &p_path);
	String _get_path() const;

	void _update_file_ui_method();
	void update_file_ui();
	bool is_updating_file();

	virtual Vector<String> _get_selected() const { return Vector<String>(); }

	virtual void _item_menu_id_pressed(int p_option);
	virtual void _set_empty_menu_item(PopupMenu *p_popup) {}
	virtual void _set_file_menu_item(PopupMenu *p_popup) {}
	virtual void _set_folder_menu_item(PopupMenu *p_popup) {}
	virtual void _set_menu_item(PopupMenu *p_popup, MenuMode p_mode);

	bool _rename_operation_confirm(const String &p_new_name);

	void _notification(int p_what);

public:
	// // TODO
	// void set_menu(FileSystemMenu *p_menu);
	// FileSystemMenu *get_menu();

	void set_current_path(const String &p_path);
	String get_current_path() const;
	virtual String get_current_dir_name() const;
	virtual Ref<Texture2D> get_current_dir_icon() const;

	virtual bool edit_selected(const FileOrFolder &p_selected) { return false; }

	void popup_menu(const Vector2 &p_pos, MenuMode p_mode);

	virtual void save_layout_to_config(Ref<ConfigFile> p_layout, const String &p_section) const {}
	virtual void load_layout_from_config(Ref<ConfigFile> p_layout, const String &p_section) {}

	FileSystemControl();
	virtual ~FileSystemControl();
};
