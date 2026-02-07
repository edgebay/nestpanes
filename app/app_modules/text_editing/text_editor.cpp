#include "text_editor.h"

#include "scene/gui/box_container.h"

#include "app/settings/app_settings.h"
#include "app/themes/app_scale.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"

/*** CODE EDITOR ****/

static constexpr float ZOOM_FACTOR_PRESETS[8] = { 0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f };

// This function should be used to handle shortcuts that could otherwise
// be handled too late if they weren't handled here.
void TextEditor::input(const Ref<InputEvent> &event) {
	ERR_FAIL_COND(event.is_null());

	const Ref<InputEventKey> key_event = event;

	if (key_event.is_null()) {
		return;
	}
	if (!key_event->is_pressed()) {
		return;
	}

	// if (!text_editor->has_focus()) {
	// 	if ((find_replace_bar != nullptr && find_replace_bar->is_visible()) && (find_replace_bar->has_focus() || (get_viewport()->gui_get_focus_owner() && find_replace_bar->is_ancestor_of(get_viewport()->gui_get_focus_owner())))) {
	// 		if (ED_IS_SHORTCUT("script_text_editor/find_next", key_event)) {
	// 			find_replace_bar->search_next();
	// 			accept_event();
	// 			return;
	// 		}
	// 		if (ED_IS_SHORTCUT("script_text_editor/find_previous", key_event)) {
	// 			find_replace_bar->search_prev();
	// 			accept_event();
	// 			return;
	// 		}
	// 	}
	// 	return;
	// }

	// if (ED_IS_SHORTCUT("script_text_editor/move_up", key_event)) {
	// 	text_editor->move_lines_up();
	// 	accept_event();
	// 	return;
	// }
	// if (ED_IS_SHORTCUT("script_text_editor/move_down", key_event)) {
	// 	text_editor->move_lines_down();
	// 	accept_event();
	// 	return;
	// }
	// if (ED_IS_SHORTCUT("script_text_editor/delete_line", key_event)) {
	// 	text_editor->delete_lines();
	// 	accept_event();
	// 	return;
	// }
	// if (ED_IS_SHORTCUT("script_text_editor/duplicate_selection", key_event)) {
	// 	text_editor->duplicate_selection();
	// 	accept_event();
	// 	return;
	// }
	// if (ED_IS_SHORTCUT("script_text_editor/duplicate_lines", key_event)) {
	// 	text_editor->duplicate_lines();
	// 	accept_event();
	// 	return;
	// }
}

void TextEditor::_text_editor_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		if (mb->is_pressed() && mb->is_command_or_control_pressed()) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP) {
				_zoom_in();
				accept_event();
				return;
			}
			if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				_zoom_out();
				accept_event();
				return;
			}
		}
	}

	Ref<InputEventMagnifyGesture> magnify_gesture = p_event;
	if (magnify_gesture.is_valid()) {
		_zoom_to(zoom_factor * std::pow(magnify_gesture->get_factor(), 0.25f));
		accept_event();
		return;
	}

	// Ref<InputEventKey> k = p_event;

	// if (k.is_valid()) {
	// 	if (k->is_pressed()) {
	// 		if (ED_IS_SHORTCUT("script_editor/zoom_in", p_event)) {
	// 			_zoom_in();
	// 			accept_event();
	// 			return;
	// 		}
	// 		if (ED_IS_SHORTCUT("script_editor/zoom_out", p_event)) {
	// 			_zoom_out();
	// 			accept_event();
	// 			return;
	// 		}
	// 		if (ED_IS_SHORTCUT("script_editor/reset_zoom", p_event)) {
	// 			_zoom_to(1);
	// 			accept_event();
	// 			return;
	// 		}
	// 	}
	// }
}

