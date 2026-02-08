#include "app_node.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/string/translation.h"
#include "core/string/ustring.h"
#include "core/version.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/container.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"

#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"

#include "scene/resources/packed_scene.h"

#include "scene/theme/theme_db.h"

#include "servers/display/display_server.h"

#include "app/app_core/app_paths.h"
#include "app/app_string_names.h"
#include "app/settings/app_settings.h"
#include "app/themes/app_scale.h"
#include "app/themes/app_theme_manager.h"

#include "app/app_core/io/file_system_access.h"

#include "app/gui/app_about.h"
#include "app/gui/app_tab_container.h"
#include "app/gui/container_manager.h"
#include "app/gui/multi_split_container.h"
#include "app/gui/pane_factory.h"

#include "app/app_modules/file_management/file_system.h"
#include "app/app_modules/file_management/gui/file_pane.h"
#include "app/app_modules/file_management/gui/navigation_pane.h"
// TODO: SettingsPane, WelcomePane
// #include "app/settings/gui/settings_pane.h"
// #include "app/gui/welcome_pane.h"

#define LEFT_SIDEBAR_NAME "left_sidebar"
#define CENTRAL_AREA_NAME "central_area"
#define RIGHT_SIDEBAR_NAME "right_sidebar"

AppNode *AppNode::singleton = nullptr;

void AppNode::_update_theme(bool p_skip_creation) {
	if (!p_skip_creation) {
		theme = AppThemeManager::generate_theme(theme);
		DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), AppStringName(App)));
	}

	Vector<Ref<Theme>> app_themes;
	app_themes.push_back(theme);
	app_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *node_tc = ThemeDB::get_singleton()->get_theme_context(this);
	if (node_tc) {
		node_tc->set_themes(app_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(this, app_themes);
	}

	Window *window = get_window();
	if (window) {
		ThemeContext *window_tc = ThemeDB::get_singleton()->get_theme_context(window);
		if (window_tc) {
			window_tc->set_themes(app_themes);
		} else {
			ThemeDB::get_singleton()->create_theme_context(window, app_themes);
		}
	}

	// Update styles.
	{
		bool global_menu = !bool(EDITOR_GET("interface/app/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		bool dark_mode = DisplayServer::get_singleton()->is_dark_mode_supported() && DisplayServer::get_singleton()->is_dark_mode();

		gui_base->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("Background"), AppStringName(AppStyles)));
		main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, theme->get_constant(SNAME("window_border_margin"), EditorStringName(App)));
		main_vbox->add_theme_constant_override("separation", theme->get_constant(SNAME("top_bar_separation"), EditorStringName(App)));

		if (main_menu_button != nullptr) {
			main_menu_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), EditorStringName(EditorIcons)));
		}

		// TODO: Set menu icon?
	}
}

