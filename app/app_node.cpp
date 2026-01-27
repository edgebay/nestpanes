#include "app_node.h"

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
// #include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"

#include "scene/main/scene_tree.h"

#include "scene/resources/packed_scene.h"

#include "scene/theme/theme_db.h"

#include "servers/display_server.h"

#include "app/app_core/app_paths.h"
#include "app/app_modules/settings/app_settings.h"
#include "app/app_string_names.h"
#include "app/themes/app_scale.h"
#include "app/themes/app_theme_manager.h"

#include "app/app_core/io/file_system_access.h"

#include "app/app_modules/file_management/file_system_list.h"
#include "app/app_modules/file_management/file_system_tree.h"
#include "app/app_modules/text_editing/text_editor.h"
#include "app/gui/app_about.h"
#include "app/gui/app_tab_container.h"
#include "app/gui/container_manager.h"
#include "app/gui/multi_split_container.h"
#include "app/gui/pane_factory.h"

#include "app/app_modules/settings/gui/settings_pane.h"
#include "app/gui/welcome_pane.h"

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

		// editor_main_screen->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("Content"), EditorStringName(EditorStyles)));
		// bottom_panel->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("BottomPanel"), EditorStringName(EditorStyles)));
		// distraction_free->set_button_icon(theme->get_icon(SNAME("DistractionFree"), EditorStringName(EditorIcons)));
		// distraction_free->add_theme_style_override(SceneStringName(pressed), theme->get_stylebox(CoreStringName(normal), "FlatMenuButton"));

		// // Do not set icon.
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_SEARCH), get_app_theme_native_menu_icon(SNAME("HelpSearch"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_COPY_SYSTEM_INFO), get_app_theme_native_menu_icon(SNAME("ActionCopy"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_ABOUT), get_app_theme_native_menu_icon(SNAME("Godot"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_SUPPORT_GODOT_DEVELOPMENT), get_app_theme_native_menu_icon(SNAME("Heart"), global_menu, dark_mode));

		// _update_renderer_color();
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

		// // Do not set icon.
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_SEARCH), get_app_theme_native_menu_icon(SNAME("HelpSearch"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_COPY_SYSTEM_INFO), get_app_theme_native_menu_icon(SNAME("ActionCopy"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_ABOUT), get_app_theme_native_menu_icon(SNAME("Godot"), global_menu, dark_mode));
		// help_menu->set_item_icon(help_menu->get_item_index(HELP_SUPPORT_GODOT_DEVELOPMENT), get_app_theme_native_menu_icon(SNAME("Heart"), global_menu, dark_mode));
		// editor_dock_manager->update_docks_menu();
	}
}

void AppNode::_menu_option(int p_option) {
	_menu_option_confirm(p_option, false);
}

void AppNode::_menu_confirm_current() {
	// _menu_option_confirm(current_menu_option, true);
}

