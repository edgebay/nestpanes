#pragma once

#include "app/gui/file_system_control.h"

#define LOCAL_HISTORY_MAX 5 // TODO: config

class Button;
class ConfirmationDialog;
class HSplitContainer;
class HBoxContainer;
class ItemList;
class LineEdit;
class MenuButton;
class OptionButton;
class TextureRect;
class VBoxContainer;

struct FileInfo;

class FileSystemList : public FileSystemControl {
	GDCLASS(FileSystemList, FileSystemControl);

private:
	// control
	VBoxContainer *vbox = nullptr;

	HBoxContainer *pathhb = nullptr;
	Button *dir_prev = nullptr;
	Button *dir_next = nullptr;
	Button *dir_up = nullptr;
	Button *refresh = nullptr;
	LineEdit *dir = nullptr;
	OptionButton *histories = nullptr;

	HSplitContainer *body_hsplit = nullptr;
	ItemList *item_list = nullptr;
	TextureRect *preview = nullptr;

	// stat
	bool show_hidden_files = false;

	void _update_dir();
	void _add_item(const FileInfo &p_fi, bool p_is_dir = false);
	void _update_file_list();
	void _update_up_button();
	void _update_history_button();

	virtual void _item_menu_id_pressed(int p_option) override;

	void _empty_clicked(const Vector2 &p_pos, MouseButton p_button);
	void _item_clicked(int p_item, const Vector2 &p_pos, MouseButton p_button);

	// double-clicking selected.
	void _item_dc_selected(int p_item);

	void _select_history(int p_idx);

	void _dir_submitted(const String &p_path);

	Vector<String> local_history;
	int local_history_pos = -1;
	void _push_history();

	void _go_history();

	void _go_back();
	void _go_forward();
	void _go_up();

	void _initialize_ui();

protected:
	virtual void _update_icons() override;
	virtual void _update_file_ui() override;

	virtual void _set_empty_menu_item(PopupMenu *p_popup) override;
	virtual void _set_file_menu_item(PopupMenu *p_popup) override;
	virtual void _set_folder_menu_item(PopupMenu *p_popup) override;

	// void _notification(int p_what);

public:
	FileSystemList();
	~FileSystemList();
};
