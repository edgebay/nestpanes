#pragma once

#include "app/gui/app_control.h"

#include "scene/gui/code_edit.h"
#include "scene/resources/text_file.h"

class TextEditor : public AppControl {
	GDCLASS(TextEditor, AppControl);

	// CodeEdit *code_edit = nullptr;
	CodeEdit *text_editor = nullptr;

	float zoom_factor = 1.0f;

	void _complete_request();

	virtual void input(const Ref<InputEvent> &event) override;
	void _text_editor_gui_input(const Ref<InputEvent> &p_event);

	void _zoom_in();
	void _zoom_out();
	void _zoom_to(float p_zoom_factor);

	HashSet<String> textfile_extensions;
	Ref<TextFile> _load_text_file(const String &p_path, Error *r_error) const;
	Error _save_text_file(Ref<TextFile> p_text_file, const String &p_path);

protected:
	void _text_changed_idle_timeout();
	void _code_complete_timer_timeout();
	void _text_changed();
	void _line_col_changed();

	void _notification(int);
	static void _bind_methods();

public:
	CodeEdit *get_text_editor() { return text_editor; }

	void set_zoom_factor(float p_zoom_factor);
	float get_zoom_factor();

	Ref<Resource> open_file(const String &p_file);

	_FORCE_INLINE_ bool edit(const Ref<Resource> &p_resource, bool p_grab_focus = true) { return edit(p_resource, -1, 0, p_grab_focus); }
	bool edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus = true);

	TextEditor();
};
