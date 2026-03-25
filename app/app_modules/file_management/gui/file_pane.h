#pragma once

#include "app/gui/pane_base.h"

class AddressBar;
class Button;
class FileContextMenu;
class HBoxContainer;
class Label;
class LineEdit;
class Popup;
class VBoxContainer;

struct FileInfo;
class FileSystem;
class FileSystemDirectory;
class FileSystemTree;

class FilePane : public PaneBase {
	GDCLASS(FilePane, PaneBase);

private:
	// control
	VBoxContainer *vbox = nullptr;

	HBoxContainer *toolbar = nullptr;
	Button *dir_prev = nullptr;
	Button *dir_next = nullptr;
	Button *dir_up = nullptr;
	Button *refresh = nullptr;
	AddressBar *address_bar = nullptr;

	FileSystemTree *tree = nullptr;

	HBoxContainer *status_bar = nullptr;
	Label *item_count = nullptr;

	FileContextMenu *context_menu = nullptr;

	// file system
	FileSystem *file_system = nullptr;

	// stat
	String current_path = "";
	Vector<String> local_history;
	int local_history_pos = -1;

	virtual String _get_pane_title() const override;
	virtual Ref<Texture2D> _get_pane_icon() const override;

	virtual void _on_active(bool p_active) override;

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;
	void _process_shortcut_input(int p_option, const Vector<String> &p_selected);

	void _update_icons();

	void _update_ui();
	void _update_ui_nocheck(FileSystemDirectory *p_dir);
	void _add_item(const FileInfo &p_fi);
	void _update_files(FileSystemDirectory *p_dir);
	void _update_status_bar();

	void _on_item_activated();
	void _on_item_selected(Object *p_item, bool p_selected);

	void _on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button);
	void _on_empty_clicked(const Vector2 &p_pos, MouseButton p_button);

	void _on_address_submitted(const String &p_path);
	void _select_history(int p_idx);

	void _go_history();

	void _go_back();
	void _go_forward();
	void _go_up();
	void _refresh();

	void _set_path(const String &p_path, bool p_update_history = true);

	void _on_file_system_changed(const String &p_path);
	void _on_reset_path(const String &p_from_path, const String &p_to_path);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual Dictionary get_config_data() override;
	virtual void apply_config_data(const Dictionary &p_dict) override;

	void set_path(const String &p_path, bool p_update_history = true);
	String get_path() const;

	void set_file_system(FileSystem *p_file_system);

	FilePane();
	~FilePane();
};