void AppNode::_menu_option_confirm(int p_option, bool p_confirmed) {
	switch (p_option) {
		case SCENE_QUIT: {
			_exit(EXIT_SUCCESS);
			// if (p_confirmed && plugin_to_save) {
			// 	plugin_to_save->save_external_data();
			// 	p_confirmed = false;
			// }

			// if (p_confirmed && stop_project_confirmation && project_run_bar->is_playing()) {
			// 	project_run_bar->stop_playing();
			// 	stop_project_confirmation = false;
			// 	p_confirmed = false;
			// }

			// if (!p_confirmed) {
			// 	if (!stop_project_confirmation && project_run_bar->is_playing()) {
			// 		if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
			// 			confirmation->set_text(TTR("Stop running project before reloading the current project?"));
			// 			confirmation->set_ok_button_text(TTR("Stop & Reload"));
			// 		} else {
			// 			confirmation->set_text(TTR("Stop running project before exiting the editor?"));
			// 			confirmation->set_ok_button_text(TTR("Stop & Quit"));
			// 		}
			// 		confirmation->reset_size();
			// 		confirmation->popup_centered();
			// 		confirmation_button->hide();
			// 		stop_project_confirmation = true;
			// 		break;
			// 	}

			// 	bool save_each = EDITOR_GET("interface/app/save_each_scene_on_quit");
			// 	if (_next_unsaved_scene(!save_each) == -1) {
			// 		if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(EditorUndoRedoManager::GLOBAL_HISTORY)) {
			// 			if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
			// 				save_confirmation->set_ok_button_text(TTR("Save & Reload"));
			// 				save_confirmation->set_text(TTR("Save modified resources before reloading?"));
			// 			} else {
			// 				save_confirmation->set_ok_button_text(TTR("Save & Quit"));
			// 				save_confirmation->set_text(TTR("Save modified resources before closing?"));
			// 			}
			// 			save_confirmation->reset_size();
			// 			save_confirmation->popup_centered();
			// 			break;
			// 		}

			// 		plugin_to_save = nullptr;
			// 		for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
			// 			const String unsaved_status = editor_data.get_editor_plugin(i)->get_unsaved_status();
			// 			if (!unsaved_status.is_empty()) {
			// 				if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
			// 					save_confirmation->set_ok_button_text(TTR("Save & Reload"));
			// 					save_confirmation->set_text(unsaved_status);
			// 				} else {
			// 					save_confirmation->set_ok_button_text(TTR("Save & Quit"));
			// 					save_confirmation->set_text(unsaved_status);
			// 				}
			// 				save_confirmation->reset_size();
			// 				save_confirmation->popup_centered();
			// 				plugin_to_save = editor_data.get_editor_plugin(i);
			// 				break;
			// 			}
			// 		}

			// 		if (plugin_to_save) {
			// 			break;
			// 		}

			// 		_discard_changes();
			// 		break;
			// 	}

			// 	if (save_each) {
			// 		tab_closing_menu_option = current_menu_option;
			// 		for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
			// 			tabs_to_close.push_back(editor_data.get_scene_path(i));
			// 		}
			// 		_proceed_closing_scene_tabs();
			// 	} else {
			// 		String unsaved_scenes;
			// 		int i = _next_unsaved_scene(true, 0);
			// 		while (i != -1) {
			// 			unsaved_scenes += "\n            " + editor_data.get_edited_scene_root(i)->get_scene_file_path();
			// 			i = _next_unsaved_scene(true, ++i);
			// 		}
			// 		if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
			// 			save_confirmation->set_ok_button_text(TTR("Save & Reload"));
			// 			save_confirmation->set_text(TTR("Save changes to the following scene(s) before reloading?") + unsaved_scenes);
			// 		} else {
			// 			save_confirmation->set_ok_button_text(TTR("Save & Quit"));
			// 			save_confirmation->set_text((p_option == SCENE_QUIT ? TTR("Save changes to the following scene(s) before quitting?") : TTR("Save changes to the following scene(s) before opening Project Manager?")) + unsaved_scenes);
			// 		}
			// 		save_confirmation->reset_size();
			// 		save_confirmation->popup_centered();
			// 	}

			// 	DisplayServer::get_singleton()->window_request_attention();
			// 	break;
			// }
			// _save_external_resources();
			// _discard_changes();
		} break;

		// case EDITOR_COMMAND_PALETTE: {
		// 	command_palette->open_popup();
		// } break;
		// case HELP_SEARCH: {
		// 	emit_signal(SNAME("request_help_search"), "");
		// } break;
		// case HELP_DOCS: {
		// 	OS::get_singleton()->shell_open(GODOT_VERSION_DOCS_URL "/");
		// } break;
		// case HELP_FORUM: {
		// 	OS::get_singleton()->shell_open("https://forum.godotengine.org/");
		// } break;
		case HELP_REPORT_A_BUG: {
			OS::get_singleton()->shell_open("https://github.com/edgebay/daily-engine/issues");
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

int AppNode::_new_tab(AppTabContainer *p_parent) {
	int tab_index = p_parent->get_tab_count();

	FileSystemList *file_system_list = memnew(FileSystemList);
	file_system_list->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	p_parent->add_child(file_system_list);
	file_system_list->set_owner(gui_main);
	file_system_lists.push_back(file_system_list);

	// Update tab.
	String path = file_system_list->get_current_path();
	String title = path.get_file();
	if (title.is_empty()) {
		title = path;
	}
	p_parent->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
	p_parent->set_tab_icon(tab_index, file_system_list->get_current_dir_icon());
	p_parent->set_tab_title(tab_index, title);
	p_parent->set_current_tab(tab_index);

	container_manager->set_current_tab_container(p_parent);

	return tab_index;
}

int AppNode::_new_editor(AppTabContainer *p_parent, const String &p_path) {
	int tab_index = p_parent->get_tab_count();

	TextEditor *editor = memnew(TextEditor);
	// editor->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	p_parent->add_child(editor);
	editor->set_owner(gui_main);
	// editors.push_back(editor);

	// Ref<TextFile> text_file = edited_res;
	// editor->get_text_editor()->set_text(text_file->get_text());
	editor->open_file(p_path);

	// Update tab.
	String path = p_path;
	String title = path.get_file();
	if (title.is_empty()) {
		title = path;
	}
	p_parent->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
	p_parent->set_tab_icon(tab_index, editor->get_app_theme_icon(SNAME("File")));
	p_parent->set_tab_title(tab_index, title);
	p_parent->set_current_tab(tab_index);

	container_manager->set_current_tab_container(p_parent);

	return tab_index;
}

// void AppNode::_on_tab_path_changed(const String &p_path) {
// 	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_fs->get_parent());
// 	if (tab_container) {
// 		String title = p_path.get_file();
// 		if (title.is_empty()) {
// 			title = p_path;
// 		}
// 		int tab_index = tab_container->get_current_tab();
// 		tab_container->set_tab_title(tab_index, title);
// 	}
// }

void AppNode::_on_tab_path_changed(FileSystemControl *p_fs) {
	AppTabContainer *tab_container = Object::cast_to<AppTabContainer>(p_fs->get_parent());
	if (tab_container) {
		String title = p_fs->get_current_dir_name();
		int tab_index = tab_container->get_current_tab();
		tab_container->set_tab_icon(tab_index, p_fs->get_current_dir_icon());
		tab_container->set_tab_title(tab_index, title);

		container_manager->set_current_tab_container(tab_container); // TODO: when tab activated/foucsed?
	}
	_save_layout();
}

void AppNode::_on_tree_item_activated(const String &p_path, bool is_dir) {
	AppTabContainer *current_tab_container = container_manager->get_current_tab_container();
	if (is_dir) {
		if (current_tab_container == nullptr) {
			return;
		}
		int tab_index = _new_tab(current_tab_container);
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(current_tab_container->get_tab_control(tab_index));
		file_system_list->set_current_path(p_path);
	} else {
		if (current_tab_container == nullptr) {
			return;
		}
		// TODO: FileSystemList::_item_dc_selected
		_new_editor(current_tab_container, p_path);
	}
}

// void AppNode::_on_tree_item_selected(TreeItem *p_item) {
void AppNode::_on_tree_item_selected(const String &p_path, bool is_dir) {
	AppTabContainer *current_tab_container = container_manager->get_current_tab_container();
	if (is_dir) {
		if (current_tab_container == nullptr) {
			return;
		}
		FileSystemList *file_system_list = Object::cast_to<FileSystemList>(current_tab_container->get_current_tab_control());
		// TODO: handle file item
		if (file_system_list) {
			file_system_list->set_current_path(p_path);
		}
	}
}

void AppNode::_exit(int p_exit_code) {
	exiting = true;
	// waiting_for_first_scan = false;
	// resource_preview->stop(); // Stop early to avoid crashes.
	// _save_editor_layout();
	_save_layout();

	// // Dim the editor window while it's quitting to make it clearer that it's busy.
	// dim_editor(true);

	// // Unload addons before quitting to allow cleanup.
	// unload_editor_addons();

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
				print_line(current_tab_container->get_current_tab(), ", next_tab: ", next_tab);
				current_tab_container->set_current_tab(next_tab);
			} else {
				is_handled = false;
			}
		} else if (ED_IS_SHORTCUT("app/prev_tab", p_event)) {
			AppTabContainer *current_tab_container = container_manager->get_current_tab_container();
			int tab_count = current_tab_container ? current_tab_container->get_tab_count() : 0;
			if (tab_count > 0) {
				int next_tab = current_tab_container->get_current_tab() - 1;
				next_tab = next_tab >= 0 ? next_tab : tab_count - 1;
				print_line(current_tab_container->get_current_tab(), ", next_tab: ", next_tab);
				current_tab_container->set_current_tab(next_tab);
			} else {
				is_handled = false;
			}
		}
		// if (ED_IS_SHORTCUT("editor/filter_files", p_event)) {
		// 	FileSystemDock::get_singleton()->focus_on_filter();
		// } else if (ED_IS_SHORTCUT("editor/editor_2d", p_event)) {
		// 	editor_main_screen->select(EditorMainScreen::EDITOR_2D);
		// } else if (ED_IS_SHORTCUT("editor/editor_3d", p_event)) {
		// 	editor_main_screen->select(EditorMainScreen::EDITOR_3D);
		// } else if (ED_IS_SHORTCUT("editor/editor_script", p_event)) {
		// 	editor_main_screen->select(EditorMainScreen::EDITOR_SCRIPT);
		// } else if (ED_IS_SHORTCUT("editor/editor_game", p_event)) {
		// 	editor_main_screen->select(EditorMainScreen::EDITOR_GAME);
		// } else if (ED_IS_SHORTCUT("editor/editor_help", p_event)) {
		// 	emit_signal(SNAME("request_help_search"), "");
		// } else if (ED_IS_SHORTCUT("editor/editor_assetlib", p_event) && AssetLibraryEditorPlugin::is_available()) {
		// 	editor_main_screen->select(EditorMainScreen::EDITOR_ASSETLIB);
		// } else if (ED_IS_SHORTCUT("editor/editor_next", p_event)) {
		// 	editor_main_screen->select_next();
		// } else if (ED_IS_SHORTCUT("editor/editor_prev", p_event)) {
		// 	editor_main_screen->select_prev();
		// } else if (ED_IS_SHORTCUT("editor/command_palette", p_event)) {
		// 	_open_command_palette();
		// } else if (ED_IS_SHORTCUT("editor/toggle_last_opened_bottom_panel", p_event)) {
		// 	bottom_panel->toggle_last_opened_bottom_panel();
		else {
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

	Ref<PackedScene> sdata;
	sdata.instantiate();
	Error err = sdata->pack(gui_main);
	if (err != OK) {
		// show_accept(TTR("Couldn't save scene. Likely dependencies (instances or inheritance) couldn't be satisfied."), TTR("OK"));
		return;
	}

	int flg = 0;
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	String scene_path = _get_main_scene_path();
	err = ResourceSaver::save(sdata, scene_path, flg);
	// if (err == OK) {
	// 	scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_file));
	// 	editor_data.set_scene_as_saved(idx);
	// 	editor_data.set_scene_modified_time(idx, FileAccess::get_modified_time(p_file));

	// 	editor_folding.save_scene_folding(scene, p_file);

	// 	_update_title();
	// 	scene_tabs->update_scene_tabs();
	// } else {
	// 	_dialog_display_save_error(p_file, err);
	// }

	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(config_path);

	for (auto control : file_system_trees) { // TODO: file_system_lists
		control->save_layout_to_config(config, "docks");
	}
	// for (int i = 0; i < file_system_controls.size(); i++) {
	// 	FileSystemControl *control = file_system_controls[i];
	// 	control->save_layout_to_config(config, "file_system_" + itos(i));
	// }

	// _save_open_scenes_to_config(config);
	// _save_central_editor_layout_to_config(config);
	// _save_window_settings_to_config(config, "EditorWindow");
	// editor_data.get_plugin_window_layout(config);

	config->save(config_path);
}

void AppNode::_load_layout() {
	// EditorProgress ep("loading_editor_layout", TTR("Loading editor"), 5);
	// ep.step(TTR("Loading editor layout..."), 0, true);
	Ref<ConfigFile> config;
	String config_path = _get_config_path();
	config.instantiate();
	Error err = config->load(config_path);
	if (err != OK) { // No config.
		// // If config is not found, expand the res:// folder and favorites by default.
		// TreeItem *root = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("res://", 0);
		// if (root) {
		// 	root->set_collapsed(false);
		// }

		// TreeItem *favorites = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("Favorites", 0);
		// if (favorites) {
		// 	favorites->set_collapsed(false);
		// }

		// if (overridden_default_layout >= 0) {
		// 	_layout_menu_option(overridden_default_layout);
		// }
	} else {
		// ep.step(TTR("Loading docks..."), 1, true);
		// editor_dock_manager->load_docks_from_config(config, "docks", true);
		for (auto control : file_system_trees) {
			control->load_layout_from_config(config, "docks");
		}

		// ep.step(TTR("Reopening scenes..."), 2, true);
		// _load_open_scenes_from_config(config);

		// ep.step(TTR("Loading central editor layout..."), 3, true);
		// _load_central_editor_layout_from_config(config);

		// ep.step(TTR("Loading plugin window layout..."), 4, true);
		// editor_data.set_plugin_window_layout(config);

		// ep.step(TTR("Editor layout ready."), 5, true);
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
	} else if (Object::cast_to<FileSystemTree>(p_node)) {
		FileSystemTree *control = Object::cast_to<FileSystemTree>(p_node);
		file_system_trees.push_back(control);
	} else if (Object::cast_to<FileSystemList>(p_node)) {
		FileSystemList *control = Object::cast_to<FileSystemList>(p_node);
		file_system_lists.push_back(control);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		Error err = _parse_node(c);
		if (err) {
			left_sidebar = nullptr;
			central_area = nullptr;
			right_sidebar = nullptr;

			tab_containers.clear();
			file_system_trees.clear();
			file_system_lists.clear();
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

	for (auto container : tab_containers) {
		// TODO
		// container->set_popup(split_menu);
		// container->connect("pre_popup_pressed", callable_mp(this, &AppNode::_select_tab_container).bind(container));
		// container->connect("new_tab", callable_mp(this, &AppNode::_new_tab).bind(container));
		// container->connect("emptied", callable_mp(this, &AppNode::_tab_container_emptied).bind(container));

		// TODO: 1. Other classes, 2. use metadata/_tab_name
		// Update tab icon and text
		for (int i = 0; i < container->get_child_count(); i++) {
			Node *control = container->get_child(i);
			if (Object::cast_to<FileSystemList>(control)) {
				FileSystemList *file_system_list = Object::cast_to<FileSystemList>(control);
				int tab_index = container->get_tab_idx_from_control(file_system_list);
				String path = file_system_list->get_current_path();
				String title = path.get_file();
				if (title.is_empty()) {
					title = path;
				}
				container->set_tab_icon_max_width(tab_index, theme->get_constant(SNAME("class_icon_size"), AppStringName(App)));
				container->set_tab_icon(tab_index, file_system_list->get_current_dir_icon());
				container->set_tab_title(tab_index, title);
			}
		}
	}
	for (auto control : file_system_trees) {
		control->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
		control->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
		control->connect("item_collapsed", callable_mp(this, &AppNode::save_layout_delayed));
	}
	for (auto control : file_system_lists) {
		control->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
	}

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
	// bool can_expand = bool(EDITOR_GET("interface/app/expand_to_title")) && DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_EXTEND_TO_TITLE);
	bool can_expand = false; // Windows is not supported.

	_update_main_menu_type();

	file_menu = memnew(PopupMenu);
	_add_to_main_menu(TTRC("File"), file_menu);

	file_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_menu_option));

	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/new_scene", TTRC("New Scene"), KeyModifierMask::CMD_OR_CTRL + Key::N), SCENE_NEW_SCENE);
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/new_inherited_scene", TTRC("New Inherited Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::N), SCENE_NEW_INHERITED_SCENE);
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/open_scene", TTRC("Open Scene..."), KeyModifierMask::CMD_OR_CTRL + Key::O), SCENE_OPEN_SCENE);
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/reopen_closed_scene", TTRC("Reopen Closed Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::T), SCENE_OPEN_PREV);

	// recent_scenes = memnew(PopupMenu);
	// recent_scenes->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	// file_menu->add_submenu_node_item(TTRC("Open Recent"), recent_scenes, SCENE_OPEN_RECENT);
	// recent_scenes->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_open_recent_scene));

	// file_menu->add_separator();
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/save_scene", TTRC("Save Scene"), KeyModifierMask::CMD_OR_CTRL + Key::S), SCENE_SAVE_SCENE);
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/save_scene_as", TTRC("Save Scene As..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::S), SCENE_SAVE_AS_SCENE);
	// file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/save_all_scenes", TTRC("Save All Scenes"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::S), SCENE_SAVE_ALL_SCENES);

	file_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/file_quit", TTRC("Quit"), KeyModifierMask::CMD_OR_CTRL + Key::Q), SCENE_QUIT, true);

	view_menu = memnew(PopupMenu);
	_add_to_main_menu(TTRC("View"), view_menu);

	// tool_menu = memnew(PopupMenu);
	// tool_menu->connect("index_pressed", callable_mp(this, &AppNode::_tool_menu_option));
	// project_menu->add_submenu_node_item(TTRC("Tools"), tool_menu);
	// tool_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/orphan_resource_explorer", TTRC("Orphan Resource Explorer...")), TOOLS_ORPHAN_RESOURCES);
	// tool_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/engine_compilation_configuration_editor", TTRC("Engine Compilation Configuration Editor...")), TOOLS_BUILD_PROFILE_MANAGER);
	// tool_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/upgrade_project", TTRC("Upgrade Project Files...")), TOOLS_PROJECT_UPGRADE);

	help_menu = memnew(PopupMenu);
	if (global_menu && NativeMenu::get_singleton()->has_system_menu(NativeMenu::HELP_MENU_ID)) {
		help_menu->set_system_menu(NativeMenu::HELP_MENU_ID);
	}
	_add_to_main_menu(TTRC("Help"), help_menu);

	help_menu->connect(SceneStringName(id_pressed), callable_mp(this, &AppNode::_menu_option));

	// TODO: "app/
	// TODO: SHORTCUT_AND_COMMAND
	ED_SHORTCUT_AND_COMMAND("editor/editor_help", TTRC("Search Help..."), Key::F1);
	ED_SHORTCUT_OVERRIDE("editor/editor_help", "macos", KeyModifierMask::ALT | Key::SPACE);
	// help_menu->add_icon_shortcut(get_app_theme_native_menu_icon(SNAME("HelpSearch"), global_menu, dark_mode), ED_GET_SHORTCUT("editor/editor_help"), HELP_SEARCH);
	// help_menu->add_separator();
	// help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/online_docs", TTRC("Online Documentation")), HELP_DOCS);
	// help_menu->add_separator();
	help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/report_a_bug", TTRC("Report a Bug")), HELP_REPORT_A_BUG);
	help_menu->add_separator();
	help_menu->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/about", TTRC("About")), HELP_ABOUT);
	// // Do not set icon.
	// if (!global_menu || !OS::get_singleton()->has_feature("macos")) {
	// 	// On macOS  "Quit" and "About" options are in the "app" menu.
	// 	help_menu->add_icon_shortcut(get_app_theme_native_menu_icon(SNAME("Godot"), global_menu, dark_mode), ED_SHORTCUT_AND_COMMAND("editor/about", TTRC("About")), HELP_ABOUT);
	// }
	// help_menu->add_icon_shortcut(get_app_theme_native_menu_icon(SNAME("Heart"), global_menu, dark_mode), ED_SHORTCUT_AND_COMMAND("editor/support_development", TTRC("Support Godot Development")), HELP_SUPPORT_GODOT_DEVELOPMENT);
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

	// TODO: main.cpp DisplayServer::get_singleton()->window_set_title(appname);
	SceneTree::get_singleton()->get_root()->set_title(APP_VERSION_NAME);

	// File system.
	FileSystemAccess::create();

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
		const Size2 minimum_size = Size2(1024, 600) * APP_SCALE;
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

	// Title Bar
	title_bar = memnew(HBoxContainer);
	main_vbox->add_child(title_bar);

	Control *spacer = memnew(Control);
	spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(spacer);

	HBoxContainer *button_hbox = memnew(HBoxContainer);
	title_bar->add_child(button_hbox);

	Button *left_toggle_button = memnew(Button);
	// left_toggle_button->set_flat(true);
	left_toggle_button->set_theme_type_variation("MainScreenButton");
	left_toggle_button->set_focus_mode(Control::FOCUS_NONE);
	left_toggle_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	left_toggle_button->set_tooltip_text(RTR("Toggle Left Sidebar"));
	button_hbox->add_child(left_toggle_button);

	Button *right_toggle_button = memnew(Button);
	// right_toggle_button->set_flat(true);
	right_toggle_button->set_theme_type_variation("MainScreenButton");
	right_toggle_button->set_focus_mode(Control::FOCUS_NONE);
	right_toggle_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	right_toggle_button->set_tooltip_text(RTR("Toggle Right Sidebar"));
	button_hbox->add_child(right_toggle_button);

	about = memnew(AppAbout);
	gui_base->add_child(about);
	_init_main_menu();

	// Body
	HBoxContainer *hbox = memnew(HBoxContainer);
	hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(hbox);

	// Ribbon
	ribbon = memnew(VBoxContainer);
	hbox->add_child(ribbon);
	// ribbon->set_custom_minimum_size(Size2(20, 20));
	// ribbon->hide();

	// TODO: VTabBar
	TabBar *actions = memnew(TabBar);
	ribbon->add_child(actions);
	actions->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	actions->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	actions->add_tab("1");
	actions->add_tab("2");

	Button *settings_button = memnew(Button);
	ribbon->add_child(settings_button);
	settings_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO

	pane_factory = memnew(PaneFactory);
	pane_factory->register_pane<SettingsPane>(
			SettingsPane::get_class_static(),
			theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO

	// pane_factory->register_pane<WelcomePane>("WelcomePane", theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	pane_factory->register_pane<WelcomePane>(
			WelcomePane::get_class_static(),
			theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons"))); // TODO
	// pane_factory->register_pane<WelcomePane>(
	// 		WelcomePane::get_class_static(),
	// 		theme->get_icon(SNAME("TripleBar"), SNAME("AppIcons")), // TODO
	// 		callable_mp(this, &AppNode::_new_tab));

	container_manager = memnew(ContainerManager);
	container_manager->init_popup_menu(gui_base);

#define LOAD_SCENE 0
	if (LOAD_SCENE && _load_main_scene()) {
		hbox->add_child(gui_main);
	} else {
		// Default layout.
		HSplitContainer *left_hsplit = memnew(HSplitContainer);
		hbox->add_child(left_hsplit);
		left_hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		left_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		gui_main = left_hsplit;

		// Left sidebar.
		FileSystemTree *file_system_tree = memnew(FileSystemTree);
		// left_tab_container->add_child(file_system_tree);
		file_system_tree->connect("item_activated", callable_mp(this, &AppNode::_on_tree_item_activated));
		file_system_tree->connect("item_selected", callable_mp(this, &AppNode::_on_tree_item_selected));
		file_system_tree->connect("item_collapsed", callable_mp(this, &AppNode::save_layout_delayed));
		// file_system_tree->set_owner(gui_main);
		file_system_trees.push_back(file_system_tree);
		left_sidebar = container_manager->create_container(LEFT_SIDEBAR_NAME, left_hsplit, gui_main, file_system_tree);

		HSplitContainer *right_hsplit = memnew(HSplitContainer);
		left_hsplit->add_child(right_hsplit);
		right_hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		right_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		right_hsplit->set_owner(gui_main);

		// Tabs.
		// _new_tab(tab_container);
		FileSystemList *file_system_list = memnew(FileSystemList);
		file_system_list->connect("path_changed", callable_mp(this, &AppNode::_on_tab_path_changed));
		file_system_lists.push_back(file_system_list);
		central_area = container_manager->create_container(CENTRAL_AREA_NAME, right_hsplit, gui_main, file_system_list);
		central_area->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		central_area->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		// Right sidebar.
		right_sidebar = container_manager->create_container(RIGHT_SIDEBAR_NAME, right_hsplit, gui_main);

		container_manager->set_current_tab_container(Object::cast_to<AppTabContainer>(central_area->get_child(0, false)));
	}

	left_toggle_button->connect(SceneStringName(pressed), callable_mp(this, &AppNode::_toggle_left_sidebar));
	right_toggle_button->connect(SceneStringName(pressed), callable_mp(this, &AppNode::_toggle_right_sidebar));

	ED_SHORTCUT("app/next_tab", TTRC("Next Tab"), KeyModifierMask::CTRL + Key::TAB);
	ED_SHORTCUT("app/prev_tab", TTRC("Previous Tab"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::TAB);
	// ED_SHORTCUT("app/filter_files", TTRC("Focus FileSystem Filter"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::P);

	// command_palette = EditorCommandPalette::get_singleton();
	// command_palette->set_title(TTR("Command Palette"));
	// gui_base->add_child(command_palette);

	layout_save_delay_timer = memnew(Timer);
	add_child(layout_save_delay_timer);
	layout_save_delay_timer->set_wait_time(0.5);
	layout_save_delay_timer->set_one_shot(true);
	layout_save_delay_timer->connect("timeout", callable_mp(this, &AppNode::_save_layout));

	set_process(true);

	set_process_shortcut_input(true);

	// follow_system_theme = EDITOR_GET("interface/theme/follow_system_theme");
	// use_system_accent_color = EDITOR_GET("interface/theme/use_system_accent_color");
	// system_theme_timer = memnew(Timer);
	// system_theme_timer->set_wait_time(1.0);
	// system_theme_timer->connect("timeout", callable_mp(this, &AppNode::_check_system_theme_changed));
	// add_child(system_theme_timer);
	// system_theme_timer->set_owner(get_owner());
	// system_theme_timer->set_autostart(true);
}

AppNode::~AppNode() {
	memdelete(container_manager);
	memdelete(pane_factory);

	AppSettings::destroy();
	AppThemeManager::finalize();
	FileSystemAccess::destroy();

	singleton = nullptr;
}
