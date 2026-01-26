#pragma once

#include "scene/gui/container.h"
#include "scene/gui/popup.h"
#include "scene/gui/tab_bar.h"

class Button;
class HBoxContainer;
class Panel;
class PanelContainer;
class PopupMenu;
class TextureRect;

class DropOverlay : public Control {
	GDCLASS(DropOverlay, Control);

public:
	enum DropPosition {
		DROP_UP,
		DROP_DOWN,
		DROP_LEFT,
		DROP_RIGHT,
		DROP_CENTER,
	};

private:
	bool is_hovering = false;
	DropPosition drop_position = DropPosition::DROP_CENTER;

	bool position_detection = true;

	DropPosition _get_position(const Point2 &p_point) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	void drop_data(const Point2 &p_point, const Variant &p_data) override;

	void gui_input(const Ref<InputEvent> &p_event) override;

	void enable_position_detection(bool p_enable);
	bool get_position_detection() const;
};

class AppTabContainer : public Container {
	GDCLASS(AppTabContainer, Container);

public:
	enum MenuOptions {
		// Scene menu.
		FILE_NEW_SCENE,
	};

	enum TabPosition {
		POSITION_TOP,
		POSITION_BOTTOM,
		POSITION_MAX,
	};

	using DropPosition = DropOverlay::DropPosition;

private:
	PanelContainer *tabbar_panel = nullptr;
	HBoxContainer *tabbar_container = nullptr;

	TabBar *tab_bar = nullptr;
	PopupMenu *tab_bar_context_menu = nullptr;
	Button *tab_add = nullptr;
	Control *tab_add_ph = nullptr;

	Panel *tab_preview_panel = nullptr;
	TextureRect *tab_preview = nullptr;

	bool tabs_visible = true;
	TabPosition tabs_position = POSITION_TOP;
	bool menu_hovered = false;
	mutable ObjectID popup_obj_id;
	bool use_hidden_tabs_for_min_size = false;
	bool theme_changing = false;
	Vector<Control *> children_removing;
	bool drag_to_rearrange_enabled = false;
	// Set the default setup current tab to be an invalid index.
	int setup_current_tab = -2;
	bool updating_visibility = false;

	bool new_tab_enabled = false;

	int current_menu_option = 0;

	bool split_dragging = false;
	DropOverlay *drop_overlay = nullptr;

	struct ThemeCache {
		int side_margin = 0;

		Ref<StyleBox> panel_style;
		Ref<StyleBox> tabbar_style;

		Ref<Texture2D> menu_icon;
		Ref<Texture2D> menu_hl_icon;

		// TabBar overrides.
		int icon_separation = 0;
		int tab_separation = 0;
		int icon_max_width = 0;
		int outline_size = 0;

		Ref<StyleBox> tab_unselected_style;
		Ref<StyleBox> tab_hovered_style;
		Ref<StyleBox> tab_selected_style;
		Ref<StyleBox> tab_disabled_style;
		Ref<StyleBox> tab_focus_style;

		Ref<Texture2D> increment_icon;
		Ref<Texture2D> increment_hl_icon;
		Ref<Texture2D> decrement_icon;
		Ref<Texture2D> decrement_hl_icon;
		Ref<Texture2D> drop_mark_icon;
		Color drop_mark_color;

		Color font_selected_color;
		Color font_hovered_color;
		Color font_unselected_color;
		Color font_disabled_color;
		Color font_outline_color;

		Ref<Font> tab_font;
		int tab_font_size;
	} theme_cache;

	void _menu_option_confirm(int p_option, bool p_confirmed);
	void _update_context_menu();
	void _custom_menu_option(int p_option);

	void _reposition_active_tab(int p_to_index);

	Rect2 _get_tab_rect() const;
	int _get_tab_height() const;
	Vector<Control *> _get_tab_controls() const;
	void _on_theme_changed();
	void _repaint();
	void _refresh_tab_indices();
	void _refresh_tab_names();
	void _update_margins();
	void _on_mouse_exited();

