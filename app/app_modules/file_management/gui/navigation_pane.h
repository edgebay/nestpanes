#pragma once

#include "app/gui/pane_base.h"

class Tree;
class TreeItem;

struct FileInfo;
class FileSystem;
class FileSystemDirectory;

class NavigationPane : public PaneBase {
	GDCLASS(NavigationPane, PaneBase);

private:
	Tree *tree = nullptr;

	FileSystem *file_system = nullptr;

	bool updating_tree = false;

	String selected_path = "";

	virtual String _get_pane_title() const override;
	virtual Ref<Texture2D> _get_pane_icon() const override;

	void _update_tree(const Vector<String> &p_uncollapsed_paths = Vector<String>(), bool p_uncollapse_root = false, bool p_scroll_to_selected = true);
	void _update_subtree(TreeItem *p_parent, const FileSystemDirectory *p_dir, const Vector<String> &p_uncollapsed_paths = Vector<String>());
	void _create_tree(TreeItem *p_parent, const FileSystemDirectory *p_dir, const Vector<String> &p_uncollapsed_paths = Vector<String>());
	void _create_file_item(TreeItem *p_parent, const FileInfo *p_file_info);

	// // void _select_file(const String &p_path, bool p_navigate = true);
	void _tree_activate_file();
	void _tree_multi_selected(Object *p_item, int p_column, bool p_selected);
	// void _tree_item_mouse_select(const Vector2 &p_pos, MouseButton p_button);
	// // void _tree_lmb_select(const Vector2 &p_pos, MouseButton p_button);
	// // void _tree_rmb_select(const Vector2 &p_pos, MouseButton p_button);
	void _tree_item_collapsed(TreeItem *p_item);

	// // virtual Vector<String> _get_selected() const override;

	// void _empty_clicked(const Vector2 &p_pos, MouseButton p_button);
	// void _item_clicked(const Vector2 &p_pos, MouseButton p_button);

	TreeItem *_search_item(const String &p_path);
	void _on_file_system_changed(FileSystemDirectory *p_dir);

protected:
	// virtual void _update_file_ui() override;

	// virtual void _set_empty_menu_item(PopupMenu *p_popup) override;
	// virtual void _set_file_menu_item(PopupMenu *p_popup) override;
	// virtual void _set_folder_menu_item(PopupMenu *p_popup) override;

	// void _item_edited();

	void _notification(int p_what);
	static void _bind_methods();

public:
	// virtual bool edit_selected(const FileOrFolder &p_selected) override;

	Vector<String> get_selected_paths() const;
	Vector<String> get_uncollapsed_paths() const;

	void set_file_system(FileSystem *p_file_system);

	NavigationPane();
	~NavigationPane();
};