void TextEditor::_line_col_changed() {
	// if (!code_complete_timer->is_stopped() && code_complete_timer_line != text_editor->get_caret_line()) {
	// 	code_complete_timer->stop();
	// }

	// String line = text_editor->get_line(text_editor->get_caret_line());

	// int positional_column = 0;
	// for (int i = 0; i < text_editor->get_caret_column(); i++) {
	// 	if (line[i] == '\t') {
	// 		positional_column += text_editor->get_indent_size(); // Tab size
	// 	} else {
	// 		positional_column += 1;
	// 	}
	// }

	// StringBuilder sb;
	// sb.append(itos(text_editor->get_caret_line() + 1).lpad(4));
	// sb.append(" : ");
	// sb.append(itos(positional_column + 1).lpad(3));

	// line_and_col_txt->set_text(sb.as_string());

	// if (find_replace_bar) {
	// 	if (!find_replace_bar->line_col_changed_for_result) {
	// 		find_replace_bar->needs_to_count_results = true;
	// 	}

	// 	find_replace_bar->line_col_changed_for_result = false;
	// }
}

void TextEditor::_text_changed() {
	// if (code_complete_enabled && text_editor->is_insert_text_operation()) {
	// 	code_complete_timer_line = text_editor->get_caret_line();
	// 	code_complete_timer->start();
	// }

	// idle->start();

	// if (find_replace_bar) {
	// 	find_replace_bar->needs_to_count_results = true;
	// }
}

void TextEditor::_code_complete_timer_timeout() {
	if (!is_visible_in_tree()) {
		return;
	}
	text_editor->request_code_completion();
}

void TextEditor::_complete_request() {
	// List<ScriptLanguage::CodeCompletionOption> entries;
	// String ctext = text_editor->get_text_for_code_completion();
	// _code_complete_script(ctext, &entries);
	// bool forced = false;
	// if (code_complete_func) {
	// 	code_complete_func(code_complete_ud, ctext, &entries, forced);
	// }

	// for (const ScriptLanguage::CodeCompletionOption &e : entries) {
	// 	Color font_color = completion_font_color;
	// 	if (!e.theme_color_name.is_empty() && APP_GET("text_editor/completion/colorize_suggestions")) {
	// 		font_color = get_theme_color(e.theme_color_name, SNAME("Editor"));
	// 	} else if (e.insert_text.begins_with("\"") || e.insert_text.begins_with("\'")) {
	// 		font_color = completion_string_color;
	// 	} else if (e.insert_text.begins_with("##") || e.insert_text.begins_with("///")) {
	// 		font_color = completion_doc_comment_color;
	// 	} else if (e.insert_text.begins_with("&")) {
	// 		font_color = completion_string_name_color;
	// 	} else if (e.insert_text.begins_with("^")) {
	// 		font_color = completion_node_path_color;
	// 	} else if (e.insert_text.begins_with("#") || e.insert_text.begins_with("//")) {
	// 		font_color = completion_comment_color;
	// 	}
	// 	text_editor->add_code_completion_option((CodeEdit::CodeCompletionKind)e.kind, e.display, e.insert_text, font_color, _get_completion_icon(e), e.default_value, e.location);
	// }
	// text_editor->update_code_completion_options(forced);
}

void TextEditor::_zoom_in() {
	int s = text_editor->get_theme_font_size(SceneStringName(font_size));
	_zoom_to(zoom_factor * (s + MAX(1.0f, APP_SCALE)) / s);
}

void TextEditor::_zoom_out() {
	int s = text_editor->get_theme_font_size(SceneStringName(font_size));
	_zoom_to(zoom_factor * (s - MAX(1.0f, APP_SCALE)) / s);
}

void TextEditor::_zoom_to(float p_zoom_factor) {
	if (zoom_factor == p_zoom_factor) {
		return;
	}

	float old_zoom_factor = zoom_factor;

	set_zoom_factor(p_zoom_factor);

	if (old_zoom_factor != zoom_factor) {
		emit_signal(SNAME("zoomed"), zoom_factor);
	}
}

Ref<TextFile> TextEditor::_load_text_file(const String &p_path, Error *r_error) const {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	String local_path = p_path;
	String path = ResourceLoader::path_remap(local_path);

	TextFile *text_file = memnew(TextFile);
	Ref<TextFile> text_res(text_file);
	Error err = text_file->load_text(path);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load text file '" + path + "'.");

	text_file->set_file_path(local_path);
	text_file->set_path(local_path, true);

	// if (ResourceLoader::get_timestamp_on_load()) {
	// 	text_file->set_last_modified_time(FileAccess::get_modified_time(path));
	// }

	if (r_error) {
		*r_error = OK;
	}

	return text_res;
}