	void _on_tab_changed(int p_tab);
	void _on_tab_clicked(int p_tab);
	void _on_tab_closed(int p_tab);
	void _on_tab_hovered(int p_tab);
	void _on_tab_exit();
	void _on_tab_input(const Ref<InputEvent> &p_input);
	void _on_tab_resized();
	void _on_tab_selected(int p_tab);
	void _on_tab_button_pressed(int p_tab);
	void _on_active_tab_rearranged(int p_tab);
	void _on_tab_visibility_changed(Control *p_child);

	void _tab_preview_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, const Variant &p_udata);

	Variant _get_drag_data_fw(const Point2 &p_point, Control *p_from_control);
	bool _can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) const;
	void _drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control);
	void _drag_move_tab(int p_from_index, int p_to_index);
	void _drag_move_tab_from(TabBar *p_from_tabbar, int p_from_index, int p_to_index);
	void _on_drop_data(const Point2 &p_point, const Variant &p_data, int p_position);

	bool _is_internal_child(Node *p_node) const;

	AppTabContainer *_get_tab_container_from_tab_bar(TabBar *p_child) const;

protected:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_key_input(const Ref<InputEvent> &p_event) override;

	void _notification(int p_what);

	virtual void add_child_notify(Node *p_child) override;
	virtual void move_child_notify(Node *p_child) override;
	virtual void remove_child_notify(Node *p_child) override;

	static void _bind_methods();

public:
	TabBar *get_tab_bar() const;

	int get_tab_idx_at_point(const Point2 &p_point) const;
	int get_tab_idx_from_control(Control *p_child) const;

	void set_tab_alignment(TabBar::AlignmentMode p_alignment);
	TabBar::AlignmentMode get_tab_alignment() const;

	void set_tabs_position(TabPosition p_tab_position);
	TabPosition get_tabs_position() const;

	void set_tab_focus_mode(FocusMode p_focus_mode);
	FocusMode get_tab_focus_mode() const;

	void set_clip_tabs(bool p_clip_tabs);
	bool get_clip_tabs() const;

	void set_tabs_visible(bool p_visible);
	bool are_tabs_visible() const;

	void set_tab_title(int p_tab, const String &p_title);
	String get_tab_title(int p_tab) const;

	void set_tab_tooltip(int p_tab, const String &p_tooltip);
	String get_tab_tooltip(int p_tab) const;

	void set_tab_icon(int p_tab, const Ref<Texture2D> &p_icon);
	Ref<Texture2D> get_tab_icon(int p_tab) const;

	void set_tab_icon_max_width(int p_tab, int p_width);
	int get_tab_icon_max_width(int p_tab) const;

	void set_tab_disabled(int p_tab, bool p_disabled);
	bool is_tab_disabled(int p_tab) const;

	void set_tab_hidden(int p_tab, bool p_hidden);
	bool is_tab_hidden(int p_tab) const;

	void set_tab_metadata(int p_tab, const Variant &p_metadata);
	Variant get_tab_metadata(int p_tab) const;

	void set_new_tab_enabled(bool p_enabled);
	bool get_new_tab_enabled() const;

	int get_tab_count() const;
	void set_current_tab(int p_current);
	int get_current_tab() const;
	int get_previous_tab() const;

	bool select_previous_available();
	bool select_next_available();

	void set_deselect_enabled(bool p_enabled);
	bool get_deselect_enabled() const;

	Control *get_tab_control(int p_idx) const;
	Control *get_current_tab_control() const;

	virtual Size2 get_minimum_size() const override;

	void set_popup(Node *p_popup);
	Popup *get_popup() const;

	void trigger_menu_option(int p_option, bool p_confirmed);

	void move_tab_from_tab_container(AppTabContainer *p_from, int p_from_index, int p_to_index = -1);
	void move_tab(const Variant &p_data, int p_to_index = -1);

	void set_drag_to_rearrange_enabled(bool p_enabled);
	bool get_drag_to_rearrange_enabled() const;
	void set_tabs_rearrange_group(int p_group_id);
	int get_tabs_rearrange_group() const;
	void set_use_hidden_tabs_for_min_size(bool p_use_hidden_tabs);
	bool get_use_hidden_tabs_for_min_size() const;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	AppTabContainer();
	~AppTabContainer();
};

VARIANT_ENUM_CAST(AppTabContainer::TabPosition);
