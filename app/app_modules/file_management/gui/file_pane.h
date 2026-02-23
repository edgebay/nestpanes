#pragma once

#include "app/gui/pane_base.h"

#include "scene/gui/item_list.h"

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

class FileSystemItemList : public ItemList {
	GDCLASS(FileSystemItemList, ItemList);

	bool popup_edit_committed = true;
	VBoxContainer *popup_editor_vb = nullptr;
	Popup *popup_editor = nullptr;
	LineEdit *line_editor = nullptr;

	virtual Control *make_custom_tooltip(const String &p_text) const override;
	void _line_editor_submit(const String &p_text);
	void _text_editor_popup_modal_close();

protected:
	static void _bind_methods();

public:
	bool edit_selected();
	String get_edit_text();

	FileSystemItemList();
};

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

	FileSystemItemList *item_list = nullptr;
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
	void _add_item(const FileInfo &p_fi, bool p_is_dir);
	void _update_item_list(FileSystemDirectory *p_dir);
	void _add_item(const FileInfo &p_fi);
	void _update_files(FileSystemDirectory *p_dir);

	// double-clicking selected.
	void _item_dc_selected(int p_item);
	void _on_item_activated();
	void _on_multi_selected(Object *p_item, int p_column, bool p_selected);

	void _on_address_submitted(const String &p_path);
	void _select_history(int p_idx);

	void _go_history();

	void _go_back();
	void _go_forward();
	void _go_up();
	void _refresh();

	void _set_path(const String &p_path, bool p_update_history = true);

	void _on_file_system_changed(const String &p_path);

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
