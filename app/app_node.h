#pragma once

#include "scene/main/node.h"

#include "core/templates/safe_refcount.h"
#include "scene/resources/theme.h"

class Control;
class HBoxContainer;
class VBoxContainer;
class SplitContainer;
class HSplitContainer;
class VSplitContainer;

class AppTabContainer;
class FileSystemControl;

class Timer;

class AppNode : public Node {
	GDCLASS(AppNode, Node);

private:
	static AppNode *singleton;

	Control *gui_base = nullptr;
	VBoxContainer *main_vbox = nullptr;

	HBoxContainer *title_bar = nullptr;

	// Split containers.
	HSplitContainer *left_hsplit = nullptr;
	HSplitContainer *right_hsplit = nullptr;

	SplitContainer *left_split = nullptr;
	SplitContainer *center_split = nullptr;

	AppTabContainer *left_sidebar = nullptr;
	AppTabContainer *main_screen = nullptr;

	Vector<FileSystemControl *> file_system_controls;

	Ref<Theme> theme;

	// Timer *system_theme_timer = nullptr;
	bool follow_system_theme = false;
	bool use_system_accent_color = false;
	bool last_dark_mode_state = false;
	Color last_system_base_color = Color(0, 0, 0, 0);
	Color last_system_accent_color = Color(0, 0, 0, 0);

	Timer *layout_save_delay_timer = nullptr;

	bool load_layout_done = false;

	void _update_theme(bool p_skip_creation = false);

	int _new_tab(AppTabContainer *p_parent);
	// void _on_tab_path_changed(const String &p_path);
	void _on_tab_path_changed(FileSystemControl *p_fs);
	void _on_tree_item_activated(const String &p_path, bool is_dir);
	// void _on_tree_item_selected(TreeItem *p_item);
	void _on_tree_item_selected(const String &p_path, bool is_dir);

	String _get_config_path() const;
	void _save_layout();
	void _load_layout();

protected:
	void _notification(int p_what);

public:
	static AppNode *get_singleton() { return singleton; }

	void save_layout_delayed();

	AppNode();
	~AppNode();
};