void AppNode::_check_system_theme_changed() {
	DisplayServer *display_server = DisplayServer::get_singleton();

	bool global_menu = !bool(EDITOR_GET("interface/app/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
	bool system_theme_changed = false;

	if (follow_system_theme) {
		if (display_server->get_base_color() != last_system_base_color) {
			system_theme_changed = true;
			last_system_base_color = display_server->get_base_color();
		}

		if (display_server->is_dark_mode_supported() && display_server->is_dark_mode() != last_dark_mode_state) {
			system_theme_changed = true;
			last_dark_mode_state = display_server->is_dark_mode();
		}
	}

	if (use_system_accent_color) {
		if (display_server->get_accent_color() != last_system_accent_color) {
			system_theme_changed = true;
			last_system_accent_color = display_server->get_accent_color();
		}
	}

	if (system_theme_changed) {
		_update_theme();
	} else if (global_menu && display_server->is_dark_mode_supported() && display_server->is_dark_mode() != last_dark_mode_state) {
		last_dark_mode_state = display_server->is_dark_mode();

		// Update system menus.
		bool dark_mode = DisplayServer::get_singleton()->is_dark_mode();

		// TODO: Set menu icon?
	}
}

void AppNode::_menu_option(int p_option) {
	_menu_option_confirm(p_option, false);
}

void AppNode::_menu_option_confirm(int p_option, bool p_confirmed) {
	switch (p_option) {
		case FILE_NEW_TAB: {
			container_manager->new_tab();
		} break;
		case FILE_CLOSE_TAB: {
			container_manager->close_current_tab();
		} break;
		case FILE_QUIT: {
			_exit(EXIT_SUCCESS);
		} break;

			// case EDITOR_COMMAND_PALETTE: {
			// 	command_palette->open_popup();
			// } break;

		case VIEW_LEFT_SIDEBAR: {
			int index = view_menu->get_item_index(p_option);
			bool is_checked = view_menu->is_item_checked(index);
			left_sidebar->set_visible(!is_checked);
			view_menu->set_item_checked(index, !is_checked);
		} break;
		// case HELP_SEARCH: {
		// 	emit_signal(SNAME("request_help_search"), "");
		// } break;
		// case HELP_DOCS: {
		// 	OS::get_singleton()->shell_open(GODOT_VERSION_DOCS_URL "/");
		// } break;
		// case HELP_FORUM: {
		// 	OS::get_singleton()->shell_open("https://forum.godotengine.org/");
		// } break;
		case HELP_GITHUB: {
			OS::get_singleton()->shell_open("https://github.com/edgebay/nestpanes");
		} break;
		case HELP_REPORT_A_BUG: {
			OS::get_singleton()->shell_open("https://github.com/edgebay/nestpanes/issues");
		} break;
		// case HELP_COPY_SYSTEM_INFO: {
		// 	String info = _get_system_info();
		// 	DisplayServer::get_singleton()->clipboard_set(info);
		// } break;
		// case HELP_SUGGEST_A_FEATURE: {
		// 	OS::get_singleton()->shell_open("https://github.com/godotengine/godot-proposals#readme");
		// } break;
		// case HELP_SEND_DOCS_FEEDBACK: {
		// 	OS::get_singleton()->shell_open("https://github.com/godotengine/godot-docs/issues");
		// } break;
		// case HELP_COMMUNITY: {
		// 	OS::get_singleton()->shell_open("https://godotengine.org/community");
		// } break;
		case HELP_ABOUT: {
			about->popup_centered(Size2(780, 500) * APP_SCALE);
		} break;
			// case HELP_SUPPORT_GODOT_DEVELOPMENT: {
			// 	OS::get_singleton()->shell_open("https://fund.godotengine.org");
			// } break;
	}
}

void AppNode::_toggle_left_sidebar() {
	bool is_visible = left_sidebar->is_visible();
	if (is_visible) {
		left_sidebar->hide();
	} else {
		left_sidebar->show();
	}
}

void AppNode::_toggle_right_sidebar() {
	bool is_visible = right_sidebar->is_visible();
	if (is_visible) {
		right_sidebar->hide();
	} else {
		right_sidebar->show();
	}
}

void AppNode::_on_navigation_pane_create(PaneBase *p_pane) {
	NavigationPane *pane = Object::cast_to<NavigationPane>(p_pane);
	if (pane) {
		pane->set_file_system(file_system);
		pane->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
		pane->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
	}
}

void AppNode::_on_file_pane_create(PaneBase *p_pane) {
	FilePane *pane = Object::cast_to<FilePane>(p_pane);
	if (pane) {
		pane->set_file_system(file_system);
	}
}

void AppNode::_on_tree_item_activated(const String &p_path, bool is_dir) {
	if (!is_dir) {
		// TODO: open_file()/run_file()
		OS::get_singleton()->shell_open(p_path);
	}
}

void AppNode::_on_tree_item_selected(const String &p_path, bool is_dir) {
	if (is_dir) {
		// Current tab container is NavigationPane
		AppTabContainer *tab_container = container_manager->get_prev_tab_container();
		if (tab_container == nullptr) {
			return;
		}

		FilePane *pane = Object::cast_to<FilePane>(tab_container->get_current_tab_control());
		if (pane) {
			pane->set_path(p_path);
		}
	}
}

void AppNode::_exit(int p_exit_code) {
	exiting = true;

	_save_layout();

	get_tree()->quit(p_exit_code);
}

void AppNode::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if ((k.is_valid() && k->is_pressed() && !k->is_echo()) || Object::cast_to<InputEventShortcut>(*p_event)) {
		bool is_handled = true;
		if (ED_IS_SHORTCUT("app/next_tab", p_event)) {
			AppTabContainer *current_tab_container = container_manager->get_current_tab_container();
			int tab_count = current_tab_container ? current_tab_container->get_tab_count() : 0;
			if (tab_count > 0) {
				int next_tab = current_tab_container->get_current_tab() + 1;
				next_tab %= tab_count;
				current_tab_container->set_current_tab(next_tab);
			}
		} else if (ED_IS_SHORTCUT("app/prev_tab", p_event)) {
			AppTabContainer *current_tab_container = container_manager->get_current_tab_container();
			int tab_count = current_tab_container ? current_tab_container->get_tab_count() : 0;
			if (tab_count > 0) {
				int next_tab = current_tab_container->get_current_tab() - 1;
				next_tab = next_tab >= 0 ? next_tab : tab_count - 1;
				current_tab_container->set_current_tab(next_tab);
			}
		} else {
			is_handled = false;
		}

		if (is_handled) {
			get_tree()->get_root()->set_input_as_handled();
		}
	}
}

String AppNode::_get_config_path() const {
	return AppPaths::get_singleton()->get_config_dir().path_join("app_layout.cfg");
}

void AppNode::_save_layout() {
	if (!load_layout_done) {
		return;
	}

	// Save layout.
	Ref<PackedScene> sdata;
	sdata.instantiate();
	Error err = sdata->pack(gui_main);
	if (err != OK) {
		// TODO: hint
		// show_accept(TTR("Couldn't save scene. Likely dependencies (instances or inheritance) couldn't be satisfied."), TTR("OK"));
		return;
	}

	int flg = 0;
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	String scene_path = _get_main_scene_path();
	err = ResourceSaver::save(sdata, scene_path, flg);
	// TODO
	if (err == OK) {
	}

	// Save config.
	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(config_path);

	// TODO: save_layout_to_config(config, "docks");

	config->save(config_path);
}

void AppNode::_load_layout() {
	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	Error err = config->load(config_path);
	// TODO
	if (err != OK) { // No config.
	} else {
	}
	load_layout_done = true;
}

String AppNode::_get_main_scene_path() const {
	return AppPaths::get_singleton()->get_config_dir().path_join("app_scene.tscn");
}

Error AppNode::_parse_node(Node *p_node) {
	if (Object::cast_to<MultiSplitContainer>(p_node)) {
		String node_name = p_node->get_name();
		if (node_name == LEFT_SIDEBAR_NAME) {
			left_sidebar = Object::cast_to<MultiSplitContainer>(p_node);
		} else if (node_name == CENTRAL_AREA_NAME) {
			central_area = Object::cast_to<MultiSplitContainer>(p_node);
		} else if (node_name == RIGHT_SIDEBAR_NAME) {
			right_sidebar = Object::cast_to<MultiSplitContainer>(p_node);
		}
	} else if (Object::cast_to<AppTabContainer>(p_node)) {
		// TODO: handle left_tab_container, right_tab_container
		AppTabContainer *container = Object::cast_to<AppTabContainer>(p_node);
		tab_containers.push_back(container);
	}

	// TODO: panes

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		Error err = _parse_node(c);
		if (err) {
			left_sidebar = nullptr;
			central_area = nullptr;
			right_sidebar = nullptr;

			tab_containers.clear();
			return err;
		}
	}

	return OK;
}

