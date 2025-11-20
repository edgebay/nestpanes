#pragma once

#include "app/gui/file_system_control.h"

#include "app/core/io/file_system_access.h"

class Button;
class HBoxContainer;
class LineEdit;
class Tree;
class TreeItem;
class VBoxContainer;

class FileSystemTreeDirectory : public Object {
	GDCLASS(FileSystemTreeDirectory, Object);

	FileInfo info;
	bool scanned = false;

	FileSystemTreeDirectory *parent = nullptr;
	Vector<FileSystemTreeDirectory *> subdirs;
	Vector<FileInfo *> files;

	friend class FileSystemTree;

public:
	const FileInfo *get_info() const;

	int get_subdir_count() const;
	FileSystemTreeDirectory *get_subdir(int p_idx) const;
	int get_file_count() const;
	FileInfo *get_file(int p_idx) const;

	FileSystemTreeDirectory *get_parent();

	FileSystemTreeDirectory();
	~FileSystemTreeDirectory();
};

class FileSystemTree : public FileSystemControl {
	GDCLASS(FileSystemTree, FileSystemControl);

private:
	VBoxContainer *vbox = nullptr;

	HBoxContainer *pathhb = nullptr;
	Button *home = nullptr;
	Button *dir_up = nullptr;
	Button *refresh = nullptr;
	LineEdit *dir = nullptr;

	Tree *tree = nullptr;

	bool updating_tree = false;
	TreeItem *collapsed_changed_item = nullptr;

	String home_path;
	String selected_path;

	FileSystemTreeDirectory *filesystem = nullptr;

	void _update_tree(const Vector<String> &p_uncollapsed_paths = Vector<String>(), bool p_uncollapse_root = false, bool p_scroll_to_selected = true);
	void _create_tree(TreeItem *p_parent, const FileSystemTreeDirectory *p_dir, const Vector<String> &p_uncollapsed_paths = Vector<String>());
	void _create_tree_item(TreeItem *p_parent, const FileInfo *p_file_info);

	void _select_file(const String &p_path, bool p_navigate = true);
	void _tree_activate_file();
	void _tree_item_selected();
	void _tree_multi_selected(Object *p_item, int p_column, bool p_selected);
	void _tree_item_mouse_select(const Vector2 &p_pos, MouseButton p_button);
	void _tree_lmb_select(const Vector2 &p_pos, MouseButton p_button);
	void _tree_rmb_select(const Vector2 &p_pos, MouseButton p_button);
	void _tree_item_collapsed(TreeItem *p_item);

	virtual Vector<String> _get_selected() const override;

	void _empty_clicked(const Vector2 &p_pos, MouseButton p_button);
	void _item_clicked(const Vector2 &p_pos, MouseButton p_button);

	void _scan_dir(FileSystemTreeDirectory *r_dir, const String &p_path, bool p_scan_subdirs = false);

	void _initialize_filesystem();

protected:
	virtual void _update_file_ui() override;

	virtual void _set_empty_menu_item(PopupMenu *p_popup) override;
	virtual void _set_file_menu_item(PopupMenu *p_popup) override;
	virtual void _set_folder_menu_item(PopupMenu *p_popup) override;

	void _item_edited();

	void _notification(int p_what);

	static void _bind_methods();

public:
	virtual bool edit_selected(const FileOrFolder &p_selected) override;

	Vector<String> get_selected_paths() const;
	Vector<String> get_uncollapsed_paths() const;

	FileSystemTree();
	~FileSystemTree();
};