Error TextEditor::_save_text_file(Ref<TextFile> p_text_file, const String &p_path) {
	Ref<TextFile> sqscr = p_text_file;
	ERR_FAIL_COND_V(sqscr.is_null(), ERR_INVALID_PARAMETER);

	String source = sqscr->get_text();

	Error err;
	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

		ERR_FAIL_COND_V_MSG(err, err, "Cannot save text file '" + p_path + "'.");

		file->store_string(source);
		if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
			return ERR_CANT_CREATE;
		}
	}

	// if (ResourceSaver::get_timestamp_on_save()) {
	// 	p_text_file->set_last_modified_time(FileAccess::get_modified_time(p_path));
	// }

	// EditorFileSystem::get_singleton()->update_file(p_path);

	// _res_saved_callback(sqscr);
	return OK;
}

void TextEditor::set_zoom_factor(float p_zoom_factor) {
	zoom_factor = CLAMP(p_zoom_factor, 0.25f, 3.0f);
	int neutral_font_size = int(APP_GET("interface/app/code_font_size")) * APP_SCALE;
	int new_font_size = Math::round(zoom_factor * neutral_font_size);

	// zoom_button->set_text(itos(Math::round(zoom_factor * 100)) + " %");

	text_editor->add_theme_font_size_override(SceneStringName(font_size), new_font_size);
}

float TextEditor::get_zoom_factor() {
	return zoom_factor;
}

Ref<Resource> TextEditor::open_file(const String &p_file) {
	Error error;
	Ref<TextFile> text_file = _load_text_file(p_file, &error);
	if (error != OK) {
		// EditorNode::get_singleton()->show_warning(TTR("Could not load file at:") + "\n\n" + p_file, TTR("Error!"));
		return Ref<Resource>();
	}

	if (text_file.is_valid()) {
		edit(text_file);
		return text_file;
	}
	return Ref<Resource>();
}

bool TextEditor::edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus) {
	Ref<TextFile> text_file = p_resource;
	if (text_file.is_valid()) {
		text_editor->set_text(text_file->get_text());
		return true;
	}
	return false;
}

void TextEditor::_notification(int) {
}

void TextEditor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("validate_script"));
	ADD_SIGNAL(MethodInfo("load_theme_settings"));
	ADD_SIGNAL(MethodInfo("show_errors_panel"));
	ADD_SIGNAL(MethodInfo("show_warnings_panel"));
	ADD_SIGNAL(MethodInfo("navigation_preview_ended"));
	ADD_SIGNAL(MethodInfo("zoomed", PropertyInfo(Variant::FLOAT, "p_zoom_factor")));
}

TextEditor::TextEditor() {
	VBoxContainer *vbox = memnew(VBoxContainer);
	add_child(vbox);
	vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	text_editor = memnew(CodeEdit);
	// add_child(text_editor);
	vbox->add_child(text_editor);
	text_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	text_editor->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_GDSCRIPT);
	text_editor->set_draw_bookmarks_gutter(true);

	text_editor->set_virtual_keyboard_show_on_focus(false);
	text_editor->set_draw_line_numbers(true);
	text_editor->set_highlight_matching_braces_enabled(true);
	text_editor->set_auto_indent_enabled(true);
	text_editor->set_deselect_on_focus_loss_enabled(false);

	text_editor->connect(SceneStringName(gui_input), callable_mp(this, &TextEditor::_text_editor_gui_input));
	text_editor->connect("caret_changed", callable_mp(this, &TextEditor::_line_col_changed));
	text_editor->connect(SceneStringName(text_changed), callable_mp(this, &TextEditor::_text_changed));
	text_editor->connect("code_completion_requested", callable_mp(this, &TextEditor::_complete_request));
	TypedArray<String> cs = { ".", ",", "(", "=", "$", "@", "\"", "\'" };
	text_editor->set_code_completion_prefixes(cs);
}