bool AppNode::_load_main_scene() {
	String scene_path = _get_main_scene_path();
	Ref<PackedScene> sdata = ResourceLoader::load(scene_path, "", ResourceFormatLoader::CACHE_MODE_REPLACE_DEEP);
	if (sdata.is_null()) {
		return false;
	}

	sdata->set_path(scene_path, true); // Take over path.

	Node *new_scene = sdata->instantiate(PackedScene::GEN_EDIT_STATE_DISABLED);
	if (!new_scene) {
		return false;
	}
	new_scene->set_scene_instance_state(Ref<SceneState>());

	Error err = _parse_node(new_scene);
	if (err != OK || (left_sidebar == nullptr || central_area == nullptr || right_sidebar == nullptr) || tab_containers.is_empty()) {
		return false;
	}

	// TODO: restore the layout

	gui_main = new_scene;
	return true;
}

void AppNode::_update_main_menu_type() {
	bool use_menu_button = EDITOR_GET("interface/app/collapse_main_menu");
	bool global_menu = !bool(EDITOR_GET("interface/app/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);

	bool already_using_button = main_menu_button != nullptr;
	bool already_using_bar = main_menu_bar != nullptr;
	if ((use_menu_button && already_using_button) || (!use_menu_button && already_using_bar)) {
		return; // Already correctly configured.
	}

	if (use_menu_button && !global_menu) {
		main_menu_button = memnew(MenuButton);
		main_menu_button->set_text(TTRC("Main Menu"));
		main_menu_button->set_theme_type_variation("MainScreenButton");
		main_menu_button->set_focus_mode(Control::FOCUS_NONE);
		if (is_inside_tree()) {
			main_menu_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), EditorStringName(EditorIcons)));
		}
		main_menu_button->set_switch_on_hover(true);

		if (main_menu_bar != nullptr) {
			Vector<PopupMenu *> menus_to_move;
			for (int i = 0; i < main_menu_bar->get_child_count(); i++) {
				PopupMenu *menu = Object::cast_to<PopupMenu>(main_menu_bar->get_child(i));
				if (menu != nullptr) {
					menus_to_move.push_back(menu);
				}
			}
			for (PopupMenu *menu : menus_to_move) {
				main_menu_bar->remove_child(menu);
				main_menu_button->get_popup()->add_submenu_node_item(menu->get_name(), menu);
			}
		}

		title_bar->add_child(main_menu_button);
		if (menu_btn_spacer == nullptr) {
			title_bar->move_child(main_menu_button, 0);
		} else {
			title_bar->move_child(main_menu_button, menu_btn_spacer->get_index() + 1);
		}
		memdelete_notnull(main_menu_bar);
		main_menu_bar = nullptr;
	} else {
		main_menu_bar = memnew(MenuBar);
		main_menu_bar->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		main_menu_bar->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		main_menu_bar->set_theme_type_variation("MainMenuBar");
		main_menu_bar->set_start_index(0); // Main menu, add to the start of global menu.
		main_menu_bar->set_prefer_global_menu(global_menu);
		main_menu_bar->set_switch_on_hover(true);

		if (main_menu_button != nullptr) {
			Vector<PopupMenu *> menus_to_move;
			for (int i = 0; i < main_menu_button->get_item_count(); i++) {
				PopupMenu *menu = main_menu_button->get_popup()->get_item_submenu_node(i);
				if (menu != nullptr) {
					menus_to_move.push_back(menu);
				}
			}
			for (PopupMenu *menu : menus_to_move) {
				menu->get_parent()->remove_child(menu);
				main_menu_bar->add_child(menu);
			}
		}

		title_bar->add_child(main_menu_bar);
		title_bar->move_child(main_menu_bar, 0);

		memdelete_notnull(menu_btn_spacer);
		memdelete_notnull(main_menu_button);
		menu_btn_spacer = nullptr;
		main_menu_button = nullptr;
	}
}

