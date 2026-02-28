#pragma once

#include "scene/gui/line_edit.h"

class PopupMenu;

class AddressBar : public LineEdit {
	GDCLASS(AddressBar, LineEdit);

private:
	PopupMenu *popup = nullptr;

	void _selected(int p_which);
	void _select(int p_which, bool p_emit = false);

	void _on_editing_toggled(bool p_toggled_on);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	int get_item_count() const;
	void add_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id = -1);
	void add_item(const String &p_label, int p_id = -1);
	void remove_item(int p_idx);

	void clear();

	PopupMenu *get_popup() const;
	void show_popup();

	AddressBar();
	virtual ~AddressBar();
};
