#pragma once

#include "app/gui/pane_base.h"

class FileContextMenu;
class FileSystemTree;
class TreeItem;

struct FileInfo;
class FileSystem;
class FileSystemDirectory;

class NavigationPane : public PaneBase {
	GDCLASS(NavigationPane, PaneBase);

private:
	FileSystemTree *tree = nullptr;

	FileContextMenu *context_menu = nullptr;

	FileSystem *file_system = nullptr;

	bool updating_tree = false;

	Vector<String> uncollapsed_paths;
	String selected_path = "";

	virtual String _get_pane_title() const override;
	virtual Ref<Texture2D> _get_pane_icon() const override;

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;
	void _process_shortcut_input(int p_option, const Vector<String> &p_selected);

	void _update_tree();
	void _update_subtree(TreeItem *p_parent, const FileSystemDirectory *p_dir);
	void _create_tree(TreeItem *p_parent, const FileSystemDirectory *p_dir, const Vector<String> &p_uncollapsed_paths = Vector<String>());
	void _create_file_item(TreeItem *p_parent, const FileInfo *p_file_info);

	void _on_item_activated();
	void _on_multi_selected(Object *p_item, int p_column, bool p_selected);
	void _tree_item_collapsed(TreeItem *p_item);

	TreeItem *_search_item(const String &p_path);
	void _on_file_system_changed(const String &p_path);

	Vector<String> _get_uncollapsed_paths(bool p_uncollapse_root = false);
	void _clear_uncollapsed_paths();

protected:
	// TODO: edit
	// void _item_edited();

	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual Dictionary get_config_data() override;
	virtual void apply_config_data(const Dictionary &p_dict) override;

	// TODO: edit
	// virtual bool edit_selected(const FileOrFolder &p_selected) override;

	void set_file_system(FileSystem *p_file_system);

	NavigationPane();
	~NavigationPane();
};