void AppNode::_add_to_main_menu(const String &p_name, PopupMenu *p_menu) {
	p_menu->set_name(p_name);
	if (main_menu_button != nullptr) {
		main_menu_button->get_popup()->add_submenu_node_item(p_name, p_menu);
	} else {
		main_menu_bar->add_child(p_menu);
	}
}

void AppNode::_init_main_menu() {
	bool global_menu = !bool(EDITOR_GET("interface/app/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
	bool dark_mode = DisplayServer::get_singleton()->is_dark_mode_supported() && DisplayServer::get_singleton()->is_dark_mode();
	bool can_expand = false; // Windows is not supported.

	_update_main_menu_type();

	file_menu = memnew(PopupMenu);
	_add_to_main_menu(TTRC("File"), file_menu);

	file_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_menu_option));

	// TODO: Use Ctrl+T to create new tab, Ctrl+N to create new file.
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/new_tab", TTRC("New Tab"), KeyModifierMask::CMD_OR_CTRL + Key::N), FILE_NEW_TAB);
	file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/new_tab", TTRC("New Tab"), KeyModifierMask::CMD_OR_CTRL + Key::T), FILE_NEW_TAB);
	file_menu->add_separator();
	file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/close_tab", TTRC("Close Tab"), KeyModifierMask::CTRL + Key::W), FILE_CLOSE_TAB);

	file_menu->add_separator();
	file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/file_quit", TTRC("Quit"), KeyModifierMask::CMD_OR_CTRL + Key::Q), FILE_QUIT, true);

	view_menu = memnew(PopupMenu);
	_add_to_main_menu(TTRC("View"), view_menu);

	view_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_menu_option));

	view_menu->add_check_shortcut(ED_SHORTCUT_AND_COMMAND("app/left_sidebar", TTRC("Left Sidebar"), KeyModifierMask::CMD_OR_CTRL + Key::B), VIEW_LEFT_SIDEBAR);
	view_menu->set_item_checked(-1, true); // Note: The left sidebar is visible by default.

	help_menu = memnew(PopupMenu);
	if (global_menu && NativeMenu::get_singleton()->has_system_menu(NativeMenu::HELP_MENU_ID)) {
		help_menu->set_system_menu(NativeMenu::HELP_MENU_ID);
	}
	_add_to_main_menu(TTRC("Help"), help_menu);

	help_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_menu_option));

	// TODO: SHORTCUT_AND_COMMAND
	// ED_SHORTCUT_AND_COMMAND("app/editor_help", TTRC("Search Help..."), Key::F1);
	// ED_SHORTCUT_OVERRIDE("app/editor_help", "macos", KeyModifierMask::ALT | Key::SPACE);
	// help_menu->add_icon_shortcut(get_app_theme_native_menu_icon(SNAME("HelpSearch"), global_menu, dark_mode), ED_GET_SHORTCUT("app/editor_help"), HELP_SEARCH);
	// help_menu->add_separator();
	// help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/online_docs", TTRC("Online Documentation")), HELP_DOCS);
	// help_menu->add_separator();
	help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/github", "GitHub"), HELP_GITHUB);
	// help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/report_a_bug", TTRC("Report a Bug")), HELP_REPORT_A_BUG);
	help_menu->add_separator();
	help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("app/about", TTRC("About")), HELP_ABOUT);
	// help_menu->add_icon_shortcut(get_app_theme_native_menu_icon(SNAME("Heart"), global_menu, dark_mode), ED_SHORTCUT_AND_COMMAND("app/support_development", TTRC("Support Godot Development")), HELP_SUPPORT_GODOT_DEVELOPMENT);
}

void AppNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_load_layout();
		} break;

		case NOTIFICATION_ENTER_TREE: {
			// Theme has already been created in the constructor, so we can skip that step.
			_update_theme(true);
		} break;

		case AppSettings::NOTIFICATION_APP_SETTINGS_CHANGED: {
			follow_system_theme = APP_GET("interface/theme/follow_system_theme");
			use_system_accent_color = APP_GET("interface/theme/use_system_accent_color");
		} break;
	}
}

void AppNode::save_layout_delayed() {
	layout_save_delay_timer->start();
}

AppNode::AppNode() {
	DEV_ASSERT(!singleton);
	singleton = this;

	// File system.
	FileSystemAccess::create();
	file_system = memnew(FileSystem);

	// Load settings.
	if (!AppSettings::get_singleton()) {
		AppSettings::create();
	}

	{
		int display_scale = EDITOR_GET("interface/app/display_scale");

		switch (display_scale) {
			case 0:
				// Try applying a suitable display scale automatically.
				AppScale::set_scale(AppSettings::get_auto_display_scale());
				break;
			case 1:
				AppScale::set_scale(0.75);
				break;
			case 2:
				AppScale::set_scale(1.0);
				break;
			case 3:
				AppScale::set_scale(1.25);
				break;
			case 4:
				AppScale::set_scale(1.5);
				break;
			case 5:
				AppScale::set_scale(1.75);
				break;
			case 6:
				AppScale::set_scale(2.0);
				break;
			default:
				AppScale::set_scale(EDITOR_GET("interface/app/custom_display_scale"));
				break;
		}
	}

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		// TODO: Determine the minimum size based on the layout?
		// const Size2 minimum_size = Size2(1024, 600) * APP_SCALE;
		const Size2 minimum_size = Size2(640, 400) * APP_SCALE;
		w->set_min_size(minimum_size); // Calling it this early doesn't sync the property with DS.
		DisplayServer::get_singleton()->window_set_min_size(minimum_size);
	}

	AppThemeManager::initialize();
	theme = AppThemeManager::generate_theme();
	DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), AppStringName(App)));

	// App layout.
	gui_base = memnew(Panel);
	add_child(gui_base);

	// Take up all screen.
	gui_base->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	gui_base->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
	gui_base->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
	gui_base->set_end(Point2(0, 0));

	main_vbox = memnew(VBoxContainer);
	gui_base->add_child(main_vbox);
	main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE);

	// Title Bar.
	title_bar = memnew(HBoxContainer);
	main_vbox->add_child(title_bar);

	Control *spacer = memnew(Control);
	spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(spacer);

	// TODO
	// HBoxContainer *button_hbox = memnew(HBoxContainer);
	// title_bar->add_child(button_hbox);

	// Button *left_toggle_button = memnew(Button);
	// // left_toggle_button->set_flat(true);
	// left_toggle_button->set_theme_type_variation("MainScreenButton");
	// left_toggle_button->set_focus_mode(Control::FOCUS_NONE);
	// left_toggle_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	// left_toggle_button->set_tooltip_text(RTR("Toggle left sidebar"));
	// left_toggle_button->connect(SceneStringName(pressed), callable_mp(this, &AppNode::_toggle_left_sidebar));
	// button_hbox->add_child(left_toggle_button);

	// Button *right_toggle_button = memnew(Button);
	// // right_toggle_button->set_flat(true);
	// right_toggle_button->set_theme_type_variation("MainScreenButton");
	// right_toggle_button->set_focus_mode(Control::FOCUS_NONE);
	// right_toggle_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	// right_toggle_button->set_tooltip_text(RTR("Toggle right sidebar"));
	// right_toggle_button->connect(SceneStringName(pressed), callable_mp(this, &AppNode::_toggle_right_sidebar));
	// button_hbox->add_child(right_toggle_button);

	about = memnew(AppAbout);
	gui_base->add_child(about);
	_init_main_menu();

	// Body.
	HBoxContainer *hbox = memnew(HBoxContainer);
	hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(hbox);

	// // Ribbon.
	// ribbon = memnew(VBoxContainer);
	// hbox->add_child(ribbon);
	// // ribbon->set_custom_minimum_size(Size2(20, 20));
	// // ribbon->hide();

	// // TODO: VTabBar
	// TabBar *actions = memnew(TabBar);
	// ribbon->add_child(actions);
	// actions->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	// actions->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// actions->add_tab("action 1");
	// actions->add_tab("action 2");

	// Button *settings_button = memnew(Button);
	// ribbon->add_child(settings_button);
	// settings_button->set_button_icon(theme->get_icon(SNAME("Settings"), SNAME("AppIcons"))); // TODO: Settings icon

	pane_factory = memnew(PaneFactory);
	pane_factory->register_pane<FilePane>(
			FilePane::get_class_static(),
			theme->get_icon(SNAME("Folder"), SNAME("AppIcons")),
			callable_mp(this, &AppNode::_on_file_pane_create));
	pane_factory->register_pane<NavigationPane>(
			NavigationPane::get_class_static(),
			theme->get_icon(SNAME("Filesystem"), SNAME("AppIcons")),
			callable_mp(this, &AppNode::_on_navigation_pane_create));

	// TODO: SettingsPane, WelcomePane
	// pane_factory->register_pane<SettingsPane>(
	// 		SettingsPane::get_class_static(),
	// 		theme->get_icon(SNAME("Settings"), SNAME("AppIcons"))); // TODO: icon
	// pane_factory->register_pane<WelcomePane>(
	// 		WelcomePane::get_class_static(),
	// 		theme->get_icon(SNAME("Welcome"), SNAME("AppIcons"))); // TODO: icon

	container_manager = memnew(ContainerManager);
	gui_base->add_child(container_manager);

