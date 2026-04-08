#pragma once

#include "scene/gui/box_container.h"

class Button;
class LineEdit;
class PopupMenu;

class AddressBar : public HBoxContainer {
	GDCLASS(AddressBar, HBoxContainer);

private:
	bool disable_shortcuts = false;
	LineEdit *line_edit = nullptr;
	Button *popup_button = nullptr;
	PopupMenu *popup = nullptr;

	void _on_text_submitted(const String &p_path);
	void _on_editing_toggled(bool p_toggled_on);

	void _on_button_pressed();

	void _on_id_pressed(int p_id);

protected:
	void _notification(int p_what);
	static void _bind_methods();

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	void edit(bool p_hide_focus = false);
	void unedit();
	bool is_editing() const;

	void set_text(String p_text);
	String get_text() const;

	void add_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id = -1);
	void add_item(const String &p_label, int p_id = -1);

	void set_focused_item(int p_idx);
	int get_focused_item() const;

	int get_item_count() const;

	void remove_item(int p_idx);
	void clear_items();

	void clear();

	PopupMenu *get_popup() const;
	void show_popup();

	AddressBar();
	virtual ~AddressBar();
};
