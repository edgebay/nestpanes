#pragma once

#include "app/gui/app_control.h"

// class PopupMenu;

// class FileSystemMenu : public PopupMenu {
// 	GDCLASS(FileSystemMenu, PopupMenu);

// public:
// 	FileSystemMenu();
// 	~FileSystemMenu();
// }

class FileSystemControl : public AppControl {
	GDCLASS(FileSystemControl, AppControl);

private:
	String current_path = "";

	bool updating_file_ui = false;

protected:
	virtual void _update_icons() {}
	virtual void _update_file_ui() {}

	virtual bool change_path(const String &p_path);

	static void _bind_methods();

	bool _set_path(const String &p_path);
	String _get_path() const;

	void _update_file_ui_method();
	void update_file_ui();
	bool is_updating_file();

	void _notification(int p_what);

public:
	// // TODO
	// void set_menu(FileSystemMenu *p_menu);
	// FileSystemMenu *get_menu();

	void set_current_path(const String &p_path);
	String get_current_path() const;
	virtual String get_current_dir_name() const;
	virtual Ref<Texture2D> get_current_dir_icon() const;

	FileSystemControl();
	virtual ~FileSystemControl();
};