#define LOAD_SCENE 0 // TODO: load layout
	if (LOAD_SCENE && _load_main_scene()) {
		hbox->add_child(gui_main);
	} else {
		// Default layout.
		HSplitContainer *hsplit = memnew(HSplitContainer);
		hbox->add_child(hsplit);
		hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		gui_main = hsplit;

		// Left sidebar.
		left_sidebar = container_manager->create_container(LEFT_SIDEBAR_NAME, hsplit, gui_main);
		container_manager->new_tab(NavigationPane::get_class_static());

		// Central content area.
		central_area = container_manager->create_container(CENTRAL_AREA_NAME, hsplit, gui_main);
		central_area->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		central_area->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		container_manager->set_tab_closable(central_area, true);
		container_manager->set_tabs_rearrange_group(central_area, 1);
		container_manager->new_tab(FilePane::get_class_static());

		// Right sidebar.
		right_sidebar = container_manager->create_container(RIGHT_SIDEBAR_NAME, hsplit, gui_main);
		right_sidebar->hide();
	}

	ED_SHORTCUT("app/next_tab", TTRC("Next Tab"), KeyModifierMask::CTRL + Key::TAB);
	ED_SHORTCUT("app/prev_tab", TTRC("Previous Tab"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::TAB);

	// TODO: command_palette

	layout_save_delay_timer = memnew(Timer);
	add_child(layout_save_delay_timer);
	layout_save_delay_timer->set_wait_time(0.5);
	layout_save_delay_timer->set_one_shot(true);
	layout_save_delay_timer->connect("timeout", callable_mp(this, &AppNode::_save_layout));

	set_process(true);
	set_process_shortcut_input(true);
}

AppNode::~AppNode() {
	memdelete(pane_factory);
	memdelete(file_system);

	AppSettings::destroy();
	AppThemeManager::finalize();
	FileSystemAccess::destroy();

	singleton = nullptr;
}
