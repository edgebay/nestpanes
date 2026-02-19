#pragma once

#include "scene/main/node.h"

#include "core/templates/safe_refcount.h"
#include "scene/resources/theme.h"

class Control;
class HBoxContainer;
class MenuBar;
class MenuButton;
class PopupMenu;
class VBoxContainer;

class LayoutManager;
class PaneManager;

class AppAbout;

class AppNode : public Node {
	GDCLASS(AppNode, Node);

public:
	enum MenuOptions {
		// File menu.
		FILE_NEW_TAB,
		FILE_CLOSE_TAB,
		FILE_QUIT,

		// View menu.
		VIEW_LEFT_SIDEBAR,
		VIEW_RIGHT_SIDEBAR,

		// Help menu.
		HELP_SEARCH,
		HELP_DOCS,
		HELP_FORUM,
		HELP_COMMUNITY,
		HELP_COPY_SYSTEM_INFO,
		HELP_GITHUB,
		HELP_REPORT_A_BUG,
		HELP_SUGGEST_A_FEATURE,
		HELP_SEND_DOCS_FEEDBACK,
		HELP_ABOUT,
		HELP_SUPPORT_GODOT_DEVELOPMENT,
	};

private:
	static AppNode *singleton;

	Control *gui_base = nullptr;
	Node *gui_main = nullptr;
	VBoxContainer *main_vbox = nullptr;

	HBoxContainer *title_bar = nullptr;
	VBoxContainer *ribbon = nullptr;

	LayoutManager *layout_manager = nullptr;
	PaneManager *pane_manager = nullptr;

	Control *menu_btn_spacer = nullptr;
	MenuButton *main_menu_button = nullptr; // TODO: Remove?
	MenuBar *main_menu_bar = nullptr;

	PopupMenu *file_menu = nullptr;
	PopupMenu *view_menu = nullptr;
	PopupMenu *help_menu = nullptr;

	AppAbout *about = nullptr;

	Ref<Theme> theme;

	bool exiting = false;
	bool dimmed = false;

	bool follow_system_theme = false;
	bool use_system_accent_color = false;
	bool last_dark_mode_state = false;
	Color last_system_base_color = Color(0, 0, 0, 0);
	Color last_system_accent_color = Color(0, 0, 0, 0);

	void _update_theme(bool p_skip_creation = false);

	void _check_system_theme_changed();

	void _menu_option(int p_option);
	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _toggle_left_sidebar();
	void _toggle_right_sidebar();

	void _exit(int p_exit_code);

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

	void _viewport_resized();

	void _save_layout();
	void _load_layout();

	String _get_main_scene_path() const;
	Error _parse_node(Node *p_node);
	bool _load_main_scene();

	void _update_main_menu_type();
	void _add_to_main_menu(const String &p_name, PopupMenu *p_menu);
	void _init_main_menu();

	void _on_area_visibility_changed(const String &p_name, bool p_visible);

protected:
	void _notification(int p_what);

public:
	static AppNode *get_singleton() { return singleton; }

	AppNode();
	~AppNode();
};
