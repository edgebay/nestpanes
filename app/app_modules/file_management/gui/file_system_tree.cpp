#include "file_system_tree.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/math/math_funcs.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/slider.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"

#include <climits>

#include "app/app_modules/file_management/gui/file_context_menu.h"

#include "app/app_string_names.h"
#include "app/gui/app_control.h"
#include "app/themes/app_scale.h"

#include "app/app_core/io/file_system_access.h"
#include "app/app_modules/file_management/file_system.h"

#define DRAG_THRESHOLD (8 * APP_SCALE)

static const int default_column_count = 5;
static FileSystemTree::ColumnSetting default_column_settings[default_column_count] = {
	{ FileSystemTree::COLUMN_TYPE_NAME, 0, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 3, false },
	{ FileSystemTree::COLUMN_TYPE_MODIFIED, 1, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1, false },
	{ FileSystemTree::COLUMN_TYPE_CREATED, 2, false, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1, false },
	{ FileSystemTree::COLUMN_TYPE_TYPE, 3, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1, false },
	{ FileSystemTree::COLUMN_TYPE_SIZE, 4, true, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1, false },
};

Size2 FileSystemTreeItem::Cell::get_icon_size() const {
	if (icon.is_null()) {
		return Size2();
	}
	if (icon_region == Rect2i()) {
		return icon->get_size();
	} else {
		return icon_region.size;
	}
}

void FileSystemTreeItem::Cell::draw_icon(const RID &p_where, const Point2 &p_pos, const Size2 &p_size, const Rect2i &p_region, const Color &p_color) const {
	if (icon.is_null()) {
		return;
	}

	Size2i dsize = (p_size == Size2()) ? icon->get_size() : p_size;

	if (p_region == Rect2i()) {
		icon->draw_rect_region(p_where, Rect2(p_pos, dsize), Rect2(Point2(), icon->get_size()), p_color);
		if (icon_overlay.is_valid()) {
			Vector2 offset = icon->get_size() - icon_overlay->get_size();
			icon_overlay->draw_rect_region(p_where, Rect2(p_pos + offset, dsize), Rect2(Point2(), icon_overlay->get_size()), p_color);
		}
	} else {
		icon->draw_rect_region(p_where, Rect2(p_pos, dsize), p_region, p_color);
		if (icon_overlay.is_valid()) {
			icon_overlay->draw_rect_region(p_where, Rect2(p_pos, dsize), p_region, p_color);
		}
	}
}

void FileSystemTreeItem::_changed_notify(int p_cell) {
	if (tree) {
		tree->item_changed(p_cell, this);
	}
}

void FileSystemTreeItem::_changed_notify() {
	if (tree) {
		tree->item_changed(-1, this);
	}
}

void FileSystemTreeItem::_change_tree(FileSystemTree *p_tree) {
	if (p_tree == tree) {
		return;
	}

	FileSystemTreeItem *c = first_child;
	while (c) {
		c->_change_tree(p_tree);
		c = c->next;
	}

	if (tree) {
		if (tree->root == this) {
			tree->root = nullptr;
		}

		if (tree->popup_edited_item == this) {
			tree->popup_edited_item = nullptr;
			tree->popup_pressing_edited_item = nullptr;
			tree->pressing_for_editor = false;
		}

		if (tree->cache.hover_item == this) {
			tree->cache.hover_item = nullptr;
		}

		if (tree->selected_item == this) {
			tree->selected_item->selected = false;
			tree->selected_item = nullptr;
		}

		if (tree->drop_mode_over == this) {
			tree->_reset_drop_mode_over();
		}

		if (tree->single_select_defer == this) {
			tree->single_select_defer = nullptr;
		}

		if (tree->edited_item == this) {
			tree->edited_item = nullptr;
			tree->pressing_for_editor = false;
		}
		tree->update_min_size_for_item_change();
		tree->queue_redraw();
	}

	tree = p_tree;

	if (tree) {
		tree->queue_redraw();
		cells.resize(tree->columns.size());
	}
}

void FileSystemTreeItem::set_cell_mode(int p_column, TreeCellMode p_mode) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].mode == p_mode) {
		return;
	}

	Cell &c = cells.write[p_column];
	c.mode = p_mode;
	c.checked = false;
	c.icon = Ref<Texture2D>();
	c.text = "";
	c.dirty = true;
	c.icon_max_w = 0;
	c.cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

FileSystemTreeItem::TreeCellMode FileSystemTreeItem::get_cell_mode(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), FileSystemTreeItem::CELL_MODE_STRING);
	return cells[p_column].mode;
}

void FileSystemTreeItem::set_auto_translate_mode(int p_column, Node::AutoTranslateMode p_mode) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].auto_translate_mode == p_mode) {
		return;
	}

	cells.write[p_column].auto_translate_mode = p_mode;
	cells.write[p_column].dirty = true;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Node::AutoTranslateMode FileSystemTreeItem::get_auto_translate_mode(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Node::AUTO_TRANSLATE_MODE_INHERIT);
	return cells[p_column].auto_translate_mode;
}

void FileSystemTreeItem::set_edit_multiline(int p_column, bool p_multiline) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].edit_multiline = p_multiline;
	_changed_notify(p_column);
}

bool FileSystemTreeItem::is_edit_multiline(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].edit_multiline;
}

void FileSystemTreeItem::set_checked(int p_column, bool p_checked) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].checked == p_checked) {
		return;
	}

	cells.write[p_column].checked = p_checked;
	cells.write[p_column].indeterminate = false;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

void FileSystemTreeItem::set_indeterminate(int p_column, bool p_indeterminate) {
	ERR_FAIL_INDEX(p_column, cells.size());

	// Prevent uncheck if indeterminate set to false twice.
	if (p_indeterminate == cells[p_column].indeterminate) {
		return;
	}

	cells.write[p_column].indeterminate = p_indeterminate;
	cells.write[p_column].checked = false;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool FileSystemTreeItem::is_checked(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].checked;
}

bool FileSystemTreeItem::is_indeterminate(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].indeterminate;
}

void FileSystemTreeItem::propagate_check(int p_column, bool p_emit_signal) {
	ERR_FAIL_INDEX(p_column, cells.size());

	bool ch = cells[p_column].checked;

	if (p_emit_signal) {
		tree->emit_signal(SNAME("check_propagated_to_item"), this, p_column);
	}
	_propagate_check_through_children(p_column, ch, p_emit_signal);
	_propagate_check_through_parents(p_column, p_emit_signal);
}

String FileSystemTreeItem::atr(int p_column, const String &p_text) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), tree->atr(p_text));

	switch (cells[p_column].auto_translate_mode) {
		case Node::AUTO_TRANSLATE_MODE_INHERIT: {
			return tree->atr(p_text);
		} break;
		case Node::AUTO_TRANSLATE_MODE_ALWAYS: {
			return tree->tr(p_text);
		} break;
		case Node::AUTO_TRANSLATE_MODE_DISABLED: {
			return p_text;
		} break;
	}

	ERR_FAIL_V_MSG(tree->atr(p_text), "Unexpected auto translate mode: " + itos(cells[p_column].auto_translate_mode));
}

void FileSystemTreeItem::_propagate_check_through_children(int p_column, bool p_checked, bool p_emit_signal) {
	FileSystemTreeItem *current = get_first_child();
	while (current) {
		current->set_checked(p_column, p_checked);
		if (p_emit_signal) {
			current->tree->emit_signal(SNAME("check_propagated_to_item"), current, p_column);
		}
		current->_propagate_check_through_children(p_column, p_checked, p_emit_signal);
		current = current->get_next();
	}
}

void FileSystemTreeItem::_propagate_check_through_parents(int p_column, bool p_emit_signal) {
	FileSystemTreeItem *current = get_parent();
	if (!current) {
		return;
	}

	bool any_checked = false;
	bool any_unchecked = false;
	bool any_indeterminate = false;

	FileSystemTreeItem *child_item = current->get_first_child();
	while (child_item) {
		if (!child_item->is_checked(p_column)) {
			any_unchecked = true;
			if (child_item->is_indeterminate(p_column)) {
				any_indeterminate = true;
				break;
			}
		} else {
			any_checked = true;
		}
		child_item = child_item->get_next();
	}

	if (any_indeterminate || (any_checked && any_unchecked)) {
		current->set_indeterminate(p_column, true);
	} else if (current->is_indeterminate(p_column) && !any_checked) {
		current->set_indeterminate(p_column, false);
	} else {
		current->set_checked(p_column, any_checked);
	}

	if (p_emit_signal) {
		current->tree->emit_signal(SNAME("check_propagated_to_item"), current, p_column);
	}
	current->_propagate_check_through_parents(p_column, p_emit_signal);
}

void FileSystemTreeItem::set_text(int p_column, String p_text) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].text == p_text) {
		return;
	}

	cells.write[p_column].text = p_text;
	cells.write[p_column].dirty = true;

	// Don't auto translate if it's in string mode and editable, as the text can be changed to anything by the user.
	if (tree && (!cells[p_column].editable || cells[p_column].mode != FileSystemTreeItem::CELL_MODE_STRING)) {
		cells.write[p_column].xl_text = atr(p_column, p_text);
	} else {
		cells.write[p_column].xl_text = p_text;
	}

	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
	if (get_tree()) {
		get_tree()->update_configuration_warnings();
	}
}

String FileSystemTreeItem::get_text(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].text;
}

void FileSystemTreeItem::set_description(int p_column, String p_text) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].description == p_text) {
		return;
	}

	cells.write[p_column].description = p_text;

	_changed_notify(p_column);
	if (get_tree()) {
		get_tree()->update_configuration_warnings();
	}
}

String FileSystemTreeItem::get_description(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].description;
}

void FileSystemTreeItem::set_text_direction(int p_column, Control::TextDirection p_text_direction) {
	ERR_FAIL_INDEX(p_column, cells.size());
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);

	if (cells[p_column].text_direction == p_text_direction) {
		return;
	}

	cells.write[p_column].text_direction = p_text_direction;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
	cells.write[p_column].cached_minimum_size_dirty = true;
}

Control::TextDirection FileSystemTreeItem::get_text_direction(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Control::TEXT_DIRECTION_INHERITED);
	return cells[p_column].text_direction;
}

void FileSystemTreeItem::set_autowrap_mode(int p_column, TextServer::AutowrapMode p_mode) {
	ERR_FAIL_INDEX(p_column, cells.size());
	ERR_FAIL_COND(p_mode < TextServer::AUTOWRAP_OFF || p_mode > TextServer::AUTOWRAP_WORD_SMART);

	if (cells[p_column].autowrap_mode == p_mode) {
		return;
	}

	cells.write[p_column].autowrap_mode = p_mode;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
	cells.write[p_column].cached_minimum_size_dirty = true;
}

TextServer::AutowrapMode FileSystemTreeItem::get_autowrap_mode(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::AUTOWRAP_OFF);
	return cells[p_column].autowrap_mode;
}

void FileSystemTreeItem::set_text_overrun_behavior(int p_column, TextServer::OverrunBehavior p_behavior) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].text_buf->get_text_overrun_behavior() == p_behavior) {
		return;
	}

	cells.write[p_column].text_buf->set_text_overrun_behavior(p_behavior);
	cells.write[p_column].dirty = true;
	cells.write[p_column].cached_minimum_size_dirty = true;
	_changed_notify(p_column);
}

TextServer::OverrunBehavior FileSystemTreeItem::get_text_overrun_behavior(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::OVERRUN_TRIM_ELLIPSIS);
	return cells[p_column].text_buf->get_text_overrun_behavior();
}

void FileSystemTreeItem::set_structured_text_bidi_override(int p_column, TextServer::StructuredTextParser p_parser) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].st_parser != p_parser) {
		cells.write[p_column].st_parser = p_parser;
		cells.write[p_column].dirty = true;
		cells.write[p_column].cached_minimum_size_dirty = true;

		_changed_notify(p_column);
	}
}

TextServer::StructuredTextParser FileSystemTreeItem::get_structured_text_bidi_override(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::STRUCTURED_TEXT_DEFAULT);
	return cells[p_column].st_parser;
}

void FileSystemTreeItem::set_structured_text_bidi_override_options(int p_column, const Array &p_args) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].st_args == p_args) {
		return;
	}

	cells.write[p_column].st_args = Array(p_args);
	cells.write[p_column].dirty = true;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Array FileSystemTreeItem::get_structured_text_bidi_override_options(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Array());
	return Array(cells[p_column].st_args);
}

void FileSystemTreeItem::set_language(int p_column, const String &p_language) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].language != p_language) {
		cells.write[p_column].language = p_language;
		cells.write[p_column].dirty = true;
		cells.write[p_column].cached_minimum_size_dirty = true;

		_changed_notify(p_column);
	}
}

String FileSystemTreeItem::get_language(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].language;
}

void FileSystemTreeItem::set_suffix(int p_column, String p_suffix) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].suffix == p_suffix) {
		return;
	}

	cells.write[p_column].suffix = p_suffix;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

String FileSystemTreeItem::get_suffix(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].suffix;
}

void FileSystemTreeItem::set_icon(int p_column, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon == p_icon) {
		return;
	}

	cells.write[p_column].icon = p_icon;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Texture2D> FileSystemTreeItem::get_icon(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Texture2D>());
	return cells[p_column].icon;
}

void FileSystemTreeItem::set_icon_overlay(int p_column, const Ref<Texture2D> &p_icon_overlay) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_overlay == p_icon_overlay) {
		return;
	}

	cells.write[p_column].icon_overlay = p_icon_overlay;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Texture2D> FileSystemTreeItem::get_icon_overlay(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Texture2D>());
	return cells[p_column].icon_overlay;
}

void FileSystemTreeItem::set_icon_region(int p_column, const Rect2 &p_icon_region) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_region == p_icon_region) {
		return;
	}

	cells.write[p_column].icon_region = p_icon_region;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Rect2 FileSystemTreeItem::get_icon_region(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Rect2());
	return cells[p_column].icon_region;
}

void FileSystemTreeItem::set_icon_modulate(int p_column, const Color &p_modulate) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_color == p_modulate) {
		return;
	}

	cells.write[p_column].icon_color = p_modulate;
	_changed_notify(p_column);
}

Color FileSystemTreeItem::get_icon_modulate(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	return cells[p_column].icon_color;
}

void FileSystemTreeItem::set_icon_max_width(int p_column, int p_max) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_max_w == p_max) {
		return;
	}

	cells.write[p_column].icon_max_w = p_max;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

int FileSystemTreeItem::get_icon_max_width(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), 0);
	return cells[p_column].icon_max_w;
}

void FileSystemTreeItem::set_metadata(int p_column, const Variant &p_meta) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].meta = p_meta;
}

Variant FileSystemTreeItem::get_metadata(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Variant());

	return cells[p_column].meta;
}

void FileSystemTreeItem::set_collapsed(bool p_collapsed) {
	if (collapsed == p_collapsed || !tree) {
		return;
	}
	collapsed = p_collapsed;
	FileSystemTreeItem *ci = tree->selected_item;
	if (ci) {
		while (ci && ci != this) {
			ci = ci->parent;
		}
		if (ci) { // Collapsing cursor/selected, move it!
			tree->selected_item = this;
			// TODO
			// emit_signal(SNAME("item_selected"), selected_item, true);

			tree->queue_redraw();
		}
	}

	_changed_notify();
	tree->emit_signal(SNAME("item_collapsed"), this);
}

bool FileSystemTreeItem::is_collapsed() {
	return collapsed;
}

void FileSystemTreeItem::set_collapsed_recursive(bool p_collapsed) {
	if (!tree) {
		return;
	}

	set_collapsed(p_collapsed);

	FileSystemTreeItem *child = get_first_child();
	while (child) {
		child->set_collapsed_recursive(p_collapsed);
		child = child->get_next();
	}
}

bool FileSystemTreeItem::_is_any_collapsed(bool p_only_visible) {
	FileSystemTreeItem *child = get_first_child();

	// Check on children directly first (avoid recursing if possible).
	while (child) {
		if (child->get_first_child() && child->is_collapsed() && (!p_only_visible || (child->is_visible() && child->get_visible_child_count()))) {
			return true;
		}
		child = child->get_next();
	}

	child = get_first_child();

	// Otherwise recurse on children.
	while (child) {
		if (child->get_first_child() && (!p_only_visible || (child->is_visible() && child->get_visible_child_count())) && child->_is_any_collapsed(p_only_visible)) {
			return true;
		}
		child = child->get_next();
	}

	return false;
}

bool FileSystemTreeItem::is_any_collapsed(bool p_only_visible) {
	if (p_only_visible && !is_visible_in_tree()) {
		return false;
	}

	// Collapsed if this is collapsed and it has children (only considers visible if only visible is set).
	if (is_collapsed() && get_first_child() && (!p_only_visible || get_visible_child_count())) {
		return true;
	}

	return _is_any_collapsed(p_only_visible);
}

void FileSystemTreeItem::set_visible(bool p_visible) {
	if (visible == p_visible) {
		return;
	}
	visible = p_visible;
	if (tree) {
		if (!visible) {
			if (selected) {
				deselect();
			}
		}

		tree->queue_redraw();
		_changed_notify();
	}

	_handle_visibility_changed(p_visible);
}

bool FileSystemTreeItem::is_visible() {
	return visible;
}

bool FileSystemTreeItem::is_visible_in_tree() const {
	return visible && parent_visible_in_tree;
}

void FileSystemTreeItem::_handle_visibility_changed(bool p_visible) {
	FileSystemTreeItem *child = get_first_child();
	while (child) {
		child->_propagate_visibility_changed(p_visible);
		child = child->get_next();
	}
}

void FileSystemTreeItem::_propagate_visibility_changed(bool p_parent_visible_in_tree) {
	parent_visible_in_tree = p_parent_visible_in_tree;
	_handle_visibility_changed(p_parent_visible_in_tree);
}

void FileSystemTreeItem::uncollapse_tree() {
	FileSystemTreeItem *t = this;
	while (t) {
		t->set_collapsed(false);
		t = t->parent;
	}
}

void FileSystemTreeItem::set_custom_minimum_height(int p_height) {
	if (custom_min_height == p_height) {
		return;
	}

	custom_min_height = p_height;

	for (Cell &c : cells) {
		c.cached_minimum_size_dirty = true;
	}

	_changed_notify();
}

int FileSystemTreeItem::get_custom_minimum_height() const {
	return custom_min_height;
}

void FileSystemTreeItem::set_custom_stylebox(int p_column, const Ref<StyleBox> &p_stylebox) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_stylebox = p_stylebox;
	_changed_notify(p_column);
}

Ref<StyleBox> FileSystemTreeItem::get_custom_stylebox(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<StyleBox>());
	return cells[p_column].custom_stylebox;
}

FileSystemTreeItem *FileSystemTreeItem::create_child(int p_index) {
	FileSystemTreeItem *ti = memnew(FileSystemTreeItem(tree));
	if (tree) {
		ti->cells.resize(tree->columns.size());
		tree->update_min_size_for_item_change();
		tree->queue_redraw();
	}

	FileSystemTreeItem *item_prev = nullptr;
	FileSystemTreeItem *item_next = first_child;

	if (p_index < 0 && last_child) {
		item_prev = last_child;
	} else {
		int idx = 0;
		if (!children_cache.is_empty()) {
			idx = MIN(int(children_cache.size()) - 1, p_index);
			item_next = children_cache[idx];
			item_prev = item_next->prev;
		}
		while (item_next) {
			if (idx == p_index) {
				item_next->prev = ti;
				ti->next = item_next;
				break;
			}

			item_prev = item_next;
			item_next = item_next->next;
			idx++;
		}
	}

	if (item_prev) {
		item_prev->next = ti;
		ti->prev = item_prev;

		if (!children_cache.is_empty()) {
			if (ti->next) {
				children_cache.insert(p_index, ti);
			} else {
				children_cache.push_back(ti);
			}
		}
	} else {
		first_child = ti;
		children_cache.insert(0, ti);
	}

	if (item_prev == last_child) {
		last_child = ti;
	}

	ti->parent = this;
	ti->parent_visible_in_tree = is_visible_in_tree();

	return ti;
}

void FileSystemTreeItem::add_child(FileSystemTreeItem *p_item) {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(p_item->tree);
	ERR_FAIL_COND(p_item->parent);

	p_item->_change_tree(tree);
	p_item->parent = this;
	p_item->parent_visible_in_tree = is_visible_in_tree();
	p_item->_handle_visibility_changed(p_item->parent_visible_in_tree);

	if (last_child) {
		last_child->next = p_item;
		p_item->prev = last_child;
	} else {
		first_child = p_item;
	}
	last_child = p_item;

	if (!children_cache.is_empty()) {
		children_cache.push_back(p_item);
	}

	validate_cache();
}

void FileSystemTreeItem::remove_child(FileSystemTreeItem *p_item) {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(p_item->parent != this);

	p_item->_unlink_from_tree();
	p_item->_change_tree(nullptr);
	p_item->prev = nullptr;
	p_item->next = nullptr;
	p_item->parent = nullptr;

	validate_cache();
}

FileSystemTree *FileSystemTreeItem::get_tree() const {
	return tree;
}

FileSystemTreeItem *FileSystemTreeItem::get_next() const {
	return next;
}

FileSystemTreeItem *FileSystemTreeItem::get_prev() {
	if (prev) {
		return prev;
	}

	if (!parent || parent->first_child == this) {
		return nullptr;
	}
	// This is an edge case.
	FileSystemTreeItem *l_prev = parent->first_child;
	while (l_prev && l_prev->next != this) {
		l_prev = l_prev->next;
	}

	prev = l_prev;

	return prev;
}

FileSystemTreeItem *FileSystemTreeItem::get_parent() const {
	return parent;
}

FileSystemTreeItem *FileSystemTreeItem::get_first_child() const {
	return first_child;
}

FileSystemTreeItem *FileSystemTreeItem::_get_prev_in_tree(bool p_wrap, bool p_include_invisible) {
	FileSystemTreeItem *current = this;

	FileSystemTreeItem *prev_item = current->get_prev();

	if (!prev_item) {
		current = current->parent;
		if (!current || (current == tree->root && tree->hide_root)) {
			if (!p_wrap) {
				return nullptr;
			}
			// Wrap around to the last visible item.
			current = this;
			FileSystemTreeItem *temp = get_next_visible();
			while (temp) {
				current = temp;
				temp = temp->get_next_visible();
			}
		}
	} else {
		current = prev_item;
		while ((!current->collapsed || p_include_invisible) && current->last_child) {
			current = current->last_child;
		}
	}

	return current;
}

FileSystemTreeItem *FileSystemTreeItem::get_prev_visible(bool p_wrap) {
	FileSystemTreeItem *loop = this;
	FileSystemTreeItem *prev_item = _get_prev_in_tree(p_wrap);
	while (prev_item && !prev_item->is_visible_in_tree()) {
		prev_item = prev_item->_get_prev_in_tree(p_wrap);
		if (prev_item == loop) {
			// Check that we haven't looped all the way around to the start.
			prev_item = nullptr;
			break;
		}
	}
	return prev_item;
}

FileSystemTreeItem *FileSystemTreeItem::_get_next_in_tree(bool p_wrap, bool p_include_invisible) {
	FileSystemTreeItem *current = this;

	if ((!current->collapsed || p_include_invisible) && current->first_child) {
		current = current->first_child;

	} else if (current->next) {
		current = current->next;
	} else {
		while (current && !current->next) {
			current = current->parent;
		}

		if (!current) {
			if (p_wrap) {
				return tree->root;
			} else {
				return nullptr;
			}
		} else {
			current = current->next;
		}
	}

	return current;
}

FileSystemTreeItem *FileSystemTreeItem::get_next_visible(bool p_wrap) {
	FileSystemTreeItem *loop = this;
	FileSystemTreeItem *next_item = _get_next_in_tree(p_wrap);
	while (next_item && !next_item->is_visible_in_tree()) {
		next_item = next_item->_get_next_in_tree(p_wrap);
		if (next_item == loop) {
			// Check that we haven't looped all the way around to the start.
			next_item = nullptr;
			break;
		}
	}
	return next_item;
}

FileSystemTreeItem *FileSystemTreeItem::get_prev_in_tree(bool p_wrap) {
	FileSystemTreeItem *prev_item = _get_prev_in_tree(p_wrap, true);
	return prev_item;
}

FileSystemTreeItem *FileSystemTreeItem::get_next_in_tree(bool p_wrap) {
	FileSystemTreeItem *next_item = _get_next_in_tree(p_wrap, true);
	return next_item;
}

FileSystemTreeItem *FileSystemTreeItem::get_child(int p_index) {
	_create_children_cache();

	if (p_index < 0) {
		p_index += children_cache.size();
	}
	ERR_FAIL_INDEX_V(p_index, (int)children_cache.size(), nullptr);

	return children_cache[p_index];
}

int FileSystemTreeItem::get_visible_child_count() {
	_create_children_cache();
	int visible_count = 0;
	for (uint32_t i = 0; i < children_cache.size(); i++) {
		if (children_cache[i]->is_visible()) {
			visible_count += 1;
		}
	}
	return visible_count;
}

int FileSystemTreeItem::get_child_count() {
	_create_children_cache();
	return children_cache.size();
}

TypedArray<FileSystemTreeItem> FileSystemTreeItem::get_children() {
	// Don't need to explicitly create children cache, because get_child_count creates it.
	int size = get_child_count();
	TypedArray<FileSystemTreeItem> arr;
	arr.resize(size);
	for (int i = 0; i < size; i++) {
		arr[i] = children_cache[i];
	}

	return arr;
}

void FileSystemTreeItem::clear_children() {
	if (tree) { // TODO: Improve
		if (tree->display_mode == FileSystemTree::DISPLAY_MODE_TREE && tree->to_select.is_empty()) {
			// Keep the selected children.
			FileSystemTreeItem *item = tree->get_root();
			item = tree->get_next_selected(item);
			while (item) {
				if (item->is_visible_in_tree()) {
					FileSystemTreeItem *parent_item = item;
					while (parent_item && parent_item != this) {
						parent_item = parent_item->get_parent();
					}
					if (parent_item) {
						Dictionary d = item->get_metadata(0);
						String path = d["path"];
						if (FileSystemAccess::path_exists(path)) {
							tree->to_select.push_back(path);
							// print_line("keep select: ", path);
						}
					}
				}
				item = tree->get_next_selected(item);
			}
		}
	}

	FileSystemTreeItem *c = first_child;
	while (c) {
		FileSystemTreeItem *aux = c;
		c = c->get_next();
		aux->parent = nullptr; // So it won't try to recursively auto-remove from me in here.
		memdelete(aux);
	}

	first_child = nullptr;
	last_child = nullptr;
	children_cache.clear();
}

int FileSystemTreeItem::get_index() {
	int idx = 0;
	FileSystemTreeItem *c = this;

	while (c) {
		c = c->get_prev();
		idx++;
	}
	return idx - 1;
}

#ifdef DEV_ENABLED
void FileSystemTreeItem::validate_cache() const {
	if (!parent || parent->children_cache.is_empty()) {
		return;
	}
	FileSystemTreeItem *scan = parent->first_child;
	uint32_t index = 0;
	while (scan) {
		DEV_ASSERT(parent->children_cache[index] == scan);
		++index;
		scan = scan->get_next();
	}
	DEV_ASSERT(index == parent->children_cache.size());
}
#endif

void FileSystemTreeItem::move_before(FileSystemTreeItem *p_item) {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(is_root);
	ERR_FAIL_NULL(p_item->parent);

	if (p_item == this) {
		return;
	}

	FileSystemTreeItem *p = p_item->parent;
	while (p) {
		ERR_FAIL_COND_MSG(p == this, "Can't move to a descendant");
		p = p->parent;
	}

	FileSystemTree *old_tree = tree;
	_unlink_from_tree();
	_change_tree(p_item->tree);

	parent = p_item->parent;

	FileSystemTreeItem *item_prev = p_item->get_prev();
	if (item_prev) {
		item_prev->next = this;
		parent->children_cache.clear();
	} else {
		parent->first_child = this;
		// If the cache is empty, it has not been built but there
		// are items in the tree (note p_item != nullptr) so we cannot update it.
		if (!parent->children_cache.is_empty()) {
			parent->children_cache.insert(0, this);
		}
	}

	prev = item_prev;
	next = p_item;
	p_item->prev = this;

	if (tree && old_tree == tree) {
		tree->queue_redraw();
	}

	validate_cache();
}

void FileSystemTreeItem::move_after(FileSystemTreeItem *p_item) {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(is_root);
	ERR_FAIL_NULL(p_item->parent);

	if (p_item == this) {
		return;
	}

	FileSystemTreeItem *p = p_item->parent;
	while (p) {
		ERR_FAIL_COND_MSG(p == this, "Can't move to a descendant");
		p = p->parent;
	}

	FileSystemTree *old_tree = tree;
	_unlink_from_tree();
	_change_tree(p_item->tree);

	if (p_item->next) {
		p_item->next->prev = this;
	}
	parent = p_item->parent;
	prev = p_item;
	next = p_item->next;
	p_item->next = this;

	if (next) {
		parent->children_cache.clear();
	} else {
		parent->last_child = this;
		// If the cache is empty, it has not been built but there
		// are items in the tree (note p_item != nullptr,) so we cannot update it.
		if (!parent->children_cache.is_empty()) {
			parent->children_cache.push_back(this);
		}
	}

	if (tree && old_tree == tree) {
		tree->queue_redraw();
	}
	validate_cache();
}

void FileSystemTreeItem::set_selectable(bool p_selectable) {
	selectable = p_selectable;
}

bool FileSystemTreeItem::is_selectable() const {
	return selectable;
}

bool FileSystemTreeItem::is_selected() {
	return selectable && selected;
}

// TODO
void FileSystemTreeItem::set_as_cursor() {
	if (!tree) {
		return;
	}
	if (tree->selected_item == this) {
		return;
	}
	tree->selected_item = this;
	tree->queue_redraw();
}

void FileSystemTreeItem::select() {
	ERR_FAIL_NULL(tree);
	tree->item_selected(this);
}

void FileSystemTreeItem::deselect() {
	ERR_FAIL_NULL(tree);
	tree->item_deselected(this);
}

void FileSystemTreeItem::set_editable(int p_column, bool p_editable) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].editable == p_editable) {
		return;
	}

	cells.write[p_column].editable = p_editable;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool FileSystemTreeItem::is_editable(int p_column) {
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].editable;
}

void FileSystemTreeItem::set_custom_color(int p_column, const Color &p_color) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_color && cells[p_column].color == p_color) {
		return;
	}

	cells.write[p_column].custom_color = true;
	cells.write[p_column].color = p_color;
	_changed_notify(p_column);
}

Color FileSystemTreeItem::get_custom_color(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	if (!cells[p_column].custom_color) {
		return Color();
	}
	return cells[p_column].color;
}

void FileSystemTreeItem::clear_custom_color(int p_column) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_color = false;
	cells.write[p_column].color = Color();
	_changed_notify(p_column);
}

void FileSystemTreeItem::set_custom_font(int p_column, const Ref<Font> &p_font) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_font == p_font) {
		return;
	}

	cells.write[p_column].custom_font = p_font;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Font> FileSystemTreeItem::get_custom_font(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Font>());
	return cells[p_column].custom_font;
}

void FileSystemTreeItem::set_custom_font_size(int p_column, int p_font_size) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_font_size == p_font_size) {
		return;
	}

	cells.write[p_column].custom_font_size = p_font_size;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

int FileSystemTreeItem::get_custom_font_size(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), -1);
	return cells[p_column].custom_font_size;
}

void FileSystemTreeItem::set_tooltip_text(int p_column, const String &p_tooltip) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].tooltip = p_tooltip;
}

String FileSystemTreeItem::get_tooltip_text(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].tooltip;
}

void FileSystemTreeItem::set_custom_bg_color(int p_column, const Color &p_color, bool p_bg_outline) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_bg_color && cells[p_column].custom_bg_outline == p_bg_outline && cells[p_column].bg_color == p_color) {
		return;
	}

	cells.write[p_column].custom_bg_color = true;
	cells.write[p_column].custom_bg_outline = p_bg_outline;
	cells.write[p_column].bg_color = p_color;
	_changed_notify(p_column);
}

void FileSystemTreeItem::clear_custom_bg_color(int p_column) {
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_bg_color = false;
	cells.write[p_column].bg_color = Color();
	_changed_notify(p_column);
}

Color FileSystemTreeItem::get_custom_bg_color(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	if (!cells[p_column].custom_bg_color) {
		return Color();
	}
	return cells[p_column].bg_color;
}

void FileSystemTreeItem::set_text_alignment(int p_column, HorizontalAlignment p_alignment) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].text_alignment == p_alignment) {
		return;
	}

	cells.write[p_column].text_alignment = p_alignment;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

HorizontalAlignment FileSystemTreeItem::get_text_alignment(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), HORIZONTAL_ALIGNMENT_LEFT);
	return cells[p_column].text_alignment;
}

void FileSystemTreeItem::set_expand_right(int p_column, bool p_enable) {
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].expand_right == p_enable) {
		return;
	}

	cells.write[p_column].expand_right = p_enable;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool FileSystemTreeItem::get_expand_right(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].expand_right;
}

void FileSystemTreeItem::set_disable_folding(bool p_disable) {
	if (disable_folding == p_disable) {
		return;
	}

	disable_folding = p_disable;

	for (Cell &c : cells) {
		c.cached_minimum_size_dirty = true;
	}

	_changed_notify(0);
}

bool FileSystemTreeItem::is_folding_disabled() const {
	return disable_folding;
}

Size2 FileSystemTreeItem::get_minimum_size(int p_column) {
	ERR_FAIL_INDEX_V(p_column, cells.size(), Size2());
	FileSystemTree *parent_tree = get_tree();
	ERR_FAIL_NULL_V(parent_tree, Size2());

	const FileSystemTreeItem::Cell &cell = cells[p_column];

	if (cell.cached_minimum_size_dirty || cell.text_buf->is_dirty() || cell.dirty) {
		Size2 size = Size2(
				parent_tree->theme_cache.inner_item_margin_left + parent_tree->theme_cache.inner_item_margin_right,
				parent_tree->theme_cache.inner_item_margin_top + parent_tree->theme_cache.inner_item_margin_bottom);
		int content_height = size.height;

		// Text.
		if (!cell.text.is_empty()) {
			if (cell.dirty) {
				parent_tree->update_item_cell(this, p_column);
			}
			Size2 text_size = cell.text_buf->get_size();
			if (get_text_overrun_behavior(p_column) == TextServer::OVERRUN_NO_TRIMMING) {
				size.width += text_size.width;
			}
			content_height = MAX(content_height, text_size.height);
		}

		// Icon.
		if (cell.mode == CELL_MODE_CHECK) {
			Size2i check_size = parent_tree->theme_cache.checked->get_size();
			size.width += check_size.width + parent_tree->theme_cache.h_separation;
			content_height = MAX(content_height, check_size.height);
		}
		if (cell.icon.is_valid()) {
			Size2i icon_size = parent_tree->_get_cell_icon_size(cell);
			size.width += icon_size.width + parent_tree->theme_cache.icon_h_separation;
			content_height = MAX(content_height, icon_size.height);
		}

		size.height += content_height;
		cells.write[p_column].cached_minimum_size = size;
		cells.write[p_column].cached_minimum_size_dirty = false;
	}

	return cell.cached_minimum_size;
}

void FileSystemTreeItem::_call_recursive_bind(const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return;
	}

	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING_NAME;
		return;
	}

	StringName method = *p_args[0];

	call_recursive(method, &p_args[1], p_argcount - 1, r_error);
}

void recursive_call_aux(FileSystemTreeItem *p_item, const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (!p_item) {
		return;
	}
	p_item->callp(p_method, p_args, p_argcount, r_error);
	FileSystemTreeItem *c = p_item->get_first_child();
	while (c) {
		recursive_call_aux(c, p_method, p_args, p_argcount, r_error);
		c = c->get_next();
	}
}

void FileSystemTreeItem::call_recursive(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	recursive_call_aux(this, p_method, p_args, p_argcount, r_error);
}

void FileSystemTreeItem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cell_mode", "column", "mode"), &FileSystemTreeItem::set_cell_mode);
	ClassDB::bind_method(D_METHOD("get_cell_mode", "column"), &FileSystemTreeItem::get_cell_mode);

	ClassDB::bind_method(D_METHOD("set_auto_translate_mode", "column", "mode"), &FileSystemTreeItem::set_auto_translate_mode);
	ClassDB::bind_method(D_METHOD("get_auto_translate_mode", "column"), &FileSystemTreeItem::get_auto_translate_mode);

	ClassDB::bind_method(D_METHOD("set_edit_multiline", "column", "multiline"), &FileSystemTreeItem::set_edit_multiline);
	ClassDB::bind_method(D_METHOD("is_edit_multiline", "column"), &FileSystemTreeItem::is_edit_multiline);

	ClassDB::bind_method(D_METHOD("set_checked", "column", "checked"), &FileSystemTreeItem::set_checked);
	ClassDB::bind_method(D_METHOD("set_indeterminate", "column", "indeterminate"), &FileSystemTreeItem::set_indeterminate);
	ClassDB::bind_method(D_METHOD("is_checked", "column"), &FileSystemTreeItem::is_checked);
	ClassDB::bind_method(D_METHOD("is_indeterminate", "column"), &FileSystemTreeItem::is_indeterminate);

	ClassDB::bind_method(D_METHOD("propagate_check", "column", "emit_signal"), &FileSystemTreeItem::propagate_check, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("set_text", "column", "text"), &FileSystemTreeItem::set_text);
	ClassDB::bind_method(D_METHOD("get_text", "column"), &FileSystemTreeItem::get_text);

	ClassDB::bind_method(D_METHOD("set_description", "column", "description"), &FileSystemTreeItem::set_description);
	ClassDB::bind_method(D_METHOD("get_description", "column"), &FileSystemTreeItem::get_description);

	ClassDB::bind_method(D_METHOD("set_text_direction", "column", "direction"), &FileSystemTreeItem::set_text_direction);
	ClassDB::bind_method(D_METHOD("get_text_direction", "column"), &FileSystemTreeItem::get_text_direction);

	ClassDB::bind_method(D_METHOD("set_autowrap_mode", "column", "autowrap_mode"), &FileSystemTreeItem::set_autowrap_mode);
	ClassDB::bind_method(D_METHOD("get_autowrap_mode", "column"), &FileSystemTreeItem::get_autowrap_mode);

	ClassDB::bind_method(D_METHOD("set_text_overrun_behavior", "column", "overrun_behavior"), &FileSystemTreeItem::set_text_overrun_behavior);
	ClassDB::bind_method(D_METHOD("get_text_overrun_behavior", "column"), &FileSystemTreeItem::get_text_overrun_behavior);

	ClassDB::bind_method(D_METHOD("set_structured_text_bidi_override", "column", "parser"), &FileSystemTreeItem::set_structured_text_bidi_override);
	ClassDB::bind_method(D_METHOD("get_structured_text_bidi_override", "column"), &FileSystemTreeItem::get_structured_text_bidi_override);

	ClassDB::bind_method(D_METHOD("set_structured_text_bidi_override_options", "column", "args"), &FileSystemTreeItem::set_structured_text_bidi_override_options);
	ClassDB::bind_method(D_METHOD("get_structured_text_bidi_override_options", "column"), &FileSystemTreeItem::get_structured_text_bidi_override_options);

	ClassDB::bind_method(D_METHOD("set_language", "column", "language"), &FileSystemTreeItem::set_language);
	ClassDB::bind_method(D_METHOD("get_language", "column"), &FileSystemTreeItem::get_language);

	ClassDB::bind_method(D_METHOD("set_suffix", "column", "text"), &FileSystemTreeItem::set_suffix);
	ClassDB::bind_method(D_METHOD("get_suffix", "column"), &FileSystemTreeItem::get_suffix);

	ClassDB::bind_method(D_METHOD("set_icon", "column", "texture"), &FileSystemTreeItem::set_icon);
	ClassDB::bind_method(D_METHOD("get_icon", "column"), &FileSystemTreeItem::get_icon);

	ClassDB::bind_method(D_METHOD("set_icon_overlay", "column", "texture"), &FileSystemTreeItem::set_icon_overlay);
	ClassDB::bind_method(D_METHOD("get_icon_overlay", "column"), &FileSystemTreeItem::get_icon_overlay);

	ClassDB::bind_method(D_METHOD("set_icon_region", "column", "region"), &FileSystemTreeItem::set_icon_region);
	ClassDB::bind_method(D_METHOD("get_icon_region", "column"), &FileSystemTreeItem::get_icon_region);

	ClassDB::bind_method(D_METHOD("set_icon_max_width", "column", "width"), &FileSystemTreeItem::set_icon_max_width);
	ClassDB::bind_method(D_METHOD("get_icon_max_width", "column"), &FileSystemTreeItem::get_icon_max_width);

	ClassDB::bind_method(D_METHOD("set_icon_modulate", "column", "modulate"), &FileSystemTreeItem::set_icon_modulate);
	ClassDB::bind_method(D_METHOD("get_icon_modulate", "column"), &FileSystemTreeItem::get_icon_modulate);

	ClassDB::bind_method(D_METHOD("set_metadata", "column", "meta"), &FileSystemTreeItem::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata", "column"), &FileSystemTreeItem::get_metadata);

	ClassDB::bind_method(D_METHOD("set_custom_stylebox", "column", "stylebox"), &FileSystemTreeItem::set_custom_stylebox);
	ClassDB::bind_method(D_METHOD("get_custom_stylebox", "column"), &FileSystemTreeItem::get_custom_stylebox);

	ClassDB::bind_method(D_METHOD("set_collapsed", "enable"), &FileSystemTreeItem::set_collapsed);
	ClassDB::bind_method(D_METHOD("is_collapsed"), &FileSystemTreeItem::is_collapsed);

	ClassDB::bind_method(D_METHOD("set_collapsed_recursive", "enable"), &FileSystemTreeItem::set_collapsed_recursive);
	ClassDB::bind_method(D_METHOD("is_any_collapsed", "only_visible"), &FileSystemTreeItem::is_any_collapsed, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("set_visible", "enable"), &FileSystemTreeItem::set_visible);
	ClassDB::bind_method(D_METHOD("is_visible"), &FileSystemTreeItem::is_visible);
	ClassDB::bind_method(D_METHOD("is_visible_in_tree"), &FileSystemTreeItem::is_visible_in_tree);

	ClassDB::bind_method(D_METHOD("uncollapse_tree"), &FileSystemTreeItem::uncollapse_tree);

	ClassDB::bind_method(D_METHOD("set_custom_minimum_height", "height"), &FileSystemTreeItem::set_custom_minimum_height);
	ClassDB::bind_method(D_METHOD("get_custom_minimum_height"), &FileSystemTreeItem::get_custom_minimum_height);

	ClassDB::bind_method(D_METHOD("set_selectable", "selectable"), &FileSystemTreeItem::set_selectable);
	ClassDB::bind_method(D_METHOD("is_selectable"), &FileSystemTreeItem::is_selectable);

	ClassDB::bind_method(D_METHOD("is_selected"), &FileSystemTreeItem::is_selected);
	ClassDB::bind_method(D_METHOD("select"), &FileSystemTreeItem::select);
	ClassDB::bind_method(D_METHOD("deselect"), &FileSystemTreeItem::deselect);

	ClassDB::bind_method(D_METHOD("set_editable", "column", "enabled"), &FileSystemTreeItem::set_editable);
	ClassDB::bind_method(D_METHOD("is_editable", "column"), &FileSystemTreeItem::is_editable);

	ClassDB::bind_method(D_METHOD("set_custom_color", "column", "color"), &FileSystemTreeItem::set_custom_color);
	ClassDB::bind_method(D_METHOD("get_custom_color", "column"), &FileSystemTreeItem::get_custom_color);
	ClassDB::bind_method(D_METHOD("clear_custom_color", "column"), &FileSystemTreeItem::clear_custom_color);

	ClassDB::bind_method(D_METHOD("set_custom_font", "column", "font"), &FileSystemTreeItem::set_custom_font);
	ClassDB::bind_method(D_METHOD("get_custom_font", "column"), &FileSystemTreeItem::get_custom_font);

	ClassDB::bind_method(D_METHOD("set_custom_font_size", "column", "font_size"), &FileSystemTreeItem::set_custom_font_size);
	ClassDB::bind_method(D_METHOD("get_custom_font_size", "column"), &FileSystemTreeItem::get_custom_font_size);

	ClassDB::bind_method(D_METHOD("set_custom_bg_color", "column", "color", "just_outline"), &FileSystemTreeItem::set_custom_bg_color, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("clear_custom_bg_color", "column"), &FileSystemTreeItem::clear_custom_bg_color);
	ClassDB::bind_method(D_METHOD("get_custom_bg_color", "column"), &FileSystemTreeItem::get_custom_bg_color);

	ClassDB::bind_method(D_METHOD("set_tooltip_text", "column", "tooltip"), &FileSystemTreeItem::set_tooltip_text);
	ClassDB::bind_method(D_METHOD("get_tooltip_text", "column"), &FileSystemTreeItem::get_tooltip_text);
	ClassDB::bind_method(D_METHOD("set_text_alignment", "column", "text_alignment"), &FileSystemTreeItem::set_text_alignment);
	ClassDB::bind_method(D_METHOD("get_text_alignment", "column"), &FileSystemTreeItem::get_text_alignment);

	ClassDB::bind_method(D_METHOD("set_expand_right", "column", "enable"), &FileSystemTreeItem::set_expand_right);
	ClassDB::bind_method(D_METHOD("get_expand_right", "column"), &FileSystemTreeItem::get_expand_right);

	ClassDB::bind_method(D_METHOD("set_disable_folding", "disable"), &FileSystemTreeItem::set_disable_folding);
	ClassDB::bind_method(D_METHOD("is_folding_disabled"), &FileSystemTreeItem::is_folding_disabled);

	ClassDB::bind_method(D_METHOD("create_child", "index"), &FileSystemTreeItem::create_child, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("add_child", "child"), &FileSystemTreeItem::add_child);
	ClassDB::bind_method(D_METHOD("remove_child", "child"), &FileSystemTreeItem::remove_child);

	ClassDB::bind_method(D_METHOD("get_tree"), &FileSystemTreeItem::get_tree);

	ClassDB::bind_method(D_METHOD("get_next"), &FileSystemTreeItem::get_next);
	ClassDB::bind_method(D_METHOD("get_prev"), &FileSystemTreeItem::get_prev);
	ClassDB::bind_method(D_METHOD("get_parent"), &FileSystemTreeItem::get_parent);
	ClassDB::bind_method(D_METHOD("get_first_child"), &FileSystemTreeItem::get_first_child);

	ClassDB::bind_method(D_METHOD("get_next_in_tree", "wrap"), &FileSystemTreeItem::get_next_in_tree, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_prev_in_tree", "wrap"), &FileSystemTreeItem::get_prev_in_tree, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("get_next_visible", "wrap"), &FileSystemTreeItem::get_next_visible, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_prev_visible", "wrap"), &FileSystemTreeItem::get_prev_visible, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("get_child", "index"), &FileSystemTreeItem::get_child);
	ClassDB::bind_method(D_METHOD("get_child_count"), &FileSystemTreeItem::get_child_count);
	ClassDB::bind_method(D_METHOD("get_children"), &FileSystemTreeItem::get_children);
	ClassDB::bind_method(D_METHOD("get_index"), &FileSystemTreeItem::get_index);

	ClassDB::bind_method(D_METHOD("move_before", "item"), &FileSystemTreeItem::move_before);
	ClassDB::bind_method(D_METHOD("move_after", "item"), &FileSystemTreeItem::move_after);

	{
		MethodInfo mi;
		mi.name = "call_recursive";
		mi.arguments.push_back(PropertyInfo(Variant::STRING_NAME, "method"));

		ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "call_recursive", &FileSystemTreeItem::_call_recursive_bind, mi);
	}

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collapsed"), "set_collapsed", "is_collapsed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "visible"), "set_visible", "is_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "disable_folding"), "set_disable_folding", "is_folding_disabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "custom_minimum_height", PROPERTY_HINT_RANGE, "0,1000,1"), "set_custom_minimum_height", "get_custom_minimum_height");

	BIND_ENUM_CONSTANT(CELL_MODE_STRING);
	BIND_ENUM_CONSTANT(CELL_MODE_CHECK);
	BIND_ENUM_CONSTANT(CELL_MODE_ICON);
}

FileSystemTreeItem::FileSystemTreeItem(FileSystemTree *p_tree) {
	tree = p_tree;
}

FileSystemTreeItem::~FileSystemTreeItem() {
	_unlink_from_tree();
	_change_tree(nullptr);

	validate_cache();
	prev = nullptr;
	clear_children();
}

/*************************** END of FileSystemTreeItem ***************************/

void FileSystemTree::_update_theme_item_cache() {
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

Size2 FileSystemTree::_get_cell_icon_size(const FileSystemTreeItem::Cell &p_cell) const {
	Size2i icon_size = p_cell.get_icon_size();

	int max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		max_width = theme_cache.icon_max_width;
	}
	if (p_cell.icon_max_w > 0 && (max_width == 0 || p_cell.icon_max_w < max_width)) {
		max_width = p_cell.icon_max_w;
	}

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

int FileSystemTree::compute_item_height(FileSystemTreeItem *p_item) const {
	if ((p_item == root && hide_root) || !p_item->is_visible_in_tree()) {
		return 0;
	}

	ERR_FAIL_COND_V(theme_cache.font.is_null(), 0);
	int height = 0;

	for (int i = 0; i < columns.size(); i++) {
		height = MAX(height, p_item->get_minimum_size(i).y);
	}
	int item_min_height = MAX(theme_cache.font->get_height(theme_cache.font_size), p_item->get_custom_minimum_height());
	if (height < item_min_height) {
		height = item_min_height;
	}

	height += theme_cache.v_separation;

	return height;
}

int FileSystemTree::get_item_height(FileSystemTreeItem *p_item) const {
	if (!p_item->is_visible_in_tree()) {
		return 0;
	}
	int height = compute_item_height(p_item);
	height += theme_cache.v_separation;

	if (!p_item->collapsed) { // If not collapsed, check the children.

		FileSystemTreeItem *c = p_item->first_child;

		while (c) {
			height += get_item_height(c);

			c = c->next;
		}
	}

	return height;
}

Point2i FileSystemTree::convert_rtl_position(const Point2i &pos, int width) const {
	if (cache.rtl) {
		return Point2i(get_size().width - pos.x - width, pos.y);
	}
	return pos;
}

Rect2i FileSystemTree::convert_rtl_rect(const Rect2i &rect) const {
	if (cache.rtl) {
		return Rect2i(Point2i(get_size().width - rect.position.x - rect.size.x, rect.position.y), rect.size);
	}
	return rect;
}

void FileSystemTree::draw_item_rect(const FileSystemTreeItem::Cell &p_cell, const Rect2i &p_rect, const Color &p_color, const Color &p_icon_color, int p_ol_size, const Color &p_ol_color) const {
	ERR_FAIL_COND(theme_cache.font.is_null());

	Rect2i rect = p_rect;
	Size2 ts = p_cell.text_buf->get_size();
	bool rtl = is_layout_rtl();

	Size2i icon_size;
	if (p_cell.icon.is_valid()) {
		icon_size = _get_cell_icon_size(p_cell);
	}

	int displayed_width = 0;
	if (p_cell.icon.is_valid()) {
		displayed_width = MIN(displayed_width + icon_size.width + theme_cache.icon_h_separation, rect.size.width);
	}
	if (displayed_width + ts.width > rect.size.width) {
		ts.width = rect.size.width - displayed_width;
	}
	displayed_width += ts.width;

	int empty_width = rect.size.width - displayed_width;

	switch (p_cell.text_alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT: {
			if (rtl) {
				rect.position.x += empty_width;
			}
		} break;
		case HORIZONTAL_ALIGNMENT_CENTER:
			rect.position.x += empty_width / 2;
			break;
		case HORIZONTAL_ALIGNMENT_RIGHT:
			if (!rtl) {
				rect.position.x += empty_width;
			}
			break;
	}

	RID ci = content_ci;

	if (rtl) {
		if (ts.width > 0) {
			Point2 draw_pos = rect.position;
			draw_pos.y += Math::floor((rect.size.y - p_cell.text_buf->get_size().y) * 0.5);
			if (p_ol_size > 0 && p_ol_color.a > 0) {
				p_cell.text_buf->draw_outline(ci, draw_pos, p_ol_size, p_ol_color);
			}
			p_cell.text_buf->draw(ci, draw_pos, p_color);
		}
		rect.position.x += ts.width + theme_cache.icon_h_separation;
	}

	if (p_cell.icon.is_valid()) {
		Point2 icon_pos = rect.position + Size2i(0, Math::floor((real_t)(rect.size.y - icon_size.y) / 2));
		Rect2i icon_region = (p_cell.icon_region == Rect2i()) ? Rect2i(Point2(), p_cell.icon->get_size()) : p_cell.icon_region;
		if (icon_size.width > rect.size.width) {
			icon_region.size.width = icon_region.size.width * rect.size.width / icon_size.width;
			icon_size.width = rect.size.width;
		}
		p_cell.draw_icon(ci, icon_pos, icon_size, icon_region, p_icon_color);
		rect.position.x += icon_size.x + theme_cache.icon_h_separation;
	}

	if (!rtl && ts.width > 0) {
		Point2 draw_pos = rect.position;
		draw_pos.y += Math::floor((rect.size.y - p_cell.text_buf->get_size().y) * 0.5);
		if (p_ol_size > 0 && p_ol_color.a > 0) {
			p_cell.text_buf->draw_outline(ci, draw_pos, p_ol_size, p_ol_color);
		}
		p_cell.text_buf->draw(ci, draw_pos, p_color);
	}
}

void FileSystemTree::update_column(int p_col) {
	columns.write[p_col].text_buf->clear();
	if (columns[p_col].text_direction == Control::TEXT_DIRECTION_INHERITED) {
		columns.write[p_col].text_buf->set_direction(is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
	} else {
		columns.write[p_col].text_buf->set_direction((TextServer::Direction)columns[p_col].text_direction);
	}

	columns.write[p_col].xl_title = atr(columns[p_col].title);
	const String &lang = columns[p_col].language.is_empty() ? _get_locale() : columns[p_col].language;
	columns.write[p_col].text_buf->add_string(columns[p_col].xl_title, theme_cache.tb_font, theme_cache.tb_font_size, lang);
	columns.write[p_col].cached_minimum_width_dirty = true;
}

void FileSystemTree::update_item_cell(FileSystemTreeItem *p_item, int p_col) const {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_INDEX(p_col, p_item->cells.size());

	String valtext;

	p_item->cells.write[p_col].text_buf->clear();
	// Don't auto translate if it's in string mode and editable, as the text can be changed to anything by the user.
	if (!p_item->cells[p_col].editable || p_item->cells[p_col].mode != FileSystemTreeItem::CELL_MODE_STRING) {
		p_item->cells.write[p_col].xl_text = p_item->atr(p_col, p_item->cells[p_col].text);
	} else {
		p_item->cells.write[p_col].xl_text = p_item->cells[p_col].text;
	}

	valtext = p_item->cells[p_col].xl_text;

	if (!p_item->cells[p_col].suffix.is_empty()) {
		if (!valtext.is_empty()) {
			valtext += " ";
		}
		valtext += p_item->cells[p_col].suffix;
	}

	if (p_item->cells[p_col].text_direction == Control::TEXT_DIRECTION_INHERITED) {
		p_item->cells.write[p_col].text_buf->set_direction(is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
	} else {
		p_item->cells.write[p_col].text_buf->set_direction((TextServer::Direction)p_item->cells[p_col].text_direction);
	}

	Ref<Font> font;
	if (p_item->cells[p_col].custom_font.is_valid()) {
		font = p_item->cells[p_col].custom_font;
	} else {
		font = theme_cache.font;
	}

	int font_size;
	if (p_item->cells[p_col].custom_font_size > 0) {
		font_size = p_item->cells[p_col].custom_font_size;
	} else {
		font_size = theme_cache.font_size;
	}
	const String &lang = p_item->cells[p_col].language.is_empty() ? _get_locale() : p_item->cells[p_col].language;
	p_item->cells.write[p_col].text_buf->add_string(valtext, font, font_size, lang);

	BitField<TextServer::LineBreakFlag> break_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	switch (p_item->cells.write[p_col].autowrap_mode) {
		case TextServer::AUTOWRAP_OFF:
			break;
		case TextServer::AUTOWRAP_ARBITRARY:
			break_flags.set_flag(TextServer::BREAK_GRAPHEME_BOUND);
			break;
		case TextServer::AUTOWRAP_WORD:
			break_flags.set_flag(TextServer::BREAK_WORD_BOUND);
			break;
		case TextServer::AUTOWRAP_WORD_SMART:
			break_flags.set_flag(TextServer::BREAK_WORD_BOUND);
			break_flags.set_flag(TextServer::BREAK_ADAPTIVE);
			break;
	}
	p_item->cells.write[p_col].text_buf->set_break_flags(break_flags);

	TS->shaped_text_set_bidi_override(p_item->cells[p_col].text_buf->get_rid(), structured_text_parser(p_item->cells[p_col].st_parser, p_item->cells[p_col].st_args, valtext));
	p_item->cells.write[p_col].dirty = false;
}

void FileSystemTree::update_item_cache(FileSystemTreeItem *p_item) const {
	for (int i = 0; i < p_item->cells.size(); i++) {
		update_item_cell(p_item, i);
	}

	FileSystemTreeItem *c = p_item->first_child;
	while (c) {
		update_item_cache(c);
		c = c->next;
	}
}

int FileSystemTree::draw_item(const Point2i &p_pos, const Point2 &p_draw_ofs, const Size2 &p_draw_size, FileSystemTreeItem *p_item, int &r_self_height) {
	ERR_FAIL_NULL_V(p_item, -1);

	const real_t bottom_margin = theme_cache.panel_style->get_margin(SIDE_BOTTOM); // Extra stylebox space below the content
	const real_t draw_height = p_draw_size.height + bottom_margin; // Visible height including bottom margin

	// Cull item if it's beyond the bottom of the visible area.
	if (p_pos.y - theme_cache.offset.y > draw_height) {
		return -1;
	}

	if (!p_item->is_visible_in_tree()) {
		return 0;
	}

	RID ci = content_ci;

	int htotal = 0;

	int label_h = 0;
	bool rtl = cache.rtl;

	// Draw label, if height fits.
	bool skip = (p_item == root && hide_root);

	if (!skip) {
		// Draw separation.

		ERR_FAIL_COND_V(theme_cache.font.is_null(), -1);

		int ofs = p_pos.x + ((p_item->disable_folding || hide_folding) ? theme_cache.h_separation : theme_cache.item_margin);
		int skip2 = 0;

		bool is_row_hovered = (!cache.hover_header_row && cache.hover_item == p_item);
		Rect2i row_rect;

		for (int i = 0; i < columns.size(); i++) {
			if (skip2) {
				skip2--;
				continue;
			}

			int item_width = get_column_width(i);

			if (i == 0) {
				item_width -= ofs;

				if (item_width <= 0) {
					ofs = get_column_width(0);
					r_self_height = compute_item_height(p_item);
					label_h = r_self_height + theme_cache.v_separation;
					continue;
				}
			} else {
				ofs += theme_cache.h_separation;
				item_width -= theme_cache.h_separation;
			}

			if (p_item->cells[i].expand_right) {
				int plus = 1;
				while (i + plus < columns.size() && !p_item->cells[i + plus].editable && p_item->cells[i + plus].mode == FileSystemTreeItem::CELL_MODE_STRING && p_item->cells[i + plus].xl_text.is_empty() && p_item->cells[i + plus].icon.is_null()) {
					item_width += get_column_width(i + plus);
					plus++;
					skip2++;
				}
			}

			int text_width = item_width - theme_cache.inner_item_margin_left - theme_cache.inner_item_margin_right;
			if (p_item->cells[i].icon.is_valid()) {
				text_width -= _get_cell_icon_size(p_item->cells[i]).x + theme_cache.icon_h_separation;
			}

			p_item->cells.write[i].text_buf->set_width(text_width);

			r_self_height = compute_item_height(p_item);
			label_h = r_self_height + theme_cache.v_separation;

			// Bottom edge for top-culling
			const real_t item_bottom = p_pos.y + label_h - theme_cache.offset.y + p_draw_ofs.y;
			if (item_bottom < 0.0f) {
				continue; // No need to draw.
			}

			Rect2i item_rect = Rect2i(Point2i(ofs, p_pos.y) - theme_cache.offset + p_draw_ofs, Size2i(item_width, label_h));
			Rect2i cell_rect = item_rect;
			if (i != 0) {
				cell_rect.position.x -= theme_cache.h_separation;
				cell_rect.size.x += theme_cache.h_separation;
			}

			if (i == 0) {
				const Rect2 content_rect = _get_content_rect();
				row_rect = Rect2i(Point2i(content_rect.position.x, item_rect.position.y), Size2i(content_rect.size.x, item_rect.size.y));
				row_rect = convert_rtl_rect(row_rect);

				if (p_item->selected || is_row_hovered) {
					if (p_item->selected) {
						if (is_row_hovered) {
							if (has_focus(true)) {
								theme_cache.hovered_selected_focus->draw(ci, row_rect);
							} else {
								theme_cache.hovered_selected->draw(ci, row_rect);
							}
						} else {
							if (has_focus(true)) {
								theme_cache.selected_focus->draw(ci, row_rect);
							} else {
								theme_cache.selected->draw(ci, row_rect);
							}
						}

						Rect2i r = cell_rect;
						p_item->focus_rect = Rect2(r.position, r.size);
					} else if (!drop_mode_flags) {
						theme_cache.hovered->draw(ci, row_rect);
					}
				}
			}

			if (theme_cache.draw_guides) {
				Rect2 r = convert_rtl_rect(cell_rect);
				RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2i(r.position.x, r.position.y + r.size.height), r.position + r.size, theme_cache.guide_color, 1);
			}

			if (p_item->cells[i].custom_bg_color) {
				Rect2 r = cell_rect;
				if (i == 0) {
					r.position.x = p_draw_ofs.x;
					r.size.x = item_width + ofs;
				}
				r = convert_rtl_rect(r);
				if (p_item->cells[i].custom_bg_outline) {
					RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y, r.size.x, 1), p_item->cells[i].bg_color);
					RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y + r.size.y - 1, r.size.x, 1), p_item->cells[i].bg_color);
					RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y, 1, r.size.y), p_item->cells[i].bg_color);
					RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x + r.size.x - 1, r.position.y, 1, r.size.y), p_item->cells[i].bg_color);
				} else {
					RenderingServer::get_singleton()->canvas_item_add_rect(ci, r, p_item->cells[i].bg_color);
				}
			}

			if (p_item->cells[i].custom_stylebox.is_valid()) {
				Rect2 r = cell_rect;
				if (i == 0) {
					r.position.x = p_draw_ofs.x;
					r.size.x = item_width + ofs;
				}
				r = convert_rtl_rect(r);
				p_item->cells[i].custom_stylebox->draw(ci, r);
			}

			if (drop_mode_flags && drop_mode_over) {
				Rect2 r = convert_rtl_rect(cell_rect);
				if (drop_mode_over == p_item) {
					if (drop_mode_section == 0 || drop_mode_section == -1) {
						// Line above.
						RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y, r.size.x, 1), theme_cache.drop_position_color);
					}
					if (drop_mode_section == 0) {
						// Side lines.
						RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y, 1, r.size.y), theme_cache.drop_position_color);
						RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x + r.size.x - 1, r.position.y, 1, r.size.y), theme_cache.drop_position_color);
					}
					if (drop_mode_section == 0 || (drop_mode_section == 1 && (!p_item->get_first_child() || p_item->is_collapsed()))) {
						// Line below.
						RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y + r.size.y, r.size.x, 1), theme_cache.drop_position_color);
					}
				} else if (drop_mode_over == p_item->get_parent()) {
					if (drop_mode_section == 1 && !p_item->get_prev()) {
						// Line above.
						RenderingServer::get_singleton()->canvas_item_add_rect(ci, Rect2(r.position.x, r.position.y, r.size.x, 1), theme_cache.drop_position_color);
					}
				}
			}

			Color cell_color;
			if (p_item->cells[i].custom_color) {
				cell_color = p_item->cells[i].color;
			} else {
				bool draw_as_hover = !drop_mode_flags && is_row_hovered;
				cell_color = p_item->selected && draw_as_hover ? theme_cache.font_hovered_selected_color : (p_item->selected ? theme_cache.font_selected_color : (draw_as_hover ? theme_cache.font_hovered_color : theme_cache.font_color));
			}

			Color font_outline_color = theme_cache.font_outline_color;
			int outline_size = theme_cache.font_outline_size;
			Color icon_col = p_item->cells[i].icon_color;

			if (p_item->cells[i].dirty) {
				update_item_cell(p_item, i);
			}

			item_rect = convert_rtl_rect(item_rect);
			item_rect = item_rect.grow_individual(-theme_cache.inner_item_margin_left, -theme_cache.inner_item_margin_top, -theme_cache.inner_item_margin_right, -theme_cache.inner_item_margin_bottom);

			Point2i text_pos = item_rect.position;
			text_pos.y += Math::floor((item_rect.size.y - p_item->cells[i].text_buf->get_size().y) * 0.5);

			switch (p_item->cells[i].mode) {
				case FileSystemTreeItem::CELL_MODE_STRING: {
					draw_item_rect(p_item->cells[i], item_rect, cell_color, icon_col, outline_size, font_outline_color);
				} break;
				case FileSystemTreeItem::CELL_MODE_CHECK: {
					Point2i check_ofs = item_rect.position;
					check_ofs.y += Math::floor((real_t)(item_rect.size.y - theme_cache.checked->get_height()) / 2);

					if (p_item->cells[i].editable) {
						if (p_item->cells[i].indeterminate) {
							theme_cache.indeterminate->draw(ci, check_ofs);
						} else if (p_item->cells[i].checked) {
							theme_cache.checked->draw(ci, check_ofs);
						} else {
							theme_cache.unchecked->draw(ci, check_ofs);
						}
					} else {
						if (p_item->cells[i].indeterminate) {
							theme_cache.indeterminate_disabled->draw(ci, check_ofs);
						} else if (p_item->cells[i].checked) {
							theme_cache.checked_disabled->draw(ci, check_ofs);
						} else {
							theme_cache.unchecked_disabled->draw(ci, check_ofs);
						}
					}

					int check_w = theme_cache.checked->get_width() + theme_cache.check_h_separation;

					text_pos.x += check_w;

					item_rect.size.x -= check_w;
					item_rect.position.x += check_w;

					if (!p_item->cells[i].editable) {
						cell_color = theme_cache.font_disabled_color;
					}

					draw_item_rect(p_item->cells[i], item_rect, cell_color, icon_col, outline_size, font_outline_color);

				} break;
				case FileSystemTreeItem::CELL_MODE_ICON: {
					if (p_item->cells[i].icon.is_null()) {
						break;
					}
					Size2i icon_size = _get_cell_icon_size(p_item->cells[i]);
					Point2i icon_ofs = (item_rect.size - icon_size) / 2;
					icon_ofs += item_rect.position;

					p_item->cells[i].icon->draw_rect(ci, Rect2(icon_ofs, icon_size), false, icon_col);

				} break;
			}

			if (i == 0) {
				ofs = get_column_width(0);
			} else {
				ofs += item_width;
			}

			if (i == 0 && selected_item == p_item && row_rect.has_area()) {
				if (has_focus(true)) {
					theme_cache.cursor->draw(ci, row_rect);
				} else {
					theme_cache.cursor_unfocus->draw(ci, row_rect);
				}
			}
		}

		// Bottom edge for arrow visibility
		const real_t item_bottom = p_pos.y + label_h - theme_cache.offset.y + p_draw_ofs.y;
		if (item_bottom >= 0.0f) {
			// Draw the folding arrow.
			if (!p_item->disable_folding && !hide_folding && p_item->first_child && p_item->get_visible_child_count() != 0) { // Has visible children, draw the guide box.
				Ref<Texture2D> arrow;

				if (p_item->collapsed) {
					if (rtl) {
						arrow = theme_cache.arrow_collapsed_mirrored;
					} else {
						arrow = theme_cache.arrow_collapsed;
					}
				} else {
					arrow = theme_cache.arrow;
				}

				Size2 arrow_full_size = arrow->get_size();

				Point2 apos = p_pos + Point2i(0, (label_h - arrow_full_size.height) / 2) - theme_cache.offset + p_draw_ofs;
				apos.x += theme_cache.item_margin - arrow_full_size.width;

				Size2 arrow_draw_size = arrow_full_size;
				int out_width = p_pos.x + theme_cache.item_margin - get_column_width(0);
				if (out_width > 0) {
					arrow_draw_size.width -= out_width;
				}

				if (arrow_draw_size.width > 0) {
					apos = convert_rtl_position(apos, arrow_draw_size.width);
					Point2 src_pos = Point2();
					if (rtl) {
						src_pos = Point2(arrow_full_size.width - arrow_draw_size.width, 0);
					}
					Rect2 arrow_rect = Rect2(apos, arrow_draw_size);
					Rect2 arrow_src_rect = Rect2(src_pos, arrow_draw_size);
					arrow->draw_rect_region(ci, arrow_rect, arrow_src_rect);
				}
			}
		}
	}

	Point2 children_pos = p_pos;

	if (!skip) {
		children_pos.x += theme_cache.item_margin;
		htotal += label_h;
		children_pos.y += htotal;
	}

	if (!p_item->collapsed) { // If not collapsed, check the children.
		FileSystemTreeItem *c = p_item->first_child;

		int base_ofs = children_pos.y - theme_cache.offset.y + p_draw_ofs.y;
		float prev_ofs = base_ofs;
		float prev_hl_ofs = base_ofs;
		bool has_sibling_selection = c && _is_sibling_branch_selected(c);

		while (c) {
			int child_h = -1;
			int child_self_height = 0;
			if (htotal >= 0) {
				child_h = draw_item(children_pos, p_draw_ofs, p_draw_size, c, child_self_height);
				child_self_height += theme_cache.v_separation;
			}

			// Draw relationship lines.
			if (theme_cache.draw_relationship_lines > 0 && (!hide_root || c->parent != root) && c->is_visible_in_tree()) {
				int parent_ofs = p_pos.x + ((p_item->disable_folding || hide_folding) ? theme_cache.h_separation : theme_cache.item_margin - theme_cache.arrow->get_width() * 0.5);
				Point2i parent_pos = Point2i(parent_ofs, p_pos.y + label_h * 0.5 + theme_cache.arrow->get_height() * 0.5) - theme_cache.offset + p_draw_ofs;

				int root_ofs = children_pos.x + ((c->disable_folding || hide_folding) ? theme_cache.h_separation : theme_cache.item_margin - (c->get_visible_child_count() > 0 ? theme_cache.arrow->get_width() : 0));
				root_ofs = MIN(root_ofs, get_column_width(0) + p_draw_ofs.x);
				Point2i root_pos = Point2i(root_ofs, children_pos.y + child_self_height * 0.5) - theme_cache.offset + p_draw_ofs;

				bool is_no_space = root_pos.x <= parent_pos.x;

				float line_width = theme_cache.relationship_line_width * Math::round(theme_cache.base_scale);
				float parent_line_width = theme_cache.parent_hl_line_width * Math::round(theme_cache.base_scale);
				float children_line_width = theme_cache.children_hl_line_width * Math::round(theme_cache.base_scale);

				float line_pixel_shift = int(Math::round(line_width * content_scale_factor)) % 2 == 0 ? 0.0 : 0.5;
				float parent_line_pixel_shift = int(Math::round(parent_line_width * content_scale_factor)) % 2 == 0 ? 0.0 : 0.5;
				float children_line_pixel_shift = int(Math::round(children_line_width * content_scale_factor)) % 2 == 0 ? 0.0 : 0.5;

				int more_prev_ofs = 0;

				if (root_pos.y + line_width >= 0) {
					root_pos = convert_rtl_position(root_pos);
					parent_pos = convert_rtl_position(parent_pos);
					float parent_bottom_y = root_pos.y + parent_line_width * 0.5 + parent_line_pixel_shift;

					// Order of parts on this bend: the horizontal line first, then the vertical line.
					if (_is_branch_selected(c)) {
						// If this item or one of its children is selected, we draw the line using parent highlight style.
						if (!is_no_space) {
							if (htotal >= 0) {
								RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(root_pos.x, root_pos.y + parent_line_pixel_shift), Point2(parent_pos.x + parent_line_width * 0.5 + parent_line_pixel_shift, root_pos.y + parent_line_pixel_shift), theme_cache.parent_hl_line_color, parent_line_width);
							}
							RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(parent_pos.x + parent_line_pixel_shift, parent_bottom_y), Point2(parent_pos.x + parent_line_pixel_shift, prev_hl_ofs), theme_cache.parent_hl_line_color, parent_line_width);
						}

						more_prev_ofs = theme_cache.parent_hl_line_margin;
						prev_hl_ofs = parent_bottom_y;
						has_sibling_selection = _is_sibling_branch_selected(c);
					} else if (p_item->is_selected()) {
						// If parent item is selected (but this item is not), we draw the line using children highlight style.
						// Siblings of the selected branch can be drawn with a slight offset and their vertical line must appear as highlighted.
						if (has_sibling_selection) {
							if (!is_no_space) {
								if (htotal >= 0) {
									RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(root_pos.x, root_pos.y + children_line_pixel_shift), Point2i(parent_pos.x + parent_line_width * 0.5 + children_line_pixel_shift, root_pos.y + children_line_pixel_shift), theme_cache.children_hl_line_color, children_line_width);
								}
								RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(parent_pos.x + parent_line_pixel_shift, parent_bottom_y), Point2(parent_pos.x + parent_line_pixel_shift, prev_hl_ofs), theme_cache.parent_hl_line_color, parent_line_width);
							}

							prev_hl_ofs = parent_bottom_y;
						} else {
							if (!is_no_space) {
								if (htotal >= 0) {
									RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(root_pos.x, root_pos.y + children_line_pixel_shift), Point2(parent_pos.x + children_line_width * 0.5 + children_line_pixel_shift, root_pos.y + children_line_pixel_shift), theme_cache.children_hl_line_color, children_line_width);
								}
								RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(parent_pos.x + children_line_pixel_shift, root_pos.y + children_line_width * 0.5 + children_line_pixel_shift), Point2(parent_pos.x + children_line_pixel_shift, prev_ofs + children_line_width * 0.5), theme_cache.children_hl_line_color, children_line_width);
							}
						}
					} else {
						// If nothing of the above is true, we draw the line using normal style.
						// Siblings of the selected branch can be drawn with a slight offset and their vertical line must appear as highlighted.
						if (has_sibling_selection) {
							if (!is_no_space) {
								if (htotal >= 0) {
									RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(root_pos.x, root_pos.y + line_pixel_shift), Point2(parent_pos.x + theme_cache.parent_hl_line_margin, root_pos.y + line_pixel_shift), theme_cache.relationship_line_color, line_width);
								}
								RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(parent_pos.x + parent_line_pixel_shift, parent_bottom_y), Point2(parent_pos.x + parent_line_pixel_shift, prev_hl_ofs), theme_cache.parent_hl_line_color, parent_line_width);
							}

							prev_hl_ofs = parent_bottom_y;
						} else {
							if (!is_no_space) {
								if (htotal >= 0) {
									RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(root_pos.x, root_pos.y + line_pixel_shift), Point2(parent_pos.x + line_width * 0.5 + line_pixel_shift, root_pos.y + line_pixel_shift), theme_cache.relationship_line_color, line_width);
								}
								RenderingServer::get_singleton()->canvas_item_add_line(ci, Point2(parent_pos.x + line_pixel_shift, root_pos.y + line_width * 0.5 + line_pixel_shift), Point2(parent_pos.x + line_pixel_shift, prev_ofs + line_width * 0.5), theme_cache.relationship_line_color, line_width);
							}
						}
					}
				} else {
					if (_is_branch_selected(c)) {
						has_sibling_selection = _is_sibling_branch_selected(c);
					}
				}

				prev_ofs = root_pos.y + more_prev_ofs + line_pixel_shift;
			}

			if (child_h < 0) {
				if (htotal == -1) {
					break; // Last loop done, stop.
				}

				if (theme_cache.draw_relationship_lines == 0) {
					return -1; // No need to draw anymore, full stop.
				}

				htotal = -1;
				children_pos.y = theme_cache.offset.y + draw_height;
			} else {
				htotal += child_h;
				children_pos.y += child_h;
			}

			c = c->next;
		}
	}

	return htotal;
}

int FileSystemTree::_count_selected_items(FileSystemTreeItem *p_from) const {
	int count = 0;

	if (p_from->is_selected()) {
		count++;
	}

	for (FileSystemTreeItem *c = p_from->get_first_child(); c; c = c->get_next()) {
		count += _count_selected_items(c);
	}

	return count;
}

bool FileSystemTree::_is_branch_selected(FileSystemTreeItem *p_from) const {
	if (p_from->is_selected()) {
		return true;
	}

	FileSystemTreeItem *child_item = p_from->get_first_child();
	while (child_item) {
		if (_is_branch_selected(child_item)) {
			return true;
		}
		child_item = child_item->get_next();
	}

	return false;
}

bool FileSystemTree::_is_sibling_branch_selected(FileSystemTreeItem *p_from) const {
	FileSystemTreeItem *sibling_item = p_from->get_next();
	while (sibling_item) {
		if (_is_branch_selected(sibling_item)) {
			return true;
		}
		sibling_item = sibling_item->get_next();
	}

	return false;
}

void FileSystemTree::select_item(FileSystemTreeItem *p_selected, FileSystemTreeItem *p_current, FileSystemTreeItem *p_prev, bool *r_in_range, bool p_force_deselect) {
	ERR_FAIL_NULL(p_selected);

	popup_editor->hide();

	// Select items in the range from the previous to the currently selected item.
	bool switched = false;
	if (r_in_range && !*r_in_range && (p_current == p_selected || p_current == p_prev)) {
		*r_in_range = true;
		switched = true;
	}

	if (p_current->selectable) {
		if (!r_in_range) { // Select single item.
			if (p_selected == p_current && (!p_current->selected || allow_reselect)) {
				p_current->selected = true;
				selected_item = p_selected;
				emit_signal(SNAME("item_selected"), p_current, true);
			} else if (p_current->selected) {
				// Deselect other selected items.
				if (p_selected != p_current) {
					p_current->selected = false;
					emit_signal(SNAME("item_selected"), p_current, false);
				}
			}
		} else { // Select the items in the range.
			if (*r_in_range && !p_force_deselect) {
				if (!p_current->selected) {
					p_current->selected = true;
					emit_signal(SNAME("item_selected"), p_current, true);
				}
			} else {
				if (p_current->selected) {
					emit_signal(SNAME("item_selected"), p_current, false);
				}
				p_current->selected = false;
			}
		}
	}

	if (r_in_range && *r_in_range) {
		if (switched && (p_selected == p_prev)) {
			*r_in_range = false;
		} else if (!switched && (p_current == p_selected || p_current == p_prev)) {
			*r_in_range = false;
		}
	}

	FileSystemTreeItem *c = p_current->first_child;

	while (c) {
		if (c->is_visible()) {
			select_item(p_selected, c, p_prev, r_in_range, p_current->is_collapsed() || p_force_deselect);
		}
		c = c->next;
	}
}

Rect2 FileSystemTree::search_item_rect(FileSystemTreeItem *p_from, FileSystemTreeItem *p_item) {
	return Rect2();
}

int FileSystemTree::propagate_mouse_event(const Point2i &p_pos, int x_ofs, int y_ofs, int x_limit, bool p_double_click, FileSystemTreeItem *p_item, MouseButton p_button, const Ref<InputEventWithModifiers> &p_mod) {
	if (p_item && !p_item->is_visible_in_tree()) {
		// Skip any processing of invisible items.
		return 0;
	}
	if (p_pos.x > x_limit) {
		// Inside the scroll area.
		return -1;
	}

	int item_h = compute_item_height(p_item) + theme_cache.v_separation;

	bool skip = (p_item == root && hide_root);

	if (!skip && p_pos.y < item_h) {
		if (!p_item->disable_folding && !hide_folding && p_item->first_child && (p_pos.x < (x_ofs + theme_cache.item_margin))) {
			if (enable_recursive_folding && p_mod->is_shift_pressed()) {
				p_item->set_collapsed_recursive(!p_item->is_collapsed());
			} else {
				p_item->set_collapsed(!p_item->is_collapsed());
			}
			return -1;
		}

		FindColumnButtonResult find_result = _find_column_and_button_at_pos(p_pos.x, p_item, x_ofs, x_limit);

		if (find_result.column_index == -1) {
			return -1;
		}

		int col = find_result.column_index;
		int col_width = find_result.column_width;
		int col_ofs = find_result.column_offset;
		int x = find_result.pos_x;
		ERR_FAIL_INDEX_V(col, p_item->cells.size(), -1);
		const FileSystemTreeItem::Cell &c = p_item->cells[col];

		if (!p_item->disable_folding && !hide_folding && !p_item->cells[col].editable && !p_item->selectable && p_item->get_first_child()) {
			if (enable_recursive_folding && p_mod->is_shift_pressed()) {
				p_item->set_collapsed_recursive(!p_item->is_collapsed());
			} else {
				p_item->set_collapsed(!p_item->is_collapsed());
			}
			return -1; // Collapse/uncollapse, because nothing can be done with the item.
		}

		bool already_selected = p_item->selected;
		bool already_cursor = (p_item == selected_item);

		if (p_button == MouseButton::LEFT || (p_button == MouseButton::RIGHT && allow_rmb_select)) {
			// Process selection.

			if (p_double_click && (!c.editable || c.mode == FileSystemTreeItem::CELL_MODE_ICON)) {
				// Emits the `item_activated` signal.
				propagate_mouse_activated = true;

				incr_search.clear();
				return -1;
			}

			if (p_item->selectable) {
				if (p_mod->is_command_or_control_pressed()) {
					if (p_item->selected && p_button == MouseButton::LEFT) {
						p_item->deselect();
						emit_signal(SNAME("item_selected"), p_item, false);
					} else {
						p_item->select();
						emit_signal(SNAME("item_selected"), p_item, true);
						emit_signal(SNAME("item_mouse_selected"), get_local_mouse_position(), p_button);
					}
				} else if (p_mod->is_shift_pressed() && selected_item && selected_item != p_item) {
					bool inrange = false;

					select_item(p_item, root, selected_item, &inrange);
					emit_signal(SNAME("item_mouse_selected"), get_local_mouse_position(), p_button);

					queue_redraw();
				} else {
					int icount = _count_selected_items(root);

					if (icount > 1 && p_button != MouseButton::RIGHT) {
						if (!already_selected) {
							select_item(p_item, root);
						}
						single_select_defer = p_item;
					} else {
						if (p_button != MouseButton::RIGHT || !p_item->selected) {
							select_item(p_item, root);
						}

						emit_signal(SNAME("item_mouse_selected"), get_local_mouse_position(), p_button);
					}
					queue_redraw();
				}
			}
		}

		if (!c.editable) {
			return -1; // If cell is not editable, don't bother.
		}

		// Editing.
		bool bring_up_editor = allow_reselect ? (p_item->selected && already_selected) : p_item->selected;
		String editor_text = c.text;

		switch (c.mode) {
			case FileSystemTreeItem::CELL_MODE_STRING: {
				// Nothing in particular.

				if (get_viewport()->get_processed_events_count() == focus_in_id || !already_cursor) {
					bring_up_editor = false;
				}

			} break;
			case FileSystemTreeItem::CELL_MODE_CHECK: {
				bring_up_editor = false; // Checkboxes are not edited with editor.
				if (force_edit_checkbox_only_on_checkbox) {
					if (x < theme_cache.checked->get_width()) {
						p_item->set_checked(col, !c.checked);
						item_edited(col, p_item, p_button);
					}
				} else {
					p_item->set_checked(col, !c.checked);
					item_edited(col, p_item, p_button);
				}
				click_handled = true;

			} break;
			case FileSystemTreeItem::CELL_MODE_ICON: {
				bring_up_editor = false;
			} break;
		};

		if (!bring_up_editor || p_button != MouseButton::LEFT) {
			return -1;
		}

		click_handled = true;
		popup_pressing_edited_item = p_item;
		popup_pressing_edited_item_column = col;
		pressing_for_editor = true;

		return -1; // Select.
	} else {
		Point2i new_pos = p_pos;

		if (!skip) {
			x_ofs += theme_cache.item_margin;
			y_ofs += item_h;
			new_pos.y -= item_h;
		}

		if (!p_item->collapsed) { // If not collapsed, check the children.

			FileSystemTreeItem *c = p_item->first_child;

			while (c) {
				int child_h = propagate_mouse_event(new_pos, x_ofs, y_ofs, x_limit, p_double_click, c, p_button, p_mod);

				if (child_h < 0) {
					return -1; // Break, stop propagating, no need to anymore.
				}

				new_pos.y -= child_h;
				y_ofs += child_h;
				c = c->next;
				item_h += child_h;
			}
		}
		if (p_item == root) {
			emit_signal(SNAME("empty_clicked"), get_local_mouse_position(), p_button);
		}
	}

	return item_h; // Nothing found.
}

void FileSystemTree::_text_editor_popup_modal_close() {
	if (popup_edit_committed) {
		return; // Already processed by `LineEdit` / `TextEdit` commit.
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	if (!popup_edited_item) {
		return;
	}

	if (popup_edited_item->is_edit_multiline(popup_edited_item_col) && popup_edited_item->get_cell_mode(popup_edited_item_col) == FileSystemTreeItem::CELL_MODE_STRING) {
		_apply_multiline_edit();
	} else {
		_line_editor_submit(line_editor->get_text());
	}
}

void FileSystemTree::_text_editor_gui_input(const Ref<InputEvent> &p_event) {
	if (popup_edit_committed) {
		return; // Already processed by `_text_editor_popup_modal_close`.
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	if (p_event->is_action_pressed("ui_text_newline_blank", true)) {
		accept_event();
	} else if (p_event->is_action_pressed("ui_text_newline")) {
		bool hide_focus = !text_editor->has_focus(true);
		popup_edit_committed = true; // End edit popup processing.
		popup_editor->hide();
		_apply_multiline_edit(hide_focus);
		accept_event();
	}
}

void FileSystemTree::_apply_multiline_edit(bool p_hide_focus) {
	if (!popup_edited_item) {
		return;
	}

	if (popup_edited_item_col < 0 || popup_edited_item_col > columns.size()) {
		return;
	}

	FileSystemTreeItem::Cell &c = popup_edited_item->cells.write[popup_edited_item_col];
	switch (c.mode) {
		case FileSystemTreeItem::CELL_MODE_STRING: {
			c.text = text_editor->get_text();
		} break;
		default: {
			ERR_FAIL();
		}
	}

	grab_focus(p_hide_focus);
	item_edited(popup_edited_item_col, popup_edited_item);
	queue_redraw();
}

void FileSystemTree::_line_editor_submit(String p_text) {
	if (popup_edit_committed) {
		return; // Already processed by _text_editor_popup_modal_close.
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	bool hide_focus = !line_editor->has_focus(true);

	popup_edit_committed = true; // End edit popup processing.
	popup_editor->hide();

	if (!popup_edited_item) {
		return;
	}

	if (popup_edited_item_col < 0 || popup_edited_item_col > columns.size()) {
		return;
	}

	FileSystemTreeItem::Cell &c = popup_edited_item->cells.write[popup_edited_item_col];
	switch (c.mode) {
		case FileSystemTreeItem::CELL_MODE_STRING: {
			c.text = p_text;
		} break;
		default: {
			ERR_FAIL();
		}
	}

	grab_focus(hide_focus);
	item_edited(popup_edited_item_col, popup_edited_item);
	queue_redraw();
}

void FileSystemTree::_go_left() {
	if (selected_item->get_first_child() != nullptr && !selected_item->is_collapsed()) {
		selected_item->set_collapsed(true);
	} else {
		if (columns.size() == 1) { // Goto parent with one column.
			FileSystemTreeItem *parent = selected_item->get_parent();
			if (selected_item != get_root() && parent && parent->is_selectable() && !(hide_root && parent == get_root())) {
				select_item(parent, get_root());
			}
		} else if (selected_item->get_prev_visible()) {
			_go_up(); // Go to upper column if possible.
		}
	}
	queue_redraw();
	accept_event();
	ensure_cursor_is_visible();
}

void FileSystemTree::_go_right() {
	if (selected_item->get_first_child() != nullptr && selected_item->is_collapsed()) {
		selected_item->set_collapsed(false);
	} else if (selected_item->get_next_visible()) {
		_go_down();
	}
	queue_redraw();
	ensure_cursor_is_visible();
	accept_event();
}

void FileSystemTree::_go_up(bool p_is_command) {
	FileSystemTreeItem *prev = nullptr;
	if (!selected_item) {
		prev = get_last_item();
	} else {
		prev = selected_item->get_prev_visible();
	}

	if (display_mode == DISPLAY_MODE_TREE) {
		if (!prev) {
			return;
		}

		// selected_item = prev;
		deselect_all();
		prev->select();

		// TODO
		// emit_signal(SNAME("item_selected"), selected_item, true);
		queue_redraw();
	} else {
		while (prev && !prev->selectable) {
			prev = prev->get_prev_visible();
		}
		if (!prev) {
			return; // Do nothing.
		}

		if (p_is_command) {
			selected_item = prev;
			queue_redraw();
		} else {
			// prev->select();
			set_selected(prev);
		}
	}

	ensure_cursor_is_visible();
	accept_event();
}

void FileSystemTree::_shift_select_range(FileSystemTreeItem *new_item) {
	if (!new_item) {
		new_item = selected_item;
	}

	bool in_range = false;
	FileSystemTreeItem *item = root;

	if (!shift_anchor) {
		shift_anchor = selected_item;
	}

	while (item) {
		bool at_range_edge = item == shift_anchor || item == new_item;
		if (at_range_edge) {
			in_range = !in_range;
		}
		if (new_item == shift_anchor) {
			in_range = false;
		}
		if (item->is_visible_in_tree()) {
			if (in_range || at_range_edge) {
				if (!item->is_selected() && item->is_selectable()) {
					item->select();
					emit_signal(SNAME("item_selected"), item, true);
				}
			} else if (item->is_selected()) {
				item->deselect();
				emit_signal(SNAME("item_selected"), item, false);
			}
		}
		item = item->get_next_in_tree(false);
	}

	selected_item = new_item;

	ensure_cursor_is_visible();
	queue_redraw();
	accept_event();
}

void FileSystemTree::_go_down(bool p_is_command) {
	FileSystemTreeItem *next = nullptr;
	if (!selected_item) {
		if (root) {
			next = hide_root ? root->get_next_visible() : root;
		}
	} else {
		next = selected_item->get_next_visible();
	}

	if (display_mode == DISPLAY_MODE_TREE) {
		if (!next) {
			return;
		}

		// selected_item = next;
		deselect_all();
		next->select();

		// TODO
		// emit_signal(SNAME("item_selected"), selected_item, true);
		queue_redraw();
	} else {
		while (next && !next->selectable) {
			next = next->get_next_visible();
		}
		if (!next) {
			return; // Do nothing.
		}

		if (p_is_command) {
			selected_item = next;
			queue_redraw();
		} else {
			// next->select();
			set_selected(next);
		}
	}

	ensure_cursor_is_visible();
	accept_event();
}

bool FileSystemTree::_scroll(bool p_horizontal, float p_pages) {
	ScrollBar *scroll = p_horizontal ? (ScrollBar *)h_scroll : (ScrollBar *)v_scroll;

	double prev_value = scroll->get_value();
	scroll->set_value(scroll->get_value() + scroll->get_page() * p_pages);

	bool scroll_happened = scroll->get_value() != prev_value;
	if (scroll_happened) {
		popup_editor->hide();

		// Update theme_cache.offset first before determining the new hovered item.
		update_scrollbars();

		if (!p_horizontal && drag_type == DRAG_BOX_SELECTION) {
			drag_from.y += prev_value - scroll->get_value();
			_update_selection();
		}

		_determine_hovered_item();
	}
	return scroll_happened;
}

Rect2 FileSystemTree::_get_scrollbar_layout_rect() const {
	const Size2 control_size = get_size();
	const Ref<StyleBox> background = theme_cache.panel_style;

	// This is the background stylebox's content rect.
	const real_t width = control_size.x - background->get_margin(SIDE_LEFT) - background->get_margin(SIDE_RIGHT);
	const real_t height = control_size.y - background->get_margin(SIDE_TOP) - background->get_margin(SIDE_BOTTOM);
	const Rect2 content_rect = Rect2(background->get_offset(), Size2(width, height));

	// Use the stylebox's margins by default. Can be overridden by `scrollbar_margin_*`.
	const real_t top = theme_cache.scrollbar_margin_top < 0 ? content_rect.get_position().y : theme_cache.scrollbar_margin_top;
	const real_t right = theme_cache.scrollbar_margin_right < 0 ? content_rect.get_end().x : (control_size.x - theme_cache.scrollbar_margin_right);
	const real_t bottom = theme_cache.scrollbar_margin_bottom < 0 ? content_rect.get_end().y : (control_size.y - theme_cache.scrollbar_margin_bottom);
	const real_t left = theme_cache.scrollbar_margin_left < 0 ? content_rect.get_position().x : theme_cache.scrollbar_margin_left;

	return Rect2(left, top, right - left, bottom - top);
}

Rect2 FileSystemTree::_get_content_rect() const {
	const Size2 control_size = get_size();
	const Ref<StyleBox> background = theme_cache.panel_style;

	// This is the background stylebox's content rect.
	const real_t width = control_size.x - background->get_margin(SIDE_LEFT) - background->get_margin(SIDE_RIGHT);
	const real_t height = control_size.y - background->get_margin(SIDE_TOP) - background->get_margin(SIDE_BOTTOM);
	const Rect2 content_rect = Rect2(background->get_offset(), Size2(width, height));

	// Scrollbars won't affect FileSystemTree's content rect if they're not visible or placed inside the stylebox margin area.
	const real_t v_size = v_scroll->is_visible() ? (v_scroll->get_combined_minimum_size().x + theme_cache.scrollbar_h_separation) : 0;
	const real_t h_size = h_scroll->is_visible() ? (h_scroll->get_combined_minimum_size().y + theme_cache.scrollbar_v_separation) : 0;
	const Point2 scroll_begin = _get_scrollbar_layout_rect().get_end() - Vector2(v_size, h_size);
	const Size2 offset = (content_rect.get_end() - scroll_begin).maxf(0);

	return content_rect.grow_individual(0, 0, -offset.x, -offset.y);
}

void FileSystemTree::_update_display_mode() {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_hide_root(true);
	set_allow_rmb_select(true);
	set_allow_reselect(true);

	uint32_t column_count = 1;
	if (display_mode == DISPLAY_MODE_TREE) {
		column_count = 1;

		set_theme_type_variation("FileSystemTree");
		set_hide_folding(false);
		set_columns(column_count);
		set_column_titles_visible(false);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		column_count = 0;
		for (uint32_t i = 0; i < column_settings.size(); i++) {
			const ColumnSetting &setting = column_settings[i];
			if (setting.visible) {
				column_count++;
			}
		}
		// print_line("column_count: ", column_count);

		set_theme_type_variation("TreeTable");
		set_hide_folding(true);
		set_columns(column_count);
		set_column_titles_visible(true);
	}

	ERR_FAIL_COND(column_count > column_settings.size());

	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size() && column < column_count; i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		String title = "";
		switch (setting.type) {
			case COLUMN_TYPE_NAME:
				title = RTR("Name");
				break;
			case COLUMN_TYPE_MODIFIED:
				title = RTR("Modified");
				break;
			case COLUMN_TYPE_CREATED:
				title = RTR("Created");
				break;
			case COLUMN_TYPE_TYPE:
				title = RTR("Type");
				break;
			case COLUMN_TYPE_SIZE:
				title = RTR("Size");
				break;
		}

		set_column_title(column, title);
		set_column_title_alignment(column, setting.alignment);
		set_column_clip_content(column, setting.clip);
		if (setting.expand_ratio < 0) {
			set_column_expand(column, false);
		} else {
			set_column_expand(column, true);
			set_column_expand_ratio(column, setting.expand_ratio);
		}
		column++;
	}
}

FileSystemTreeItem *FileSystemTree::_add_tree_item(const FileInfo &p_fi, FileSystemTreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = FileSystemAccess::is_dir_type(p_fi);
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	FileSystemTreeItem *item = create_item(p_parent, p_index);
	String path = p_fi.path;
	item->set_text(0, p_fi.name);
	item->set_icon(0, icon);
	item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);

	// TODO
	// if (da->is_link(path)) {
	// 	item->set_icon_overlay(0, get_app_theme_icon(SNAME("LinkOverlay")));
	// 	// TRANSLATORS: This is a tooltip for a file that is a symbolic link to another file.
	// 	item->set_tooltip_text(0, vformat(RTR("Link to: %s"), da->read_link(path)));
	// }

	const int icon_size = get_theme_constant(SNAME("class_icon_size"));
	item->set_icon_max_width(0, icon_size);

	// TODO
	// Color parent_bg_color = p_parent->get_custom_bg_color(0);
	// if (has_custom_color) {
	// 	item->set_custom_bg_color(0, parent_bg_color.darkened(ITEM_BG_DARK_SCALE));
	// } else if (parent_bg_color != Color()) {
	// 	item->set_custom_bg_color(0, parent_bg_color);
	// }

	// item->set_editable(0, true); // Affects double-click behavior, use force_edit.
	item->set_selectable(true);

	if (!FileSystemAccess::is_dir_type(p_fi.type)) {
		item->collapsed = true; // default value is false.
		item->_changed_notify();
	}

	return item;
}

FileSystemTreeItem *FileSystemTree::_add_list_item(const FileInfo &p_fi, FileSystemTreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = FileSystemAccess::is_dir_type(p_fi);
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	FileSystemTreeItem *item = create_item(p_parent, p_index);
	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size(); i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		switch (setting.type) {
			case COLUMN_TYPE_NAME: {
				item->set_text(column, p_fi.name);
				item->set_icon(column, icon);
				// item->set_editable(column, true); // Affects double-click behavior, use force_edit.
			} break;
			case COLUMN_TYPE_MODIFIED: {
				String modified_time = "";
				if (p_fi.modified_time > 0) {
					modified_time = FileSystem::parse_time(p_fi.modified_time);
				}
				item->set_text(column, modified_time);
				// item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_CREATED: {
				String creation_time = "";
				if (p_fi.creation_time > 0) {
					creation_time = FileSystem::parse_time(p_fi.creation_time);
				}
				item->set_text(column, creation_time);
				// item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_TYPE: {
				String type = p_fi.type;
				if (FileSystemAccess::is_root_type(p_fi)) {
					type = "";
				} else if (FileSystemAccess::is_drive_type(p_fi)) {
					type = ""; // RTR("Disk"); // TODO: Distinguish local disk and network drive.
				} else if (FileSystemAccess::is_dir_type(p_fi)) {
					type = RTR("Folder");
				} else {
					type = type.to_upper();
				}
				item->set_text(column, type);
				// item->set_editable(column, false);
			} break;
			case COLUMN_TYPE_SIZE: {
				String size = "";
				if (!FileSystemAccess::is_dir_type(p_fi)) {
					size = FileSystem::parse_size(p_fi.size);
				}
				item->set_text(column, size);
				item->set_text_alignment(column, HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
				// item->set_editable(column, false);
			} break;
		}
		// item->set_selectable(column, true);

		column++;
	}

	return item;
}

void FileSystemTree::_build_empty_menu() {
	context_menu->clear();

	if (display_mode == DISPLAY_MODE_TREE) {
		// TODO
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_EXPAND_TO_CURRENT);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

		context_menu->add_separator();

		context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW);
		// context_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
		FileContextMenu *new_menu = memnew(FileContextMenu);
		new_menu->set_file_system(context_menu->get_file_system());
		new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
		// new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("Folder")));
		new_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		// new_menu->set_item_icon(-1, get_app_theme_icon(SNAME("File")));
		context_menu->set_item_submenu_node(-1, new_menu);
		new_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

		context_menu->add_separator();

		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
	}
}

void FileSystemTree::_build_file_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY_PATH);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_SHOW_IN_EXPLORER);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void FileSystemTree::_build_folder_menu() {
	context_menu->clear();

	if (display_mode == DISPLAY_MODE_TREE) {
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB); // TODO: create new tab in central area
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

		// context_menu->add_separator();

		// context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_NEW_FOLDER);
		context_menu->add_item(RTR("New TextFile..."), FileContextMenu::FILE_MENU_NEW_TEXTFILE);
		context_menu->add_item(RTR("New Folder..."), FileContextMenu::FILE_MENU_NEW_FOLDER);

		context_menu->add_separator();
	} else if (display_mode == DISPLAY_MODE_LIST) {
		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN);
		context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_TAB);
		// context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_NEW_WINDOW);

		context_menu->add_separator();
	}

	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY_PATH);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_OPEN_IN_TERMINAL);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_SHOW_IN_EXPLORER);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

	context_menu->add_separator();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void FileSystemTree::_build_selected_items_menu() {
	context_menu->clear();

	context_menu->add_file_item(FileContextMenu::FILE_MENU_CUT);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_COPY);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_PASTE);

	context_menu->add_separator();

	// context_menu->add_file_item(FileContextMenu::FILE_MENU_RENAME);
	context_menu->add_file_item(FileContextMenu::FILE_MENU_DELETE);
}

void FileSystemTree::_on_item_mouse_selected(const Vector2 &p_pos, MouseButton p_button) {
	ERR_FAIL_NULL(context_menu);

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	FileSystemTreeItem *cursor_item = get_selected();
	if (!cursor_item) {
		return;
	}

	Vector<String> targets = get_selected_paths();
	context_menu->set_targets(targets);

	if (targets.size() > 1) {
		_build_selected_items_menu();
	} else {
		Dictionary d = cursor_item->get_metadata(0);
		String type = d["type"];
		if (FileSystemAccess::is_root_type(type)) {
			return;
		} else if (FileSystemAccess::is_drive_type(type)) {
			return;
		} else if (FileSystemAccess::is_dir_type(type)) {
			_build_folder_menu();
		} else {
			_build_file_menu();
		}
	}

	if (context_menu->get_item_count() > 0) {
		drag_type = DRAG_NONE;
		prev_selected_item = nullptr;

		context_menu->set_position(get_screen_position() + p_pos);
		context_menu->reset_size();
		context_menu->popup();
	}
}

void FileSystemTree::_on_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	ERR_FAIL_NULL(context_menu);

	deselect_all();

	// TODO
	if (display_mode == DISPLAY_MODE_TREE) {
		return;
	}

	if (p_button != MouseButton::RIGHT) {
		return;
	}

	FileSystemTreeItem *root_item = get_root();
	ERR_FAIL_NULL(root_item);

	Dictionary d = root_item->get_metadata(0);
	if (d.is_empty()) {
		return;
	}
	String type = d.get("type", "");
	String path = d.get("path", "");
	if (path.is_empty() || FileSystemAccess::is_root_type(type)) {
		return;
	}

	Vector<String> targets;
	targets.push_back(path);
	context_menu->set_targets(targets);

	_build_empty_menu();

	if (context_menu->get_item_count() > 0) {
		drag_type = DRAG_NONE;
		prev_selected_item = nullptr;

		context_menu->set_position(get_screen_position() + p_pos);
		context_menu->reset_size();
		context_menu->popup();
	}
}

void FileSystemTree::_on_item_edited() {
	ERR_FAIL_NULL(context_menu);

	FileSystemTreeItem *ti = get_edited();
	int col_index = 0; // TODO
	ERR_FAIL_COND(col_index < 0);

	Dictionary d = ti->get_metadata(0);
	String from = d["path"];
	String new_name = ti->get_text(col_index).strip_edges();
	if (_rename_operation_confirm(from, new_name)) {
		if (file_system) {
			to_select.clear();
			String path = from.get_base_dir();
			to_select.push_back(path.path_join(new_name));

			file_system->scan(path, true);
		}
	} else {
		ti->set_text(0, d["name"]);
	}
}

bool FileSystemTree::_rename_operation_confirm(const String &p_from, const String &p_new_name) {
	bool rename_error = false;

	if (p_new_name.length() == 0) {
		// TODO: hint
		// print_line(RTR("No name provided."));
		rename_error = true;
	} else if (p_new_name.contains_char('/') || p_new_name.contains_char('\\') || p_new_name.contains_char(':')) {
		// print_line(RTR("Name contains invalid characters."));
		rename_error = true;
	} else if (p_new_name[0] == '.') {
		// print_line(RTR("This filename begins with a dot rendering the file invisible to the app.\nIf you want to rename it anyway, use your operating system's file manager."));
		rename_error = true;
	}

	if (rename_error) {
		return false;
	}

	String from = p_from;
	String to = from.get_base_dir().path_join(p_new_name);
	if (to == from) {
		return false;
	}
	return FileSystemAccess::rename(p_from, to) == OK;
}

void FileSystemTree::_on_draw() {
	if (rename_item) {
		FileSystemTreeItem *item = get_selected();
		if (item) {
			item->set_as_cursor();
			if (edit_selected(true)) {
				Dictionary d = item->get_metadata(0);
				String path = d["path"];
				String name = path.get_file();
				if (FileSystemAccess::file_exists(path)) {
					set_editor_selection(0, name.rfind_char('.'));
				} else {
					set_editor_selection(0, name.length());
				}
			}
		}
		rename_item = false;
	}
}

bool FileSystemTree::_process_id_pressed(int p_option, const Vector<String> &p_selected) {
	bool is_handled = true;

	switch (p_option) {
		case FileContextMenu::FILE_MENU_OPEN: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::path_exists(path)) {
				break;
			}

			// TODO: Note the display_mode
			emit_signal(SNAME("item_activated"));
		} break;

		case FileContextMenu::FILE_MENU_PASTE: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (FileSystemAccess::file_exists(path)) {
				path = path.get_base_dir();
			}

			if (!FileSystemAccess::dir_exists(path)) {
				break;
			}

			bool ret = false;
			bool is_cut = false;
			Vector<String> clipboard_paths;
			ret = FileSystemAccess::get_clipboard_paths(clipboard_paths, is_cut);
			if (!ret || clipboard_paths.is_empty()) {
				break;
			}

			Vector<String> dest_paths;
			ret = FileSystemAccess::paste(path, dest_paths);
			// print_line("paste ret: ", ret, is_cut, dest_paths);
			if (ret && file_system) {
				file_system->scan(path, true);

				if (is_cut) {
					// Update the source directory for cut operations.
					Vector<String> dirs;
					for (const String &clipboard_path : clipboard_paths) {
						String base_dir = clipboard_path.get_base_dir();
						if (!dirs.has(base_dir)) {
							dirs.push_back(base_dir);
						}
					}
					// print_line("dirs: ", dirs.size(), dirs);
					file_system->scan(dirs, true);
				}

				if (!dest_paths.is_empty()) {
					to_select.clear();
					to_select = dest_paths;
				}
			}
		} break;

		case FileContextMenu::FILE_MENU_RENAME: {
			String path = p_selected.is_empty() ? "" : p_selected[0];
			if (path.is_empty()) {
				break;
			} else if (FileSystemAccess::dir_exists(path)) {
				FileSystemTreeItem *item = get_selected();
				if (item) {
					item->set_as_cursor();
					grab_focus(!has_focus(true));
					bool result = edit_selected(true);
					String name = path.get_file();
					set_editor_selection(0, name.length());
				}
			} else if (FileSystemAccess::file_exists(path)) {
				FileSystemTreeItem *item = get_selected();
				if (item) {
					item->set_as_cursor();
					grab_focus(!has_focus(true));
					bool result = edit_selected(true);
					String name = path.get_file();
					set_editor_selection(0, name.rfind_char('.'));
				}
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_FOLDER: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new folder");
			// TODO
			String name = new_name;
			String path = dir.path_join(name);
			int i = 1;
			while (FileSystemAccess::dir_exists(path)) {
				name = vformat("%s (%d)", new_name, i);
				path = dir.path_join(name);
				i++;
			}
			Error err = FileSystemAccess::make_dir(path);
			if (err == OK) {
				if (file_system) {
					to_select.clear();
					to_select.push_back(path);
					rename_item = true;

					file_system->scan(dir, true);
				}
			}
		} break;

		case FileContextMenu::FILE_MENU_NEW_TEXTFILE: {
			String dir = p_selected.is_empty() ? "" : p_selected[0];
			// print_line("new text dir: ", dir);
			if (!FileSystemAccess::dir_exists(dir)) {
				break;
			}

			String new_name = RTR("new file");
			String name = new_name + ".txt";
			String path = dir.path_join(name);
			// print_line("path: ", new_name, path, FileSystemAccess::file_exists(path));
			int i = 1;
			while (FileSystemAccess::file_exists(path)) {
				name = vformat("%s (%d).txt", new_name, i);
				path = dir.path_join(name);
				// print_line("i: ", i, name, path);
				i++;
			}
			Error err = FileSystemAccess::create_file(dir, name);
			// print_line("create file: ", err);
			if (err == OK) {
				to_select.clear();
				to_select.push_back(path);
				rename_item = true;

				if (file_system) {
					file_system->scan(dir, true);
				}
			}
		} break;

		default: {
			is_handled = false;
		}
	}

	return is_handled;
}

void FileSystemTree::_context_menu_id_pressed(int p_option) {
	_process_id_pressed(p_option, context_menu->get_targets());
}

void FileSystemTree::_draw_selection() {
	if (drag_type == DRAG_BOX_SELECTION) {
		// print_line("draw: ", drag_type, drag_from, box_selecting_to);

		// Draw the dragging box
		Point2 bsfrom = drag_from;
		Point2 bsto = box_selecting_to;

		RenderingServer *rs = RenderingServer::get_singleton();
		rs->canvas_item_add_rect(content_ci,
				Rect2(bsfrom, bsto - bsfrom),
				get_theme_color(SNAME("box_selection_fill_color"), AppStringName(App)));
		// Top.
		rs->canvas_item_add_line(content_ci,
				bsfrom,
				Point2(bsto.x, bsfrom.y),
				get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
				Math::round(APP_SCALE));
		// Bottom.
		rs->canvas_item_add_line(content_ci,
				bsto,
				Point2(bsfrom.x, bsto.y),
				get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
				Math::round(APP_SCALE));
		// Left.
		rs->canvas_item_add_line(content_ci,
				bsfrom,
				Point2(bsfrom.x, bsto.y),
				get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
				Math::round(APP_SCALE));
		// Right.
		rs->canvas_item_add_line(content_ci,
				bsto,
				Point2(bsto.x, bsfrom.y),
				get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
				Math::round(APP_SCALE));

		// draw_rect(
		// 		Rect2(bsfrom, bsto - bsfrom),
		// 		get_theme_color(SNAME("box_selection_fill_color"), AppStringName(App)));

		// draw_rect(
		// 		Rect2(bsfrom, bsto - bsfrom),
		// 		get_theme_color(SNAME("box_selection_stroke_color"), AppStringName(App)),
		// 		false,
		// 		Math::round(APP_SCALE));
	}
}

void FileSystemTree::_update_selection() {
	ERR_FAIL_COND(drag_type != DRAG_BOX_SELECTION);

	// Rect2 box = Rect2(drag_from, box_selecting_to - drag_from);
	FileSystemTreeItem *last_item = _get_last_selectable();
	Rect2 last_rect = get_item_rect(last_item);
	real_t min_pos_y = _get_title_button_height();
	real_t max_pos_y = MIN(get_size().y - theme_cache.v_separation, last_rect.position.y + last_rect.size.y - theme_cache.v_separation);
	// print_line("range: ", min_pos_y, max_pos_y, get_size().y, last_rect.position.y + last_rect.size.y);

	Point2 selecting_to = Point2(0, CLAMP((real_t)box_selecting_to.y, min_pos_y, max_pos_y));
	FileSystemTreeItem *current_item = nullptr;
	if (box_selecting_to.y < min_pos_y && drag_from.y < min_pos_y ||
			box_selecting_to.y > max_pos_y && drag_from.y > max_pos_y) {
		// print_line("out of range: ", box_selecting_to.y, drag_from.y);
	} else {
		current_item = get_item_at_position(selecting_to);
	}

	// print_line("box selection: ", drag_from, selecting_to, drag_from_item, current_item);
	if (!drag_from_item && !current_item) {
		// bool has_selection = false;
		// if (selected_item) {
		// 	Rect2 rect = get_item_rect(selected_item);
		// 	real_t pos_y = rect.position.y + rect.size.y;
		// 	if (drag_from.y <= pos_y && box_selecting_to.y >= pos_y ||
		// 			drag_from.y >= pos_y && box_selecting_to.y <= pos_y) {
		// 		has_selection = true;
		// 		print_line("has selection: ", has_selection, pos_y);
		// 	}
		// }
		// if (!has_selection) {
		if (selected_item) {
			emit_signal(SNAME("item_selected"), selected_item, false);
		}
		deselect_all();
		// }
	} else if (current_item) {
		// if (current_item->selectable) {
		bool inrange = false;

		select_item(current_item, root, drag_from_item, &inrange);
		selected_item = drag_from_item ? drag_from_item : current_item;

		// emit_signal(SNAME("item_mouse_selected"), get_local_mouse_position(), p_button);

		// }
	}
	queue_redraw();
}

FileSystemTreeItem *FileSystemTree::_get_last_selectable() {
	FileSystemTreeItem *end = root;
	int child_count = end->get_visible_child_count();
	while (end && !end->is_collapsed() && child_count > 0) {
		// Get last child.
		end = end->get_child(child_count - 1);
		child_count = end->get_visible_child_count();
	}
	if (end == root && is_root_hidden()) {
		return nullptr;
	}
	while (end && !end->selectable) {
		FileSystemTreeItem *prev = end->get_prev_visible();
		if (prev) {
			end = prev;
		} else {
			end = end->get_parent();
		}
	}
	return end;
}

Vector<FileSystemTreeItem *> FileSystemTree::_get_selected_items() {
	Vector<FileSystemTreeItem *> items;

	FileSystemTreeItem *cursor_item = get_selected();
	if (cursor_item) {
		items.push_back(cursor_item);
	}

	FileSystemTreeItem *selected = get_root();
	selected = get_next_selected(selected);
	while (selected) {
		if (selected != cursor_item && selected->is_visible_in_tree()) {
			items.push_back(selected);
		}
		selected = get_next_selected(selected);
	}

	return items;
}

void FileSystemTree::_key_input_input(const Ref<InputEventKey> &p_event) {
	Ref<InputEventKey> k = p_event;

	if (k.is_valid() && k->get_keycode() == Key::SHIFT && !k->is_pressed()) {
		shift_anchor = nullptr;
	}

	bool is_command = k.is_valid() && k->is_command_or_control_pressed();
	if (p_event->is_action(cache.rtl ? "ui_left" : "ui_right") && p_event->is_pressed()) {
		// For shortcuts with Alt (e.g. Alt+Left/Right for back/forward in Explorer).
		if (k->is_alt_pressed()) {
			return;
		}

		if (!cursor_can_exit_tree) {
			accept_event();
		}

		if (!selected_item) {
			return;
		}

		if (k.is_valid() && k->is_shift_pressed()) {
			selected_item->set_collapsed_recursive(false);
		} else if (display_mode == DISPLAY_MODE_TREE) {
			_go_right();
		} else if (selected_item->get_first_child() != nullptr && selected_item->is_collapsed()) {
			selected_item->set_collapsed(false);
		} else {
			_go_down();
		}
	} else if (p_event->is_action(cache.rtl ? "ui_right" : "ui_left") && p_event->is_pressed()) {
		// For shortcuts with Alt (e.g. Alt+Left/Right for back/forward in Explorer).
		if (k->is_alt_pressed()) {
			return;
		}

		if (!cursor_can_exit_tree) {
			accept_event();
		}

		if (!selected_item) {
			return;
		}

		if (k.is_valid() && k->is_shift_pressed()) {
			selected_item->set_collapsed_recursive(true);
		} else if (display_mode == DISPLAY_MODE_TREE) {
			_go_left();
		} else if (selected_item->get_first_child() != nullptr && !selected_item->is_collapsed()) {
			selected_item->set_collapsed(true);
		} else {
			_go_up();
		}
	} else if (p_event->is_action("ui_up") && p_event->is_pressed()) {
		// For shortcuts with Alt (e.g. Alt+Left/Right for back/forward in Explorer).
		if (k->is_alt_pressed()) {
			return;
		}

		if (!cursor_can_exit_tree) {
			accept_event();
		}
		// Shift Up Selection.
		if (k.is_valid() && k->is_shift_pressed() && selected_item) {
			FileSystemTreeItem *new_item = selected_item->get_prev_visible(false);
			_shift_select_range(new_item);
		} else {
			_go_up(is_command);
		}
	} else if (p_event->is_action("ui_down") && p_event->is_pressed()) {
		// For shortcuts with Alt (e.g. Alt+Left/Right for back/forward in Explorer).
		if (k->is_alt_pressed()) {
			return;
		}

		if (!cursor_can_exit_tree) {
			accept_event();
		}
		// Shift Down Selection.
		if (k.is_valid() && k->is_shift_pressed() && selected_item) {
			FileSystemTreeItem *new_item = selected_item->get_next_visible(false);
			_shift_select_range(new_item);
		} else {
			_go_down(is_command);
		}
	} else if (p_event->is_action("ui_menu") && p_event->is_pressed()) {
		if (allow_rmb_select && selected_item) {
			emit_signal(SNAME("item_mouse_selected"), get_item_rect(selected_item).position, MouseButton::RIGHT);
		}

		accept_event();
	} else if (p_event->is_action("ui_page_down") && p_event->is_pressed()) {
		if (!cursor_can_exit_tree) {
			accept_event();
		}

		FileSystemTreeItem *next = nullptr;
		if (!selected_item) {
			return;
		}
		next = selected_item;

		for (int i = 0; i < 10; i++) { // TODO
			FileSystemTreeItem *_n = next->get_next_visible();
			if (_n) {
				next = _n;
			} else {
				break;
			}
		}
		if (next == selected_item) {
			return;
		}

		if (display_mode == DISPLAY_MODE_TREE) {
			// selected_item = next;
			deselect_all();
			next->select();

			// TODO
			// emit_signal(SNAME("item_selected"), selected_item, true);
			queue_redraw();
		} else {
			while (next && !next->selectable) {
				next = next->get_next_visible();
			}
			if (!next) {
				return; // Do nothing.
			}
			// next->select();
			set_selected(next);
		}

		ensure_cursor_is_visible();
	} else if (p_event->is_action("ui_page_up") && p_event->is_pressed()) {
		if (!cursor_can_exit_tree) {
			accept_event();
		}

		FileSystemTreeItem *prev = nullptr;
		if (!selected_item) {
			return;
		}
		prev = selected_item;

		for (int i = 0; i < 10; i++) { // TODO
			FileSystemTreeItem *_n = prev->get_prev_visible();
			if (_n) {
				prev = _n;
			} else {
				break;
			}
		}
		if (prev == selected_item) {
			return;
		}

		if (display_mode == DISPLAY_MODE_TREE) {
			// selected_item = prev;
			deselect_all();
			prev->select();

			// TODO
			// emit_signal(SNAME("item_selected"), selected_item, true);
			queue_redraw();
		} else {
			while (prev && !prev->selectable) {
				prev = prev->get_prev_visible();
			}
			if (!prev) {
				return; // Do nothing.
			}
			// prev->select();
			set_selected(prev);
		}

		ensure_cursor_is_visible();
	}
	// else if (p_event->get_keycode() == Key::HOME && p_event->is_pressed()) {
	else if (p_event->is_action("ui_home") && p_event->is_pressed()) {
		if (!cursor_can_exit_tree) {
			accept_event();
		}

		if (!root) {
			return;
		}

		FileSystemTreeItem *home = root;
		if (is_root_hidden() || !home->selectable) {
			home = root->get_next_visible();
			while (home && !home->selectable) {
				home = home->get_next_visible();
			}
		}
		if (!home) {
			return;
		}

		if (display_mode == DISPLAY_MODE_TREE) {
			// selected_item = home;
			deselect_all();
			home->select();

			// TODO
			// emit_signal(SNAME("item_selected"), selected_item, true);
			queue_redraw();
		} else {
			set_selected(home);
		}

		ensure_cursor_is_visible();
	} else if (p_event->is_action("ui_end") && p_event->is_pressed()) {
		if (!cursor_can_exit_tree) {
			accept_event();
		}

		if (!root) {
			return;
		}

		FileSystemTreeItem *end = _get_last_selectable();
		if (!end) {
			return;
		}

		if (display_mode == DISPLAY_MODE_TREE) {
			// selected_item = end;
			deselect_all();
			end->select();

			// TODO
			// emit_signal(SNAME("item_selected"), selected_item, true);
			queue_redraw();
		} else {
			set_selected(end);
		}

		ensure_cursor_is_visible();
	} else if (p_event->is_action("ui_select") && p_event->is_pressed()) {
		if (!selected_item) {
			return;
		}
		// if (selected_item->is_selected()) {
		// 	selected_item->deselect();
		// 	emit_signal(SNAME("item_selected"), selected_item, false);
		// } else if (selected_item->is_selectable()) {
		// 	selected_item->select();
		// 	emit_signal(SNAME("item_selected"), selected_item, true);
		// }
		if (!selected_item->is_selected()) {
			if (display_mode == DISPLAY_MODE_TREE) {
				set_selected(selected_item);
			} else {
				selected_item->select();
				emit_signal(SNAME("item_selected"), selected_item, true);
			}
		}

		accept_event();
	} else if (p_event->is_action("ui_accept") && p_event->is_pressed()) {
		if (selected_item) {
			// Bring up editor if possible.
			if (!edit_selected()) {
				emit_signal(SNAME("item_activated"));
				incr_search.clear();
			}
		}
		accept_event();
	}

	if (allow_search && k.is_valid()) { // Incremental search.
		if (!k->is_pressed()) {
			return;
		}
		if (k->is_command_or_control_pressed() || (k->is_shift_pressed() && k->get_unicode() == 0) || k->is_meta_pressed()) {
			return;
		}
		if (!root) {
			return;
		}

		if (hide_root && !root->get_next_visible()) {
			return;
		}

		if (k->get_unicode() > 0) {
			_do_incr_search(String::chr(k->get_unicode()));
			accept_event();

			return;
		} else {
			if (k->get_keycode() != Key::SHIFT) {
				last_keypress = 0;
			}
		}
	}
}

void FileSystemTree::_mouse_motion_input(const Ref<InputEventMouseMotion> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;

	hovered_pos = mm->get_position();

	Point2 click = hovered_pos;
	// print_line("pos: ", click, drag_type, drag_from, click.distance_to(drag_from), DRAG_THRESHOLD);
	if (drag_type == DRAG_DETECTING) {
		if (click.distance_to(drag_from) > DRAG_THRESHOLD) {
			// Start a box selection.
			if (!(mm->is_shift_pressed() || mm->is_command_or_control_pressed())) {
				// Clear the selection if not additive.
				deselect_all();
				queue_redraw();
			};

			drag_type = DRAG_BOX_SELECTION;
			box_selecting_to = drag_from;
		}
	} else if (drag_type == DRAG_BOX_SELECTION) {
		// Update box selection.
		box_selecting_to = click;

		_update_selection();
	}

	_determine_hovered_item();
}

void FileSystemTree::_mouse_button_input(const Ref<InputEventMouseButton> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	bool rtl = is_layout_rtl();

	if (!mb->is_pressed()) {
		if (drag_type == DRAG_BOX_SELECTION) {
			if (mb->get_button_index() == MouseButton::LEFT ||
					mb->get_button_index() == MouseButton::RIGHT) {
				// End box selection.
				drag_type = DRAG_NONE;
				prev_selected_item = nullptr;

				if (mb->get_button_index() == MouseButton::RIGHT) {
					_on_item_mouse_selected(mb->get_position(), mb->get_button_index());
				}
			}
		} else {
			if (mb->get_button_index() == MouseButton::LEFT ||
					mb->get_button_index() == MouseButton::RIGHT) {
				// if (drag_type == DRAG_DETECTING) {
				drag_type = DRAG_NONE;
				prev_selected_item = nullptr;
				// }

				Point2 pos = mb->get_position();
				if (rtl) {
					pos.x = get_size().width - pos.x - 1;
				}
				pos -= theme_cache.panel_style->get_offset();
				if (show_column_titles) {
					pos.y -= _get_title_button_height();

					if (pos.y < 0) {
						pos.x += theme_cache.offset.x;
						int len = 0;
						for (int i = 0; i < columns.size(); i++) {
							len += get_column_width(i);
							if (pos.x < static_cast<real_t>(len)) {
								_column_title_clicked(i, mb->get_button_index());
								break;
							}
						}
					}
				}
			}

			if (mb->get_button_index() == MouseButton::LEFT) {
				if (single_select_defer) {
					select_item(single_select_defer, root);
					single_select_defer = nullptr;
				}

				if (pressing_for_editor) {
					if (range_drag_enabled) {
						range_drag_enabled = false;
						Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
						warp_mouse(range_drag_capture_pos);
					} else {
						Rect2 rect = _get_item_focus_rect(get_selected());
						Point2 mpos = mb->get_position();
						int icon_size_x = 0;
						Ref<Texture2D> icon = get_selected()->get_icon(0); // TODO
						if (icon.is_valid()) {
							icon_size_x = _get_cell_icon_size(get_selected()->cells[0]).x;
						}
						// Icon is treated as if it is outside of the rect so that double clicking on it will emit the `item_icon_double_clicked` signal.
						if (rtl) {
							mpos.x = get_size().width - (mpos.x + icon_size_x);
						} else {
							mpos.x -= icon_size_x;
						}
						if (rect.has_point(mpos)) {
							if (!edit_selected()) {
								emit_signal(SNAME("item_icon_double_clicked"));
							}
						} else {
							emit_signal(SNAME("item_icon_double_clicked"));
						}
					}
					pressing_for_editor = false;
				}
			}
		}

		cache.click_type = Cache::CLICK_NONE;
		cache.click_index = -1;
		cache.click_id = -1;
		cache.click_item = nullptr;
		cache.click_column = 0;
		queue_redraw();
		return;
	}

	if (range_drag_enabled) {
		return;
	}

	switch (mb->get_button_index()) {
		case MouseButton::RIGHT:
		case MouseButton::LEFT: {
			Ref<StyleBox> bg = theme_cache.panel_style;

			Point2 pos = mb->get_position();
			if (rtl) {
				pos.x = get_size().width - pos.x - 1;
			}
			pos -= bg->get_offset();
			cache.click_type = Cache::CLICK_NONE;
			if (show_column_titles) {
				pos.y -= _get_title_button_height();

				if (pos.y < 0) {
					pos.x += theme_cache.offset.x;
					int len = 0;
					for (int i = 0; i < columns.size(); i++) {
						len += get_column_width(i);
						if (pos.x < static_cast<real_t>(len)) {
							cache.click_type = Cache::CLICK_TITLE;
							cache.click_index = i;
							queue_redraw();
							break;
						}
					}
					break;
				}
			}

			if (!root || (!root->get_first_child() && hide_root)) {
				emit_signal(SNAME("empty_clicked"), get_local_mouse_position(), mb->get_button_index());
				break;
			}

			click_handled = false;
			pressing_for_editor = false;
			propagate_mouse_activated = false;

			int x_limit = _get_content_rect().size.x;
			bool double_click = mb->is_double_click();

			cache.rtl = is_layout_rtl();
			blocked++;
			if (drag_type == DRAG_NONE) {
				if (!double_click) {
					drag_type = DRAG_DETECTING;
					drag_from = mb->get_position();
					drag_from_item = get_item_at_position(drag_from);
					prev_selected_item = selected_item;
				}
			}
			propagate_mouse_event(pos + theme_cache.offset, 0, 0, x_limit + theme_cache.offset.width, double_click, root, mb->get_button_index(), mb);
			blocked--;

			if (pressing_for_editor) {
				pressing_pos = mb->get_position();
				if (rtl) {
					pressing_pos.x = get_size().width - pressing_pos.x;
				}
			} else if (double_click && get_item_at_position(mb->get_position()) != nullptr) {
				emit_signal(SNAME("item_icon_double_clicked"));
			}

			if (mb->get_button_index() == MouseButton::RIGHT) {
				break;
			}

			if (!click_handled) {
				if (mb->get_button_index() == MouseButton::LEFT) {
					if (get_item_at_position(mb->get_position()) == nullptr && !mb->is_shift_pressed() && !mb->is_command_or_control_pressed()) {
						emit_signal(SNAME("nothing_selected"));
					}
				}
			}

			if (propagate_mouse_activated) {
				emit_signal(SNAME("item_activated"));
				propagate_mouse_activated = false;
			}

		} break;
		case MouseButton::WHEEL_UP: {
			if (_scroll(mb->is_shift_pressed(), -mb->get_factor() / 8)) {
				accept_event();
			}

		} break;
		case MouseButton::WHEEL_DOWN: {
			if (_scroll(mb->is_shift_pressed(), mb->get_factor() / 8)) {
				accept_event();
			}

		} break;
		case MouseButton::WHEEL_LEFT: {
			if (_scroll(!mb->is_shift_pressed(), -mb->get_factor() / 8)) {
				accept_event();
			}

		} break;
		case MouseButton::WHEEL_RIGHT: {
			if (_scroll(!mb->is_shift_pressed(), mb->get_factor() / 8)) {
				accept_event();
			}

		} break;
		default:
			break;
	}
}

void FileSystemTree::_pan_gesture_input(const Ref<InputEventPanGesture> &p_event) {
	Ref<InputEventPanGesture> pan_gesture = p_event;
	double prev_v = v_scroll->get_value();
	v_scroll->set_value(v_scroll->get_value() + v_scroll->get_page() * pan_gesture->get_delta().y / 8);

	double prev_h = h_scroll->get_value();
	if (is_layout_rtl()) {
		h_scroll->set_value(h_scroll->get_value() + h_scroll->get_page() * -pan_gesture->get_delta().x / 8);
	} else {
		h_scroll->set_value(h_scroll->get_value() + h_scroll->get_page() * pan_gesture->get_delta().x / 8);
	}

	if (v_scroll->get_value() != prev_v || h_scroll->get_value() != prev_h) {
		accept_event();
	}
}

void FileSystemTree::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		_key_input_input(k);
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		_mouse_motion_input(mm);
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		_mouse_button_input(mb);
		return;
	}

	Ref<InputEventPanGesture> pan_gesture = p_event;
	if (pan_gesture.is_valid()) {
		_pan_gesture_input(pan_gesture);
		return;
	}
}

void FileSystemTree::_determine_hovered_item() {
	hovered_update_queued = false;

	Ref<StyleBox> bg = theme_cache.panel_style;
	bool rtl = is_layout_rtl();

	Point2 pos = hovered_pos;
	if (rtl) {
		pos.x = get_size().width - pos.x - 1;
	}
	pos -= theme_cache.panel_style->get_offset();

	bool old_header_row = cache.hover_header_row;
	int old_header_column = cache.hover_header_column;
	FileSystemTreeItem *old_item = cache.hover_item;
	int old_column = cache.hover_column;

	// Determine hover on column headers.
	cache.hover_header_row = false;
	cache.hover_header_column = 0;
	if (show_column_titles && is_mouse_hovering) {
		pos.y -= _get_title_button_height();
		if (pos.y < 0) {
			pos.x += theme_cache.offset.x;
			int len = 0;
			for (int i = 0; i < columns.size(); i++) {
				len += get_column_width(i);
				if (pos.x < len) {
					cache.hover_header_row = true;
					cache.hover_header_column = i;
					break;
				}
			}
		}
	}

	// Determine hover on rows and items.
	if (root && is_mouse_hovering && hovered_pos.y >= 0) {
		FileSystemTreeItem *it;
		int col, section, col_button_index;
		_find_button_at_pos(hovered_pos, it, col, col_button_index, section);

		if (drop_mode_flags) {
			if (it != drop_mode_over) {
				drop_mode_over = it;
				if (enable_drag_unfolding) {
					dropping_unfold_timer->start(theme_cache.dragging_unfold_wait_msec * 0.001);
				}
				queue_redraw();
			}
			if (it && section != drop_mode_section) {
				drop_mode_section = section;
				queue_redraw();
			}
		}

		cache.hover_item = it;
		cache.hover_column = col;

		if (it != old_item || col != old_column) {
			if (old_item && old_column >= old_item->cells.size()) {
				// Columns may have changed since last `redraw()`.
				queue_redraw();
			}
		}
	}

	// Mouse has moved from row to row.
	bool item_hover_needs_redraw = !cache.hover_header_row && cache.hover_item != old_item;
	// Mouse has moved between two different column header sections.
	bool header_hover_needs_redraw = cache.hover_header_row && cache.hover_header_column != old_header_column;
	// Mouse has moved between header and "main" areas.
	bool whole_needs_redraw = cache.hover_header_row != old_header_row;

	if (whole_needs_redraw || header_hover_needs_redraw || item_hover_needs_redraw) {
		queue_redraw();
	}
}

void FileSystemTree::_on_dropping_unfold_timer_timeout() {
	if (enable_drag_unfolding && drop_mode_over && drop_mode_section == 0) {
		drop_mode_over->set_collapsed(false);
	}
}

void FileSystemTree::_reset_drop_mode_over() {
	drop_mode_over = nullptr;
	if (!dropping_unfold_timer->is_stopped()) {
		dropping_unfold_timer->stop();
	}
}

void FileSystemTree::_queue_update_hovered_item() {
	if (hovered_update_queued) {
		return;
	}
	hovered_update_queued = true;
	callable_mp(this, &FileSystemTree::_determine_hovered_item).call_deferred();
}

bool FileSystemTree::edit_selected(bool p_force_edit) {
	FileSystemTreeItem *s = get_selected();
	ERR_FAIL_NULL_V_MSG(s, false, "No item selected.");
	ensure_cursor_is_visible();
	int col = 0; // TODO
	ERR_FAIL_INDEX_V_MSG(col, columns.size(), false, "No item column selected.");

	if (!s->cells[col].editable && !p_force_edit) {
		return false;
	}

	Size2 scale = popup_editor->get_parent_viewport()->get_popup_base_transform().get_scale() * get_global_transform_with_canvas().get_scale();
	real_t popup_scale = MIN(scale.x, scale.y);

	Rect2 rect = _get_item_focus_rect(s);
	rect.position *= popup_scale;
	popup_edited_item = s;
	popup_edited_item_col = col;

	const FileSystemTreeItem::Cell &c = s->cells[col];

	if (c.mode == FileSystemTreeItem::CELL_MODE_CHECK) {
		s->set_checked(col, !c.checked);
		item_edited(col, s);
		return true;
	} else if (c.mode == FileSystemTreeItem::CELL_MODE_STRING && !c.edit_multiline) {
		Rect2 popup_rect;
		// `floor()` centers vertically.
		Vector2 ofs(0, Math::floor((MAX(line_editor->get_minimum_size().height, rect.size.height) - rect.size.height) / 2));

		// Account for icon.
		real_t icon_ofs = 0;
		if (c.icon.is_valid()) {
			icon_ofs = _get_cell_icon_size(c).x * popup_scale + theme_cache.icon_h_separation;
		}

		popup_rect.size = rect.size + Vector2(-icon_ofs, 0);

		popup_rect.position = rect.position - ofs;
		popup_rect.position.x += icon_ofs;
		if (cache.rtl) {
			popup_rect.position.x = get_size().width - popup_rect.position.x - popup_rect.size.x;
		}
		popup_rect.position += get_screen_position();

		bool hide_focus = !has_focus(true);

		line_editor->clear();
		line_editor->set_text(c.text);
		line_editor->select_all();
		line_editor->show();

		text_editor->hide();

		popup_editor->set_position(popup_rect.position);
		popup_editor->set_size(popup_rect.size * popup_scale);
		if (!popup_editor->is_embedded()) {
			popup_editor->set_content_scale_factor(popup_scale);
		}
		popup_edit_committed = false; // Start edit popup processing.
		popup_editor->popup();
		popup_editor->child_controls_changed();

		line_editor->grab_focus(hide_focus);

		return true;
	} else if (c.mode == FileSystemTreeItem::CELL_MODE_STRING && c.edit_multiline) {
		bool hide_focus = !has_focus(true);

		line_editor->hide();

		text_editor->clear();
		text_editor->set_text(c.text);
		text_editor->select_all();
		text_editor->show();

		popup_editor->set_position(get_screen_position() + rect.position);
		popup_editor->set_size(rect.size * popup_scale);
		if (!popup_editor->is_embedded()) {
			popup_editor->set_content_scale_factor(popup_scale);
		}
		popup_edit_committed = false; // Start edit popup processing.
		popup_editor->popup();
		popup_editor->child_controls_changed();

		text_editor->grab_focus(hide_focus);

		return true;
	}

	return false;
}

Rect2 FileSystemTree::_get_item_focus_rect(const FileSystemTreeItem *p_item) const {
	ERR_FAIL_NULL_V(p_item, Rect2());
	return p_item->focus_rect;
}

bool FileSystemTree::is_editing() {
	return popup_editor->is_visible();
}

void FileSystemTree::set_editor_selection(int p_from_line, int p_to_line, int p_from_column, int p_to_column, int p_caret) {
	if (p_from_column == -1 || p_to_column == -1) {
		line_editor->select(p_from_line, p_to_line);
	} else {
		text_editor->select(p_from_line, p_from_column, p_to_line, p_to_column, p_caret);
	}
}

Size2 FileSystemTree::get_internal_min_size() const {
	Size2i size;
	if (root) {
		size.height += get_item_height(root);
	}
	for (int i = 0; i < columns.size(); i++) {
		size.width += get_column_minimum_width(i);
	}

	return size;
}

void FileSystemTree::update_scrollbars() {
	const Size2 control_size = get_size();
	const Ref<StyleBox> background = theme_cache.panel_style;

	// This is the background stylebox's content rect.
	const real_t width = control_size.x - background->get_margin(SIDE_LEFT) - background->get_margin(SIDE_RIGHT);
	const real_t height = control_size.y - background->get_margin(SIDE_TOP) - background->get_margin(SIDE_BOTTOM);
	const Rect2 content_rect = Rect2(background->get_offset(), Size2(width, height));

	const Size2 hmin = h_scroll->get_combined_minimum_size();
	const Size2 vmin = v_scroll->get_combined_minimum_size();

	const Size2 internal_min_size = get_internal_min_size();
	const int title_button_height = _get_title_button_height();

	Size2 tree_content_size = content_rect.get_size() - Vector2(0, title_button_height);
	bool display_vscroll = internal_min_size.height > tree_content_size.height;
	bool display_hscroll = internal_min_size.width > tree_content_size.width;
	for (int i = 0; i < 2; i++) {
		// Check twice, as both values are dependent on each other.
		if (display_hscroll) {
			tree_content_size.height = content_rect.get_size().height - title_button_height - hmin.height;
			display_vscroll = internal_min_size.height > tree_content_size.height;
		}
		if (display_vscroll) {
			tree_content_size.width = content_rect.get_size().width - vmin.width;
			display_hscroll = internal_min_size.width > tree_content_size.width;
		}
	}

	if (display_vscroll) {
		v_scroll->show();
		v_scroll->set_max(internal_min_size.height);
		v_scroll->set_page(tree_content_size.height);
		theme_cache.offset.y = v_scroll->get_value();
	} else {
		v_scroll->hide();
		v_scroll->set_value(0);
		theme_cache.offset.y = 0;
	}

	if (display_hscroll) {
		h_scroll->show();
		h_scroll->set_max(internal_min_size.width);
		h_scroll->set_page(tree_content_size.width);
		theme_cache.offset.x = h_scroll->get_value();
	} else {
		h_scroll->hide();
		h_scroll->set_value(0);
		theme_cache.offset.x = 0;
	}

	const Rect2 scroll_rect = _get_scrollbar_layout_rect();
	v_scroll->set_begin(scroll_rect.get_position() + Vector2(scroll_rect.get_size().x - vmin.width, 0));
	v_scroll->set_end(scroll_rect.get_end() - Vector2(0, display_hscroll ? hmin.height : 0));
	h_scroll->set_begin(scroll_rect.get_position() + Vector2(0, scroll_rect.get_size().y - hmin.height));
	h_scroll->set_end(scroll_rect.get_end() - Vector2(display_vscroll ? vmin.width : 0, 0));
}

int FileSystemTree::_get_title_button_height() const {
	ERR_FAIL_COND_V(theme_cache.tb_font.is_null() || theme_cache.title_button.is_null(), 0);
	int h = 0;
	if (show_column_titles) {
		for (int i = 0; i < columns.size(); i++) {
			h = MAX(h, columns[i].text_buf->get_size().y + theme_cache.title_button->get_minimum_size().height);
		}
	}
	return h;
}

void FileSystemTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_FOCUS_ENTER: {
			if (get_viewport()) {
				focus_in_id = get_viewport()->get_processed_events_count();
			}
		} break;

		case NOTIFICATION_MOUSE_ENTER: {
			is_mouse_hovering = true;
			_determine_hovered_item();
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			is_mouse_hovering = false;
			// Clear hovered item cache.
			if (cache.hover_header_row || cache.hover_item != nullptr) {
				cache.hover_header_row = false;
				cache.hover_header_column = -1;
				cache.hover_item = nullptr;
				cache.hover_column = -1;
				queue_redraw();
			}
		} break;

		case NOTIFICATION_DRAG_END: {
			drag_type = DRAG_NONE;

			set_drop_mode_flags(DROP_MODE_DISABLED);

			scrolling = false;
			set_process_internal(false);
			queue_redraw();
		} break;

		case NOTIFICATION_DRAG_BEGIN: {
			drag_type = DRAG_DROPPING;

			Dictionary dd = get_viewport()->gui_get_drag_data();
			if (is_visible_in_tree() && dd.has("type")) {
				if ((String(dd["type"]) == "files") || (String(dd["type"]) == "files_and_dirs")) {
					// set_drop_mode_flags(DROP_MODE_ON_ITEM | DROP_MODE_INBETWEEN);
					set_drop_mode_flags(DROP_MODE_ON_ITEM); // TODO: Remove?
				}
			}

			single_select_defer = nullptr;
			if (theme_cache.scroll_speed > 0) {
				scrolling = true;
				set_process_internal(true);
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			Point2 mouse_position = get_viewport()->get_mouse_position() - get_global_position();
			if (scrolling && get_rect().grow(theme_cache.scroll_border).has_point(mouse_position)) {
				Point2 point;

				if ((Math::abs(mouse_position.x) < Math::abs(mouse_position.x - get_size().width)) && (Math::abs(mouse_position.x) < theme_cache.scroll_border)) {
					point.x = mouse_position.x - theme_cache.scroll_border;
				} else if (Math::abs(mouse_position.x - get_size().width) < theme_cache.scroll_border) {
					point.x = mouse_position.x - (get_size().width - theme_cache.scroll_border);
				}

				if ((Math::abs(mouse_position.y) < Math::abs(mouse_position.y - get_size().height)) && (Math::abs(mouse_position.y) < theme_cache.scroll_border)) {
					point.y = mouse_position.y - theme_cache.scroll_border;
				} else if (Math::abs(mouse_position.y - get_size().height) < theme_cache.scroll_border) {
					point.y = mouse_position.y - (get_size().height - theme_cache.scroll_border);
				}

				point *= theme_cache.scroll_speed * get_process_delta_time();
				point += get_scroll();
				h_scroll->set_value(point.x);
				v_scroll->set_value(point.y);
			}
		} break;

		case NOTIFICATION_DRAW: {
			v_scroll->set_custom_step(theme_cache.font->get_height(theme_cache.font_size));

			update_scrollbars();
			RID ci = get_canvas_item();

			Ref<StyleBox> bg = theme_cache.panel_style;
			const Rect2 content_rect = _get_content_rect();

			Point2 draw_ofs = content_rect.position;
			Size2 draw_size = content_rect.size;

			int tbh = _get_title_button_height();

			draw_ofs.y += tbh;
			draw_size.y -= tbh;

			const real_t clip_top = show_column_titles ? draw_ofs.y : 0.0f; // Start clipping below headers when they are visible
			const real_t bottom_margin = theme_cache.panel_style->get_margin(SIDE_BOTTOM);
			const real_t clip_bottom = draw_ofs.y + draw_size.y + bottom_margin; // End clip after the bottom margin
			const Rect2 main_clip_rect = Rect2(0.0f, clip_top, get_size().x, clip_bottom - clip_top);
			RenderingServer *rendering_server = RenderingServer::get_singleton();

			bg->draw(ci, Rect2(Point2(), get_size()));

			Rect2 header_clip_rect = Rect2(content_rect.position.x, 0.0f, content_rect.size.x, get_size().y); // Keep headers out of the scrollbar area.
			if (v_scroll->is_visible()) {
				const Rect2 v_scroll_rect = Rect2(v_scroll->get_position(), v_scroll->get_size()); // Actual scrollbar rect in FileSystemTree coordinates.
				if (v_scroll_rect.position.x <= header_clip_rect.position.x) {
					const real_t header_right = header_clip_rect.get_end().x;
					header_clip_rect.position.x = v_scroll_rect.get_end().x;
					header_clip_rect.size.x = MAX(0.0f, header_right - header_clip_rect.position.x);
				} else if (v_scroll_rect.position.x < header_clip_rect.get_end().x) {
					header_clip_rect.size.x = MAX(0.0f, v_scroll_rect.position.x - header_clip_rect.position.x);
				}
			}
			rendering_server->canvas_item_clear(header_ci);
			rendering_server->canvas_item_set_custom_rect(header_ci, !is_visibility_clip_disabled(), header_clip_rect);
			rendering_server->canvas_item_set_clip(header_ci, true);

			rendering_server->canvas_item_clear(content_ci);
			rendering_server->canvas_item_set_custom_rect(content_ci, !is_visibility_clip_disabled(), main_clip_rect);
			rendering_server->canvas_item_set_clip(content_ci, true);

			cache.rtl = is_layout_rtl();
			content_scale_factor = popup_editor->is_embedded() ? 1.0 : popup_editor->get_parent_visible_window()->get_content_scale_factor();

			if (root && get_size().x > 0 && get_size().y > 0) {
				int self_height = 0; // Just to pass a reference, we don't need the root's `self_height`.
				draw_item(Point2(), draw_ofs, draw_size, root, self_height);
			}

			if (show_column_titles) {
				// Title buttons.
				int ofs2 = theme_cache.panel_style->get_margin(SIDE_LEFT);
				for (int i = 0; i < columns.size(); i++) {
					Ref<StyleBox> sb = (cache.click_type == Cache::CLICK_TITLE && cache.click_index == i) ? theme_cache.title_button_pressed : ((cache.hover_header_row && cache.hover_header_column == i) ? theme_cache.title_button_hover : theme_cache.title_button);
					Rect2 tbrect = Rect2(ofs2 - theme_cache.offset.x, bg->get_margin(SIDE_TOP), get_column_width(i), tbh);
					if (cache.rtl) {
						tbrect.position.x = get_size().width - tbrect.size.x - tbrect.position.x;
					}
					sb->draw(header_ci, tbrect);
					ofs2 += tbrect.size.width;

					// Arrow.
					if (current_column >= 0) {
						int visible_column = _get_visible_column(i);
						if (visible_column >= 0 && visible_column == current_column) {
							Ref<Texture2D> arrow;

							bool reverse = column_settings[current_column].reversed;
							if (reverse) {
								arrow = theme_cache.arrow_down;
							} else {
								arrow = theme_cache.arrow_up;
							}

							Size2 arrow_full_size = arrow->get_size();
							Size2 arrow_draw_size = Size2(arrow_full_size.x, arrow_full_size.y / 2);

							Point2 apos = tbrect.position + Point2i((tbrect.size.x - arrow_draw_size.x) / 2, 0);
							Rect2 arrow_rect = Rect2(apos, arrow_draw_size);

							Point2 src_pos = Point2(0, arrow_full_size.y / 4);
							Rect2 arrow_src_rect = Rect2(src_pos, arrow_draw_size);
							arrow->draw_rect_region(header_ci, arrow_rect, arrow_src_rect);
						}
					}

					// Text.
					int clip_w = tbrect.size.width - sb->get_minimum_size().width;
					columns.write[i].text_buf->set_width(clip_w);
					columns.write[i].cached_minimum_width_dirty = true;

					Vector2 text_pos = Point2i(tbrect.position.x, tbrect.position.y + (tbrect.size.height - columns[i].text_buf->get_size().y) / 2);
					switch (columns[i].title_alignment) {
						case HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT: {
							text_pos.x += cache.rtl ? tbrect.size.width - (sb->get_offset().x + columns[i].text_buf->get_size().x) : sb->get_offset().x;
							break;
						}

						case HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT: {
							text_pos.x += cache.rtl ? sb->get_offset().x : tbrect.size.width - (sb->get_offset().x + columns[i].text_buf->get_size().x);
							break;
						}

						default: {
							text_pos.x += (tbrect.size.width - columns[i].text_buf->get_size().x) / 2;
							break;
						}
					}

					if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
						columns[i].text_buf->draw_outline(header_ci, text_pos, theme_cache.font_outline_size, theme_cache.font_outline_color);
					}
					columns[i].text_buf->draw(header_ci, text_pos, theme_cache.title_button_color);
				}
			}

			if (scroll_hint_mode != SCROLL_HINT_MODE_DISABLED) {
				Size2 size = get_size();
				float v_scroll_value = v_scroll->get_value();
				bool v_scroll_below_max = v_scroll_value < (get_internal_min_size().height - (content_rect.get_size().height - _get_title_button_height()) - 1);
				if (v_scroll_value > 1 || v_scroll_below_max) {
					int hint_height = theme_cache.scroll_hint->get_height();
					if ((scroll_hint_mode == SCROLL_HINT_MODE_BOTH || scroll_hint_mode == SCROLL_HINT_MODE_TOP) && v_scroll_value > 1) {
						draw_texture_rect(theme_cache.scroll_hint, Rect2(Point2(), Size2(size.width, hint_height)), tile_scroll_hint, theme_cache.scroll_hint_color);
					}
					if ((scroll_hint_mode == SCROLL_HINT_MODE_BOTH || scroll_hint_mode == SCROLL_HINT_MODE_BOTTOM) && v_scroll_below_max) {
						draw_texture_rect(theme_cache.scroll_hint, Rect2(Point2(0, size.height - hint_height), Size2(size.width, -hint_height)), tile_scroll_hint, theme_cache.scroll_hint_color);
					}
				}
			}

			// Draw the focus outline last, so that it is drawn in front of the section headings.
			// Otherwise, section heading backgrounds can appear to be in front of the focus outline when scrolling.
			if (has_focus(true)) {
				RenderingServer::get_singleton()->canvas_item_add_clip_ignore(ci, true);
				theme_cache.focus_style->draw(ci, Rect2(Point2(), get_size()));
				RenderingServer::get_singleton()->canvas_item_add_clip_ignore(ci, false);
			}

			_draw_selection();
		} break;

		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {
			// 	_update_display_mode();

			_update_all();
		} break;

		case NOTIFICATION_RESIZED:
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (popup_edited_item != nullptr) {
				Rect2 rect = _get_item_focus_rect(popup_edited_item);

				popup_editor->set_position(get_global_position() + rect.position);
				popup_editor->set_size(rect.size);
				popup_editor->child_controls_changed();
			}
		} break;
	}
}

void FileSystemTree::set_self_modulate(const Color &p_self_modulate) {
	if (get_self_modulate() == p_self_modulate) {
		return;
	}

	CanvasItem::set_self_modulate(p_self_modulate);
	RS::get_singleton()->canvas_item_set_self_modulate(header_ci, p_self_modulate);
	RS::get_singleton()->canvas_item_set_self_modulate(content_ci, p_self_modulate);
}

void FileSystemTree::_update_all() {
	for (int i = 0; i < columns.size(); i++) {
		update_column(i);
	}
	if (root) {
		update_item_cache(root);
	}
}

Size2 FileSystemTree::get_minimum_size() const {
	Vector2 min_size = Vector2(0, _get_title_button_height());

	if (theme_cache.panel_style.is_valid()) {
		min_size += theme_cache.panel_style->get_minimum_size();
	}

	Vector2 content_min_size = get_internal_min_size();
	if (h_scroll_enabled) {
		content_min_size.x = 0;
		min_size.y += h_scroll->get_combined_minimum_size().height;
	}
	if (v_scroll_enabled) {
		min_size.x += v_scroll->get_combined_minimum_size().width;
		content_min_size.y = 0;
	}

	return min_size + content_min_size;
}

FileSystemTreeItem *FileSystemTree::create_item(FileSystemTreeItem *p_parent, int p_index) {
	ERR_FAIL_COND_V(blocked > 0, nullptr);

	FileSystemTreeItem *ti = nullptr;

	if (p_parent) {
		ERR_FAIL_COND_V_MSG(p_parent->tree != this, nullptr, "A different tree owns the given parent");
		ti = p_parent->create_child(p_index);
	} else {
		if (!root) {
			// No root exists, make the given item the new root.
			ti = memnew(FileSystemTreeItem(this));
			ERR_FAIL_NULL_V(ti, nullptr);
			ti->cells.resize(columns.size());
			ti->is_root = true;
			root = ti;
		} else {
			// Root exists, append or insert to root.
			ti = create_item(root, p_index);
		}
	}
	_queue_update_hovered_item();
	return ti;
}

FileSystemTreeItem *FileSystemTree::get_root() const {
	return root;
}

FileSystemTreeItem *FileSystemTree::get_last_item() const {
	FileSystemTreeItem *last = root;
	while (last && last->last_child && !last->collapsed) {
		last = last->last_child;
	}

	return last;
}

void FileSystemTree::item_edited(int p_column, FileSystemTreeItem *p_item, MouseButton p_custom_mouse_index) {
	edited_item = p_item;
	if (p_item != nullptr && p_column >= 0 && p_column < p_item->cells.size()) {
		edited_item->cells.write[p_column].dirty = true;
		edited_item->cells.write[p_column].cached_minimum_size_dirty = true;
	}
	emit_signal(SNAME("item_edited"));
	if (p_custom_mouse_index != MouseButton::NONE) {
		emit_signal(SNAME("custom_item_clicked"), p_custom_mouse_index);
	}
}

void FileSystemTree::item_changed(int p_column, FileSystemTreeItem *p_item) {
	if (p_item != nullptr) {
		if (p_column >= 0 && p_column < p_item->cells.size()) {
			p_item->cells.write[p_column].dirty = true;
			columns.write[p_column].cached_minimum_width_dirty = true;
		} else if (p_column == -1) {
			for (int i = 0; i < p_item->cells.size(); i++) {
				p_item->cells.write[i].dirty = true;
				columns.write[i].cached_minimum_width_dirty = true;
			}
		}
	}
	update_min_size_for_item_change();
	queue_redraw();
}

void FileSystemTree::item_selected(FileSystemTreeItem *p_item) {
	if (!p_item->selectable) {
		return;
	}

	p_item->selected = true;
	//emit_signal(SNAME("item_selected"),p_item,true); - NO this is for `FileSystemTreeItem::select`

	selected_item = p_item;

	queue_redraw();
}

void FileSystemTree::item_deselected(FileSystemTreeItem *p_item) {
	selected_item = p_item;
	p_item->selected = false;
	queue_redraw();
}

void FileSystemTree::update_min_size_for_item_change() {
	// Only need to update when any scroll bar is disabled because that's the only time item size
	// affects tree size.
	if (!h_scroll_enabled || !v_scroll_enabled) {
		update_minimum_size();
	}
}

void FileSystemTree::deselect_all() {
	if (root) {
		FileSystemTreeItem *item = root;
		while (item) {
			item->deselect();
			FileSystemTreeItem *prev_item = item;
			item = get_next_selected(root);
			ERR_FAIL_COND(item == prev_item);
		}
	}

	selected_item = nullptr;
	queue_redraw();
}

bool FileSystemTree::is_anything_selected() {
	return (selected_item != nullptr);
}

void FileSystemTree::clear() {
	ERR_FAIL_COND(blocked > 0);

	if (pressing_for_editor) {
		if (range_drag_enabled) {
			range_drag_enabled = false;
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
			warp_mouse(range_drag_capture_pos);
		}
		pressing_for_editor = false;
	}

	if (root) {
		memdelete(root);
		root = nullptr;
	};

	selected_item = nullptr;
	edited_item = nullptr;
	popup_edited_item = nullptr;
	popup_pressing_edited_item = nullptr;

	current_column = -1;
	drag_type = DRAG_NONE;
	drag_from_item = nullptr;

	_determine_hovered_item();

	queue_redraw();
}

void FileSystemTree::set_hide_root(bool p_enabled) {
	if (hide_root == p_enabled) {
		return;
	}

	hide_root = p_enabled;
	queue_redraw();
	update_minimum_size();
}

bool FileSystemTree::is_root_hidden() const {
	return hide_root;
}

void FileSystemTree::set_column_custom_minimum_width(int p_column, int p_min_width) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (columns[p_column].custom_min_width == p_min_width) {
		return;
	}

	if (p_min_width < 0) {
		return;
	}
	columns.write[p_column].custom_min_width = p_min_width;
	columns.write[p_column].cached_minimum_width_dirty = true;
	queue_redraw();
}

void FileSystemTree::set_column_expand(int p_column, bool p_expand) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (columns[p_column].expand == p_expand) {
		return;
	}

	columns.write[p_column].expand = p_expand;
	columns.write[p_column].cached_minimum_width_dirty = true;
	queue_redraw();
}

void FileSystemTree::set_column_expand_ratio(int p_column, int p_ratio) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (columns[p_column].expand_ratio == p_ratio) {
		return;
	}

	columns.write[p_column].expand_ratio = p_ratio;
	columns.write[p_column].cached_minimum_width_dirty = true;
	queue_redraw();
}

void FileSystemTree::set_column_clip_content(int p_column, bool p_fit) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (columns[p_column].clip_content == p_fit) {
		return;
	}

	columns.write[p_column].clip_content = p_fit;
	columns.write[p_column].cached_minimum_width_dirty = true;
	queue_redraw();
}

bool FileSystemTree::is_column_expanding(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), false);

	return columns[p_column].expand;
}

int FileSystemTree::get_column_expand_ratio(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), 1);

	return columns[p_column].expand_ratio;
}

bool FileSystemTree::is_column_clipping_content(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), false);

	return columns[p_column].clip_content;
}

FileSystemTreeItem *FileSystemTree::get_selected() const {
	return selected_item;
}

void FileSystemTree::set_selected(FileSystemTreeItem *p_item) {
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND_MSG(p_item->get_tree() != this, "The provided FileSystemTreeItem does not belong to this FileSystemTree. Ensure that the FileSystemTreeItem is a part of the FileSystemTree before setting it as selected.");

	select_item(p_item, get_root());

	queue_redraw();
}

FileSystemTreeItem *FileSystemTree::get_edited() const {
	return edited_item;
}

FileSystemTreeItem *FileSystemTree::get_next_selected(FileSystemTreeItem *p_item) {
	if (!root) {
		return nullptr;
	}

	while (true) {
		if (!p_item) {
			p_item = root;
		} else {
			if (p_item->first_child) {
				p_item = p_item->first_child;

			} else if (p_item->next) {
				p_item = p_item->next;
			} else {
				while (!p_item->next) {
					p_item = p_item->parent;
					if (p_item == nullptr) {
						return nullptr;
					}
				}

				p_item = p_item->next;
			}
		}

		if (p_item->selected) {
			return p_item;
		}
	}

	return nullptr;
}

int FileSystemTree::get_column_minimum_width(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), -1);

	if (columns[p_column].cached_minimum_width_dirty) {
		// Use the custom minimum width.
		int min_width = columns[p_column].custom_min_width;

		// Check if the visible title of the column is wider.
		if (show_column_titles) {
			const float padding = theme_cache.title_button->get_margin(SIDE_LEFT) + theme_cache.title_button->get_margin(SIDE_RIGHT);
			min_width = MAX(theme_cache.font->get_string_size(columns[p_column].xl_title, HORIZONTAL_ALIGNMENT_LEFT, -1, theme_cache.font_size).width + padding, min_width);
		}

		if (root && !columns[p_column].clip_content) {
			int depth = 1;

			FileSystemTreeItem *last = nullptr;
			FileSystemTreeItem *first = hide_root ? root->get_next_visible() : root;
			for (FileSystemTreeItem *item = first; item; last = item, item = item->get_next_visible()) {
				// Get column indentation.
				int indent;
				if (p_column == 0) {
					if (last) {
						if (item->parent == last) {
							depth += 1;
						} else if (item->parent != last->parent) {
							depth = hide_root ? 0 : 1;
							for (FileSystemTreeItem *iter = item->parent; iter; iter = iter->parent) {
								depth += 1;
							}
						}
					}
					indent = theme_cache.item_margin * depth;
				} else {
					indent = theme_cache.h_separation;
				}

				// Get the item minimum size.
				Size2 item_size = item->get_minimum_size(p_column);
				item_size.width += indent;

				// Check if the item is wider.
				min_width = MAX(min_width, item_size.width);
			}
		}

		columns.get(p_column).cached_minimum_width = min_width;
		columns.get(p_column).cached_minimum_width_dirty = false;
	}

	return columns[p_column].cached_minimum_width;
}

int FileSystemTree::get_column_width(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), -1);

	int column_width = get_column_minimum_width(p_column);

	if (columns[p_column].expand) {
		int expand_area = _get_content_rect().size.width;
		int expanding_total = 0;

		for (int i = 0; i < columns.size(); i++) {
			expand_area -= get_column_minimum_width(i);
			if (columns[i].expand) {
				expanding_total += columns[i].expand_ratio;
			}
		}

		if (expand_area >= expanding_total && expanding_total > 0) {
			column_width += expand_area * columns[p_column].expand_ratio / expanding_total;
		}
	}

	return column_width;
}

void FileSystemTree::propagate_set_columns(FileSystemTreeItem *p_item) {
	p_item->cells.resize(columns.size());

	FileSystemTreeItem *c = p_item->get_first_child();
	while (c) {
		propagate_set_columns(c);
		c = c->next;
	}
}

void FileSystemTree::set_columns(int p_columns) {
	ERR_FAIL_COND(p_columns < 1);
	ERR_FAIL_COND(blocked > 0);

	columns.resize(p_columns);

	if (root) {
		propagate_set_columns(root);
	}
	update_min_size_for_item_change();
	queue_redraw();
}

int FileSystemTree::get_columns() const {
	return columns.size();
}

void FileSystemTree::_scroll_moved(float) {
	_determine_hovered_item();
	queue_redraw();
}

Rect2 FileSystemTree::get_custom_popup_rect() const {
	return custom_popup_rect;
}

int FileSystemTree::get_item_offset(FileSystemTreeItem *p_item) const {
	FileSystemTreeItem *it = root;
	int ofs = _get_title_button_height();
	if (!it) {
		return 0;
	}

	while (true) {
		if (it == p_item) {
			return ofs;
		}

		if ((it != root || !hide_root) && it->is_visible_in_tree()) {
			ofs += compute_item_height(it);
			ofs += theme_cache.v_separation;
		}

		if (it->first_child && !it->collapsed) {
			it = it->first_child;

		} else if (it->next) {
			it = it->next;
		} else {
			while (!it->next) {
				it = it->parent;
				if (it == nullptr) {
					return 0;
				}
			}

			it = it->next;
		}
	}

	return -1; // Not found.
}

void FileSystemTree::ensure_cursor_is_visible() {
	if (!is_inside_tree()) {
		return;
	}
	if (!selected_item) {
		return; // Nothing under cursor.
	}

	// Note: Code below similar to `FileSystemTree::scroll_to_item()`, in case of bug fix both.
	const Size2 area_size = _get_content_rect().size;

	int y_offset = get_item_offset(selected_item);
	if (y_offset != -1) {
		const int tbh = _get_title_button_height();
		y_offset -= tbh;

		const int cell_h = compute_item_height(selected_item) + theme_cache.v_separation;
		int screen_h = area_size.height - tbh;

		if (cell_h > screen_h) { // Screen size is too small, maybe it was not resized yet.
			v_scroll->set_value(y_offset);
		} else if (y_offset + cell_h > v_scroll->get_value() + screen_h) {
			callable_mp((Range *)v_scroll, &Range::set_value).call_deferred(y_offset - screen_h + cell_h);
		} else if (y_offset < v_scroll->get_value()) {
			v_scroll->set_value(y_offset);
		}
	}

	// Scroll horizontally.
	// if (display_mode == DISPLAY_MODE_TREE) { // Cursor always at column 0 in list mode.
	// 	int x_offset = 0;
	// 	const int cell_w = get_column_width(0);
	// 	const int screen_w = area_size.width;

	// 	if (cell_w > screen_w) {
	// 		h_scroll->set_value(x_offset);
	// 	} else if (x_offset + cell_w > h_scroll->get_value() + screen_w) {
	// 		callable_mp((Range *)h_scroll, &Range::set_value).call_deferred(x_offset - screen_w + cell_w);
	// 	} else if (x_offset < h_scroll->get_value()) {
	// 		h_scroll->set_value(x_offset);
	// 	}
	// }
}

int FileSystemTree::get_pressed_button() const {
	return pressed_button;
}

Rect2 FileSystemTree::get_item_rect(FileSystemTreeItem *p_item, int p_column, int p_button) const {
	ERR_FAIL_NULL_V(p_item, Rect2());
	ERR_FAIL_COND_V(p_item->tree != this, Rect2());
	if (p_column != -1) {
		ERR_FAIL_INDEX_V(p_column, columns.size(), Rect2());
	}
	ERR_FAIL_COND_V(p_button != -1, Rect2()); // Buttons removed.

	int ofs = get_item_offset(p_item);
	int height = compute_item_height(p_item) + theme_cache.v_separation;
	Rect2 r;
	r.position.y = ofs - theme_cache.offset.y + theme_cache.panel_style->get_offset().y;
	r.size.height = height;
	bool rtl = is_layout_rtl();
	const Rect2 content_rect = _get_content_rect();

	if (p_column == -1) {
		r.position.x = 0;
		r.size.x = get_size().width;
	} else {
		int accum = 0;
		for (int i = 0; i < p_column; i++) {
			accum += get_column_width(i);
		}
		r.position.x = (rtl) ? get_size().x - (accum - theme_cache.offset.x) - get_column_width(p_column) - theme_cache.panel_style->get_margin(SIDE_LEFT) : accum - theme_cache.offset.x + theme_cache.panel_style->get_margin(SIDE_LEFT);
		r.size.x = get_column_width(p_column);
	}

	return r;
}

void FileSystemTree::set_column_titles_visible(bool p_show) {
	if (show_column_titles == p_show) {
		return;
	}

	show_column_titles = p_show;
	queue_redraw();
	update_minimum_size();
}

bool FileSystemTree::are_column_titles_visible() const {
	return show_column_titles;
}

void FileSystemTree::set_column_title(int p_column, const String &p_title) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (columns[p_column].title == p_title) {
		return;
	}

	columns.write[p_column].title = p_title;
	columns.write[p_column].xl_title = atr(p_title);
	update_column(p_column);
	queue_redraw();
}

String FileSystemTree::get_column_title(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].title;
}

void FileSystemTree::set_column_title_tooltip_text(int p_column, const String &p_tooltip) {
	ERR_FAIL_INDEX(p_column, columns.size());
	columns.write[p_column].title_tooltip = p_tooltip;
}

String FileSystemTree::get_column_title_tooltip_text(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].title_tooltip;
}

void FileSystemTree::set_column_title_alignment(int p_column, HorizontalAlignment p_alignment) {
	ERR_FAIL_INDEX(p_column, columns.size());

	if (p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
		WARN_PRINT("HORIZONTAL_ALIGNMENT_FILL is not supported for column titles.");
	}

	if (columns[p_column].title_alignment == p_alignment) {
		return;
	}

	columns.write[p_column].title_alignment = p_alignment;
	update_column(p_column);
	queue_redraw();
}

HorizontalAlignment FileSystemTree::get_column_title_alignment(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	return columns[p_column].title_alignment;
}

void FileSystemTree::set_column_title_direction(int p_column, Control::TextDirection p_text_direction) {
	ERR_FAIL_INDEX(p_column, columns.size());
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (columns[p_column].text_direction != p_text_direction) {
		columns.write[p_column].text_direction = p_text_direction;
		update_column(p_column);
		queue_redraw();
	}
}

Control::TextDirection FileSystemTree::get_column_title_direction(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), TEXT_DIRECTION_INHERITED);
	return columns[p_column].text_direction;
}

void FileSystemTree::set_column_title_language(int p_column, const String &p_language) {
	ERR_FAIL_INDEX(p_column, columns.size());
	if (columns[p_column].language != p_language) {
		columns.write[p_column].language = p_language;
		update_column(p_column);
		queue_redraw();
	}
}

String FileSystemTree::get_column_title_language(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].language;
}

Point2 FileSystemTree::get_scroll() const {
	Point2 ofs;
	if (h_scroll->is_visible_in_tree()) {
		ofs.x = h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		ofs.y = v_scroll->get_value();
	}
	return ofs;
}

void FileSystemTree::scroll_to_item(FileSystemTreeItem *p_item, bool p_center_on_item) {
	ERR_FAIL_NULL(p_item);

	update_scrollbars();

	// Note: Code below similar to `FileSystemTree::ensure_cursor_is_visible()`, in case of bug fix both.
	const Size2 area_size = _get_content_rect().size;

	int y_offset = get_item_offset(p_item);
	if (y_offset != -1) {
		const int title_button_height = _get_title_button_height();
		y_offset -= title_button_height;

		const int cell_h = compute_item_height(p_item) + theme_cache.v_separation;
		int screen_h = area_size.height - title_button_height;

		if (p_center_on_item) {
			// This makes sure that centering the offset doesn't overflow.
			const double v_scroll_value = y_offset - MAX((screen_h - cell_h) / 2.0, 0.0);
			v_scroll->set_value(v_scroll_value);
		} else {
			if (cell_h > screen_h) { // Screen size is too small, maybe it was not resized yet.
				v_scroll->set_value(y_offset);
			} else if (y_offset + cell_h > v_scroll->get_value() + screen_h) {
				v_scroll->set_value(y_offset - screen_h + cell_h);
			} else if (y_offset < v_scroll->get_value()) {
				v_scroll->set_value(y_offset);
			}
		}
	}
}

void FileSystemTree::set_h_scroll_enabled(bool p_enable) {
	if (h_scroll_enabled == p_enable) {
		return;
	}

	h_scroll_enabled = p_enable;
	update_minimum_size();
}

bool FileSystemTree::is_h_scroll_enabled() const {
	return h_scroll_enabled;
}

void FileSystemTree::set_v_scroll_enabled(bool p_enable) {
	if (v_scroll_enabled == p_enable) {
		return;
	}

	v_scroll_enabled = p_enable;
	update_minimum_size();
}

bool FileSystemTree::is_v_scroll_enabled() const {
	return v_scroll_enabled;
}

void FileSystemTree::set_scroll_hint_mode(ScrollHintMode p_mode) {
	if (scroll_hint_mode == p_mode) {
		return;
	}

	scroll_hint_mode = p_mode;
	queue_redraw();
}

FileSystemTree::ScrollHintMode FileSystemTree::get_scroll_hint_mode() const {
	return scroll_hint_mode;
}

void FileSystemTree::set_tile_scroll_hint(bool p_enable) {
	if (tile_scroll_hint == p_enable) {
		return;
	}

	tile_scroll_hint = p_enable;
	queue_redraw();
}

bool FileSystemTree::is_scroll_hint_tiled() {
	return tile_scroll_hint;
}

FileSystemTreeItem *FileSystemTree::_search_item_text(FileSystemTreeItem *p_at, const String &p_find, int *r_col, bool p_selectable, bool p_backwards) {
	FileSystemTreeItem *from = p_at;
	FileSystemTreeItem *loop = nullptr; // Safe-guard against infinite loop.

	while (p_at) {
		for (int i = 0; i < columns.size(); i++) {
			if (p_at->get_text(i).findn(p_find) == 0 && (!p_selectable || p_at->is_selectable())) {
				if (r_col) {
					*r_col = i;
				}
				return p_at;
			}
		}

		if (p_backwards) {
			p_at = p_at->get_prev_visible(true);
		} else {
			p_at = p_at->get_next_visible(true);
		}

		if ((p_at) == from) {
			break;
		}

		if (!loop) {
			loop = p_at;
		} else if (loop == p_at) {
			break;
		}
	}

	return nullptr;
}

FileSystemTreeItem *FileSystemTree::search_item_text(const String &p_find, int *r_col, bool p_selectable) {
	FileSystemTreeItem *from = get_selected();

	if (!from) {
		from = root;
	}
	if (!from) {
		return nullptr;
	}

	return _search_item_text(from->get_next_visible(true), p_find, r_col, p_selectable);
}

FileSystemTreeItem *FileSystemTree::get_item_with_text(const String &p_find) const {
	for (FileSystemTreeItem *current = root; current; current = current->get_next_visible()) {
		for (int i = 0; i < columns.size(); i++) {
			if (current->get_text(i) == p_find) {
				return current;
			}
		}
	}
	return nullptr;
}

FileSystemTreeItem *FileSystemTree::get_item_with_metadata(const Variant &p_find, int p_column) const {
	if (p_column < 0) {
		for (FileSystemTreeItem *current = root; current; current = current->get_next_in_tree()) {
			for (int i = 0; i < columns.size(); i++) {
				if (current->get_metadata(i) == p_find) {
					return current;
				}
			}
		}
		return nullptr;
	}

	for (FileSystemTreeItem *current = root; current; current = current->get_next_in_tree()) {
		if (current->get_metadata(p_column) == p_find) {
			return current;
		}
	}
	return nullptr;
}

void FileSystemTree::_do_incr_search(const String &p_add) {
	uint64_t time = OS::get_singleton()->get_ticks_usec() / 1000; // Convert to msec.
	uint64_t diff = time - last_keypress;
	if (diff > uint64_t(GLOBAL_GET_CACHED(uint64_t, "gui/timers/incremental_search_max_interval_msec"))) {
		incr_search = p_add;
	} else if (incr_search != p_add) {
		incr_search += p_add;
	}

	last_keypress = time;
	int col;
	FileSystemTreeItem *item = search_item_text(incr_search, &col, true);
	if (!item) {
		return;
	}

	// item->set_as_cursor();
	set_selected(item);

	ensure_cursor_is_visible();
}

FileSystemTreeItem *FileSystemTree::_find_item_at_pos(FileSystemTreeItem *p_item, const Point2 &p_pos, int &r_column, int &r_height, int &r_section) const {
	r_column = -1;
	r_height = 0;
	r_section = -100;

	if (!root) {
		return nullptr;
	}

	Point2 pos = p_pos;

	if ((root != p_item || !hide_root) && p_item->is_visible_in_tree()) {
		r_height = compute_item_height(p_item) + theme_cache.v_separation;
		if (pos.y < r_height) {
			if (drop_mode_flags == DROP_MODE_ON_ITEM) {
				r_section = 0;
			} else if (drop_mode_flags == DROP_MODE_INBETWEEN) {
				r_section = pos.y < r_height / 2 ? -1 : 1;
			} else if (pos.y < r_height / 4) {
				r_section = -1;
			} else if (pos.y >= (r_height * 3 / 4)) {
				r_section = 1;
			} else {
				r_section = 0;
			}

			for (int i = 0; i < columns.size(); i++) {
				int col_width = get_column_width(i);

				if (p_item->cells[i].expand_right) {
					int plus = 1;
					while (i + plus < columns.size() && !p_item->cells[i + plus].editable && p_item->cells[i + plus].mode == FileSystemTreeItem::CELL_MODE_STRING && p_item->cells[i + plus].text.is_empty() && p_item->cells[i + plus].icon.is_null()) {
						col_width += theme_cache.h_separation;
						col_width += get_column_width(i + plus);
						plus++;
					}
				}

				if (pos.x < col_width) {
					r_column = i;
					return p_item;
				}
				pos.x -= col_width;
			}

			return nullptr;
		} else {
			pos.y -= r_height;
		}
	} else {
		r_height = 0;
	}

	if (p_item->is_collapsed() || !p_item->is_visible_in_tree()) {
		return nullptr; // Do not try children, it's collapsed.
	}

	FileSystemTreeItem *n = p_item->get_first_child();
	while (n) {
		int ch;
		FileSystemTreeItem *r = _find_item_at_pos(n, pos, r_column, ch, r_section);
		pos.y -= ch;
		r_height += ch;
		if (r) {
			return r;
		}
		n = n->get_next();
	}

	return nullptr;
}

void FileSystemTree::_find_button_at_pos(const Point2 &p_pos, FileSystemTreeItem *&r_item, int &r_column, int &r_index, int &r_section) const {
	// When on a button, `r_index` is valid.
	// When on an item, both `r_item` and `r_column` are valid.
	// Otherwise, all output arguments are invalid.
	r_item = nullptr;
	r_column = -1;
	r_index = -1;
	r_section = -1;

	if (!root) {
		return;
	}

	Point2 pos = p_pos;
	if (cache.rtl) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return;
	}
	pos += theme_cache.offset; // Scrolling.

	int x_limit = _get_content_rect().size.x + theme_cache.offset.width;

	if (pos.x > x_limit) {
		// Inside the scroll area.
		return;
	}

	int col, h, section;
	FileSystemTreeItem *it = _find_item_at_pos(root, pos, col, h, section);
	if (!it) {
		return;
	}

	r_item = it;
	r_column = col;
	r_section = section;

	return;
}

FileSystemTree::FindColumnButtonResult FileSystemTree::_find_column_and_button_at_pos(int p_x, const FileSystemTreeItem *p_item, int p_x_ofs, int p_x_limit) const {
	int x = p_x;

	int col = -1;
	int col_ofs = 0;
	int col_width = 0;

	int limit_w = p_x_limit;

	FindColumnButtonResult result;

	for (int i = 0; i < columns.size(); i++) {
		col_width = get_column_width(i);

		if (p_item->cells[i].expand_right) {
			int plus = 1;
			while (i + plus < columns.size() && !p_item->cells[i + plus].editable && p_item->cells[i + plus].mode == FileSystemTreeItem::CELL_MODE_STRING && p_item->cells[i + plus].text.is_empty() && p_item->cells[i + plus].icon.is_null()) {
				col_width += theme_cache.h_separation;
				col_width += get_column_width(i + plus);
				plus++;
			}
		}

		if (x < col_width) {
			col = i;
			break;
		}

		col_ofs += col_width;
		x -= col_width;
		limit_w -= col_width;
	}

	if (col >= 0) {
		if (col == 0) {
			int margin = p_x_ofs + theme_cache.item_margin;
			col_width -= margin;
			limit_w -= margin;
			col_ofs += margin;
			x -= margin;
		} else {
			col_width -= theme_cache.h_separation;
			limit_w -= theme_cache.h_separation;
			x -= theme_cache.h_separation;
		}
	}

	result.column_index = col;
	result.column_offset = col_ofs;
	result.column_width = col_width;
	result.pos_x = x;

	return result;
}

int FileSystemTree::get_column_at_position(const Point2 &p_pos) const {
	int col, h, section;
	if (_get_item_at_pos(p_pos, col, h, section)) {
		return col;
	}
	return -1;
}

int FileSystemTree::get_drop_section_at_position(const Point2 &p_pos) const {
	int col, h, section;
	if (_get_item_at_pos(p_pos, col, h, section)) {
		return section;
	}
	return -100;
}

Variant FileSystemTree::get_drag_data(const Point2 &p_point) {
	if (drag_type == DRAG_BOX_SELECTION) {
		return false;
	}

	int col, h, section;
	FileSystemTreeItem *item = _get_item_at_pos(p_point, col, h, section);
	// if (!item || col != 0) {
	if (!item) {
		return Variant(); // Control::get_drag_data(p_point);
	}

	int icount = _count_selected_items(root);
	if (icount <= 0 || (icount == 1 && prev_selected_item != item) || (icount > 1 && !item->selected)) {
		return Variant();
	}

	bool has_folder = false;
	bool has_file = false;
	Vector<FileSystemTreeItem *> selected_items = _get_selected_items();
	// Vector<String> paths = get_selected_paths();
	Vector<String> paths;

	int max_rows = 6;
	int num_rows = selected_items.size() > max_rows ? max_rows - 1 : selected_items.size(); // Don't waste a row to say "1 more file" - list it instead.
	VBoxContainer *vbox = memnew(VBoxContainer);

	for (int i = 0; i < selected_items.size(); i++) {
		const FileSystemTreeItem *ti = selected_items[i];
		Dictionary d = ti->get_metadata(0);
		paths.push_back(d["path"]);

		bool is_folder = FileSystemAccess::is_dir_type(d["type"]);
		has_folder |= is_folder;
		has_file |= !is_folder;

		if (i < num_rows) {
			HBoxContainer *hbox = memnew(HBoxContainer);
			TextureRect *icon = memnew(TextureRect);
			Label *label = memnew(Label);
			label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
			label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);

			if (is_folder) {
				label->set_text(paths[i].substr(0, paths[i].length() - 1).get_file());
				icon->set_texture(get_app_theme_icon(SNAME("Folder")));
			} else {
				label->set_text(paths[i].get_file());
				icon->set_texture(get_app_theme_icon(SNAME("File")));
			}
			icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			icon->set_size(Size2(16, 16));
			hbox->add_child(icon);
			hbox->add_child(label);
			vbox->add_child(hbox);
		}
	}

	if (paths.size() > num_rows) {
		Label *label = memnew(Label);
		label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		if (has_file && has_folder) {
			label->set_text(vformat(RTR("%d more files or folders"), paths.size() - num_rows));
		} else if (has_folder) {
			label->set_text(vformat(RTR("%d more folders"), paths.size() - num_rows));
		} else {
			label->set_text(vformat(RTR("%d more files"), paths.size() - num_rows));
		}
		vbox->add_child(label);
	}
	set_drag_preview(vbox);

	Dictionary drag_data;
	drag_data["type"] = has_folder ? "files_and_dirs" : "files";
	drag_data["files"] = paths;
	// drag_data["from"] = this;
	return drag_data;
}

bool FileSystemTree::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	Dictionary drag_data = p_data;

	if (drag_data.has("type") && (String(drag_data["type"]) == "files" || String(drag_data["type"]) == "files_and_dirs")) {
		// Move files or dir.
		String to_dir;
		_get_drag_target_folder(to_dir, p_point);

		if (to_dir.is_empty()) {
			return false;
		}

		// Attempting to move a folder into itself will fail later,
		// rather than bring up a message don't try to do it in the first place.
		// Disallow moving an item into its own folder.
		Vector<String> fnames = drag_data["files"];
		for (int i = 0; i < fnames.size(); ++i) {
			if (to_dir.begins_with(fnames[i]) || fnames[i].get_base_dir() == to_dir) {
				// print_line("disallow:", fnames[i], fnames[i].get_base_dir(), to_dir);
				return false;
			}
		}

		return true;
	}

	return false; // Control::can_drop_data(p_point, p_data);
}

void FileSystemTree::drop_data(const Point2 &p_point, const Variant &p_data) {
	Dictionary drag_data = p_data;

	if (drag_data.has("type") && (String(drag_data["type"]) == "files" || String(drag_data["type"]) == "files_and_dirs")) {
		// Move files or add to favorites.
		String to_dir;
		_get_drag_target_folder(to_dir, p_point);

		if (to_dir.is_empty()) {
			return;
		}

		Vector<String> fnames = drag_data["files"];

		bool is_copy = Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL);
		Vector<String> dest_paths;
		Error err = FileSystemAccess::move(is_copy, to_dir, fnames, dest_paths);
		// print_line("drop: ", is_copy, to_dir, fnames, dest_paths);
		if (err == OK && file_system) {
			// print_line("drop scan: ", to_dir);
			file_system->scan(to_dir, true);

			if (!is_copy) {
				// Update the source directory for cut operations.
				Vector<String> dirs;
				for (const String &path : fnames) {
					String base_dir = path.get_base_dir();
					if (!dirs.has(base_dir)) {
						dirs.push_back(base_dir);
					}
				}
				// print_line("drop scan dirs: ", dirs.size(), dirs);
				file_system->scan(dirs, true);
			}

			if (!dest_paths.is_empty()) {
				to_select.clear();
				to_select = dest_paths;
			}
		}
	}
}

void FileSystemTree::_get_drag_target_folder(String &target, const Point2 &p_point) const {
	target = String();

	FileSystemTreeItem *ti = (p_point == Vector2(Math::INF, Math::INF)) ? get_root() : get_item_at_position(p_point);

	if (!ti && display_mode == DISPLAY_MODE_LIST) {
		ti = get_root();
	}

	if (ti) {
		// int section = (p_point == Vector2(Math::INF, Math::INF)) ? get_drop_section_at_position(get_item_rect(ti).position) : get_drop_section_at_position(p_point);
		Dictionary d = ti->get_metadata(0);
		if (!d.is_empty()) {
			String type = d.get("type", "");
			String path = d.get("path", "");
			// bool is_folder = FileSystemAccess::is_dir_type(type);
			// if (is_folder && path != COMPUTER_PATH) {
			if (FileSystemAccess::is_dir_type(type)) {
				if (!FileSystemAccess::is_root_type(type)) {
					target = path;
				}
			} else {
				FileSystemTreeItem *parent = ti->get_parent();
				if (parent) {
					d = parent->get_metadata(0);
					if (!d.is_empty()) {
						type = d.get("type", "");
						path = d.get("path", "");
						if (FileSystemAccess::is_dir_type(type)) {
							if (!FileSystemAccess::is_root_type(type)) {
								target = path;
							}
						}
					}
				}
			}
		}
	}
	// print_line("drag target: ", p_point, target);
}

// static inline bool _folder_comparator(bool &r_ret, const Dictionary &a_meta, const Dictionary &b_meta) {
// 	bool a_is_folder = FileSystemAccess::is_dir_type(a_meta.get("type", ""));
// 	bool b_is_folder = FileSystemAccess::is_dir_type(b_meta.get("type", ""));
// 	if (a_is_folder && !b_is_folder) {
// 		r_ret = true;
// 		return true;
// 	} else if (!a_is_folder && b_is_folder) {
// 		r_ret = false;
// 		return true;
// 	}
// 	return false;
// }

struct ColumnNameComparator {
	bool operator()(const FileSystemTreeItem *p_a, const FileSystemTreeItem *p_b) const {
		Dictionary a_meta = p_a->get_metadata(0);
		Dictionary b_meta = p_b->get_metadata(0);
		// bool ret = false;
		// if (_folder_comparator(ret, a_meta, b_meta)) {
		// 	return ret;
		// }
		String a_name = a_meta.get("name", "");
		String b_name = b_meta.get("name", "");
		return FileNoCaseComparator()(a_name, b_name);
	}
};

struct ColumnTypeComparator {
	bool operator()(const FileSystemTreeItem *p_a, const FileSystemTreeItem *p_b) const {
		Dictionary a_meta = p_a->get_metadata(0);
		Dictionary b_meta = p_b->get_metadata(0);
		// bool ret = false;
		// if (_folder_comparator(ret, a_meta, b_meta)) {
		// 	return ret;
		// }
		String a_name = a_meta.get("name", "");
		String b_name = b_meta.get("name", "");
		String a_type = a_meta.get("type", "");
		String b_type = b_meta.get("type", "");
		return FileNoCaseComparator()(a_name.get_extension() + a_type + a_name.get_basename(), b_name.get_extension() + b_type + b_name.get_basename());
	}
};

struct ColumnModifiedTimeComparator {
	bool operator()(const FileSystemTreeItem *p_a, const FileSystemTreeItem *p_b) const {
		Dictionary a_meta = p_a->get_metadata(0);
		Dictionary b_meta = p_b->get_metadata(0);
		// bool ret = false;
		// if (_folder_comparator(ret, a_meta, b_meta)) {
		// 	return ret;
		// }
		uint64_t a_modified_time = uint64_t(a_meta.get("modified_time", 0));
		uint64_t b_modified_time = uint64_t(b_meta.get("modified_time", 0));
		if (a_modified_time == b_modified_time) {
			String a_name = a_meta.get("name", "");
			String b_name = b_meta.get("name", "");
			return FileNoCaseComparator()(a_name, b_name);
		}
		return a_modified_time < b_modified_time;
	}
};

struct ColumnCreationTimeComparator {
	bool operator()(const FileSystemTreeItem *p_a, const FileSystemTreeItem *p_b) const {
		Dictionary a_meta = p_a->get_metadata(0);
		Dictionary b_meta = p_b->get_metadata(0);
		// bool ret = false;
		// if (_folder_comparator(ret, a_meta, b_meta)) {
		// 	return ret;
		// }
		uint64_t a_creation_time = uint64_t(a_meta.get("creation_time", 0));
		uint64_t b_creation_time = uint64_t(b_meta.get("creation_time", 0));
		if (a_creation_time == b_creation_time) {
			String a_name = a_meta.get("name", "");
			String b_name = b_meta.get("name", "");
			return FileNoCaseComparator()(a_name, b_name);
		}
		return a_creation_time < b_creation_time;
	}
};

struct ColumnSizeComparator {
	bool operator()(const FileSystemTreeItem *p_a, const FileSystemTreeItem *p_b) const {
		Dictionary a_meta = p_a->get_metadata(0);
		Dictionary b_meta = p_b->get_metadata(0);
		// bool ret = false;
		// if (_folder_comparator(ret, a_meta, b_meta)) {
		// 	return ret;
		// }
		uint64_t a_size = uint64_t(a_meta.get("size", 0));
		uint64_t b_size = uint64_t(b_meta.get("size", 0));
		if (a_size == b_size) {
			String a_name = a_meta.get("name", "");
			String b_name = b_meta.get("name", "");
			return FileNoCaseComparator()(a_name, b_name);
		}
		return a_size < b_size;
	}
};

int FileSystemTree::_get_visible_column(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, columns.size(), -1);

	int visible_index = 0;
	for (uint32_t i = 0; i < column_settings.size(); i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}
		if (visible_index >= p_column) {
			return i;
		}
		visible_index++;
	}
	return -1;
}

void FileSystemTree::_column_title_clicked(int p_column, MouseButton p_button) {
	ERR_FAIL_NULL(root);
	ERR_FAIL_INDEX(p_column, columns.size());

	if (display_mode != DISPLAY_MODE_LIST) {
		return;
	}

	if (root->get_child_count() <= 0) {
		return;
	}

	int visible_column = _get_visible_column(p_column);
	ERR_FAIL_INDEX(visible_column, (int)column_settings.size());

	List<FileSystemTreeItem *> dirs;
	List<FileSystemTreeItem *> files;
	// List<FileInfo> file_list;
	FileSystemTreeItem *item = root->get_first_child();
	while (item) {
		Dictionary d = item->get_metadata(0);
		if (FileSystemAccess::is_dir_type(d.get("type", ""))) {
			dirs.push_back(item);
		} else {
			files.push_back(item);
		}

		// FileInfo file_info;
		// file_info.name = d.get("name", "");
		// file_info.type = d.get("type", "");
		// file_info.modified_time = d.get("modified_time", 0);

		// file_list.push_back(file_info);

		FileSystemTreeItem *next = item->get_next();
		root->remove_child(item);

		item = next;
	}

	const ColumnSetting &setting = column_settings[visible_column];
	switch (setting.type) {
		case COLUMN_TYPE_NAME: {
			dirs.sort_custom<ColumnNameComparator>();
			files.sort_custom<ColumnNameComparator>();
		} break;
		case COLUMN_TYPE_MODIFIED: {
			dirs.sort_custom<ColumnModifiedTimeComparator>();
			files.sort_custom<ColumnModifiedTimeComparator>();
		} break;
		case COLUMN_TYPE_CREATED: {
			dirs.sort_custom<ColumnCreationTimeComparator>();
			files.sort_custom<ColumnCreationTimeComparator>();
		} break;
		case COLUMN_TYPE_TYPE: {
			dirs.sort_custom<ColumnTypeComparator>();
			files.sort_custom<ColumnTypeComparator>();
		} break;
		case COLUMN_TYPE_SIZE: {
			dirs.sort_custom<ColumnSizeComparator>();
			files.sort_custom<ColumnSizeComparator>();
		} break;
	}

	bool reversed = visible_column == current_column ? (!setting.reversed) : false;
	if (reversed) {
		dirs.reverse();
		files.reverse();
		for (FileSystemTreeItem *item : files) {
			root->add_child(item);
		}
		for (FileSystemTreeItem *item : dirs) {
			root->add_child(item);
		}
	} else {
		for (FileSystemTreeItem *item : dirs) {
			root->add_child(item);
		}
		for (FileSystemTreeItem *item : files) {
			root->add_child(item);
		}
	}
	column_settings[visible_column].reversed = reversed;
	// print_line("sorted by column: ", p_column, "visible: ", visible_column, " reversed: ", reversed, " prev column: ", current_column);

	current_column = visible_column;

	emit_signal(SNAME("column_title_clicked"), p_column, p_button);
}

FileSystemTreeItem *FileSystemTree::_get_item_at_pos(const Point2 &p_pos, int &r_column, int &r_height, int &r_section) const {
	if (!root || !Rect2(Vector2(), get_size()).has_point(p_pos)) {
		return nullptr;
	}

	Point2 pos = p_pos;
	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return nullptr;
	}

	if (h_scroll->is_visible_in_tree()) {
		pos.x += h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		pos.y += v_scroll->get_value();
	}

	FileSystemTreeItem *it = _find_item_at_pos(root, pos, r_column, r_height, r_section);

	if (it) {
		return it;
	}
	return nullptr;
}

FileSystemTreeItem *FileSystemTree::get_item_at_position(const Point2 &p_pos) const {
	int col, h, section;
	return _get_item_at_pos(p_pos, col, h, section);
}

String FileSystemTree::get_tooltip(const Point2 &p_pos) const {
	Point2 pos = p_pos - theme_cache.panel_style->get_offset();
	pos.y -= _get_title_button_height();

	// `pos.y` less than 0 indicates we're in the header.
	if (pos.y < 0) {
		// Get the x position of the cursor.
		real_t pos_x = p_pos.x;
		if (is_layout_rtl()) {
			pos_x = get_size().width - pos_x;
		}
		pos_x -= theme_cache.panel_style->get_offset().x;
		if (h_scroll->is_visible_in_tree()) {
			pos_x += h_scroll->get_value();
		}

		// Walk forwards until we know which column we're in.
		int next_edge = 0;
		int i = 0;
		for (const ColumnInfo &column : columns) {
			next_edge += get_column_width(i++);

			if (pos_x < next_edge) {
				if (!column.title_tooltip.is_empty()) {
					return column.title_tooltip;
				}
				break;
			}
		}

		// If the column has no tooltip, use the default.
		return Control::get_tooltip(p_pos);
	}

	FileSystemTreeItem *it;
	int col, index, section;
	_find_button_at_pos(p_pos, it, col, index, section);

	if (it) {
		const String item_tooltip = it->get_tooltip_text(col);
		if (enable_auto_tooltip && item_tooltip.is_empty()) {
			return it->get_text(col);
		}
		return item_tooltip;
	}

	return Control::get_tooltip(p_pos);
}

void FileSystemTree::set_cursor_can_exit_tree(bool p_enable) {
	cursor_can_exit_tree = p_enable;
}

void FileSystemTree::set_hide_folding(bool p_hide) {
	if (hide_folding == p_hide) {
		return;
	}

	hide_folding = p_hide;
	queue_redraw();
}

bool FileSystemTree::is_folding_hidden() const {
	return hide_folding;
}

void FileSystemTree::set_enable_recursive_folding(bool p_enable) {
	enable_recursive_folding = p_enable;
}

bool FileSystemTree::is_recursive_folding_enabled() const {
	return enable_recursive_folding;
}

void FileSystemTree::set_enable_drag_unfolding(bool p_enable) {
	if (enable_drag_unfolding == p_enable) {
		return;
	}

	enable_drag_unfolding = p_enable;
	if (!enable_drag_unfolding && !dropping_unfold_timer->is_stopped()) {
		dropping_unfold_timer->stop();
	}
}

bool FileSystemTree::is_drag_unfolding_enabled() const {
	return enable_drag_unfolding;
}

void FileSystemTree::set_drop_mode_flags(int p_flags) {
	if (drop_mode_flags == p_flags) {
		return;
	}
	drop_mode_flags = p_flags;
	if (drop_mode_flags == 0) {
		_reset_drop_mode_over();
	}

	queue_redraw();
}

int FileSystemTree::get_drop_mode_flags() const {
	return drop_mode_flags;
}

void FileSystemTree::set_edit_checkbox_cell_only_when_checkbox_is_pressed(bool p_enable) {
	force_edit_checkbox_only_on_checkbox = p_enable;
}

bool FileSystemTree::get_edit_checkbox_cell_only_when_checkbox_is_pressed() const {
	return force_edit_checkbox_only_on_checkbox;
}

void FileSystemTree::set_allow_rmb_select(bool p_allow) {
	allow_rmb_select = p_allow;
}

bool FileSystemTree::get_allow_rmb_select() const {
	return allow_rmb_select;
}

void FileSystemTree::set_allow_reselect(bool p_allow) {
	allow_reselect = p_allow;
}

bool FileSystemTree::get_allow_reselect() const {
	return allow_reselect;
}

void FileSystemTree::set_allow_search(bool p_allow) {
	allow_search = p_allow;
}

bool FileSystemTree::get_allow_search() const {
	return allow_search;
}

void FileSystemTree::set_auto_tooltip(bool p_enable) {
	enable_auto_tooltip = p_enable;
}

bool FileSystemTree::is_auto_tooltip_enabled() const {
	return enable_auto_tooltip;
}

void FileSystemTree::set_display_mode(DisplayMode p_display_mode) {
	if (display_mode == p_display_mode) {
		return;
	}

	display_mode = p_display_mode;
	_update_display_mode();
}

FileSystemTree::DisplayMode FileSystemTree::get_display_mode() const {
	return display_mode;
}

void FileSystemTree::set_column_settings(const Array &p_column_settings) {
}

Array FileSystemTree::get_column_settings() const {
	return Array();
}

void FileSystemTree::set_context_menu(FileContextMenu *p_menu) {
	if (context_menu == p_menu) {
		return;
	}

	if (context_menu && context_menu != p_menu) {
		context_menu->disconnect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));
	}

	context_menu = p_menu;
	if (context_menu) {
		context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &FileSystemTree::_context_menu_id_pressed));

		if (!is_connected(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw))) {
			// CanvasItem::_redraw_callback()
			connect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (!is_connected("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited))) {
			connect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (!is_connected("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected))) {
			connect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected)); // multi_selected already contains left mouse button select
		}
		if (!is_connected("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked))) {
			connect("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked));
		}
	} else {
		if (is_connected(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw))) {
			disconnect(SceneStringName(draw), callable_mp(this, &FileSystemTree::_on_draw));
		}
		if (is_connected("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited))) {
			disconnect("item_edited", callable_mp(this, &FileSystemTree::_on_item_edited));
		}
		if (is_connected("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected))) {
			disconnect("item_mouse_selected", callable_mp(this, &FileSystemTree::_on_item_mouse_selected));
		}
		if (is_connected("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked))) {
			disconnect("empty_clicked", callable_mp(this, &FileSystemTree::_on_empty_clicked));
		}
	}
}

FileContextMenu *FileSystemTree::get_context_menu() const {
	return context_menu;
}

void FileSystemTree::process_menu_id(int p_option, const Vector<String> &p_selected) {
	ERR_FAIL_NULL(context_menu);

	if (!_process_id_pressed(p_option, p_selected)) {
		context_menu->set_targets(p_selected);
		context_menu->file_option(p_option);
	}
}

void FileSystemTree::set_file_system(FileSystem *p_file_system) {
	file_system = p_file_system;
}

FileSystem *FileSystemTree::get_file_system() const {
	return file_system;
}

Vector<String> FileSystemTree::get_selected_paths() {
	Vector<String> selected_paths;

	for (const FileSystemTreeItem *item : _get_selected_items()) {
		Dictionary d = item->get_metadata(0);
		selected_paths.push_back(d["path"]);
	}

	return selected_paths;
}

Vector<String> FileSystemTree::get_uncollapsed_paths() const {
	Vector<String> paths;
	FileSystemTreeItem *root_item = get_root();
	if (root_item) {
		// BFS to find all uncollapsed paths of the file system directory.
		FileSystemTreeItem *file_system_subtree = root_item->get_first_child();
		if (file_system_subtree) {
			List<FileSystemTreeItem *> queue;
			queue.push_back(file_system_subtree);

			while (!queue.is_empty()) {
				FileSystemTreeItem *ti = queue.back()->get();
				queue.pop_back();
				// if (!ti->is_collapsed() && ti->get_child_count() > 0) {
				if (!ti->is_collapsed()) {
					Dictionary d = ti->get_metadata(0);
					if (FileSystemAccess::is_dir_type(d["type"])) {
						paths.push_back(d["path"]);
					}
				}
				for (int i = 0; i < ti->get_child_count(); i++) {
					queue.push_back(ti->get_child(i));
				}
			}
		}
	}
	return paths;
}

FileSystemTreeItem *FileSystemTree::add_root() {
	if (root) {
		root->set_metadata(0, Dictionary());
		return root;
	}

	FileSystemTreeItem *item = create_item();
	item->set_metadata(0, Dictionary());
	return item;
}

FileSystemTreeItem *FileSystemTree::add_root(const FileInfo &p_fi) {
	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["type"] = p_fi.type;

	if (root) {
		root->set_metadata(0, d);
		return root;
	}

	FileSystemTreeItem *item = create_item();
	item->set_metadata(0, d);
	return item;
}

FileSystemTreeItem *FileSystemTree::add_item(const FileInfo &p_fi, FileSystemTreeItem *p_parent, int p_index) {
	FileSystemTreeItem *item = nullptr;
	if (display_mode == DISPLAY_MODE_TREE) {
		item = _add_tree_item(p_fi, p_parent, p_index);
		// print_line("add tree item :", p_fi.path, item);
		if (item && !to_select.is_empty()) {
			int idx = to_select.find(p_fi.path);
			// print_line("find:", idx, to_select);
			if (idx >= 0) {
				FileSystemTreeItem *root_item = get_root();
				FileSystemTreeItem *parent = item->get_parent();
				while (parent && parent != root_item) {
					if (parent->is_collapsed()) {
						parent->set_collapsed(false);

						// Dictionary d = parent->get_metadata(0);
						// print_line("set uncollapsed: ", d["path"]);
					}
					parent = parent->get_parent();
				}
				item->select();
				to_select.remove_at(idx);
			}
		}
	} else if (display_mode == DISPLAY_MODE_LIST) {
		item = _add_list_item(p_fi, p_parent, p_index);
		if (item && !to_select.is_empty()) {
			int idx = to_select.find(p_fi.path);
			if (idx >= 0) {
				item->select();
				to_select.remove_at(idx);
			}
		}
	}

	if (item) {
		Dictionary d;
		d["name"] = p_fi.name;
		d["path"] = p_fi.path;
		d["type"] = p_fi.type;

		d["size"] = p_fi.size;
		d["creation_time"] = p_fi.creation_time;
		d["modified_time"] = p_fi.modified_time;
		item->set_metadata(0, d);
	}

	return item;
}

void FileSystemTree::_bind_methods() {
	ClassDB::bind_method(D_METHOD("clear"), &FileSystemTree::clear);
	ClassDB::bind_method(D_METHOD("create_item", "parent", "index"), &FileSystemTree::create_item, DEFVAL(Variant()), DEFVAL(-1));

	ClassDB::bind_method(D_METHOD("get_root"), &FileSystemTree::get_root);
	ClassDB::bind_method(D_METHOD("set_column_custom_minimum_width", "column", "min_width"), &FileSystemTree::set_column_custom_minimum_width);
	ClassDB::bind_method(D_METHOD("set_column_expand", "column", "expand"), &FileSystemTree::set_column_expand);
	ClassDB::bind_method(D_METHOD("set_column_expand_ratio", "column", "ratio"), &FileSystemTree::set_column_expand_ratio);
	ClassDB::bind_method(D_METHOD("set_column_clip_content", "column", "enable"), &FileSystemTree::set_column_clip_content);
	ClassDB::bind_method(D_METHOD("is_column_expanding", "column"), &FileSystemTree::is_column_expanding);
	ClassDB::bind_method(D_METHOD("is_column_clipping_content", "column"), &FileSystemTree::is_column_clipping_content);
	ClassDB::bind_method(D_METHOD("get_column_expand_ratio", "column"), &FileSystemTree::get_column_expand_ratio);

	ClassDB::bind_method(D_METHOD("get_column_width", "column"), &FileSystemTree::get_column_width);

	ClassDB::bind_method(D_METHOD("set_hide_root", "enable"), &FileSystemTree::set_hide_root);
	ClassDB::bind_method(D_METHOD("is_root_hidden"), &FileSystemTree::is_root_hidden);
	ClassDB::bind_method(D_METHOD("get_next_selected", "from"), &FileSystemTree::get_next_selected);
	ClassDB::bind_method(D_METHOD("get_selected"), &FileSystemTree::get_selected);
	ClassDB::bind_method(D_METHOD("set_selected", "item"), &FileSystemTree::set_selected);
	ClassDB::bind_method(D_METHOD("get_pressed_button"), &FileSystemTree::get_pressed_button);
	ClassDB::bind_method(D_METHOD("deselect_all"), &FileSystemTree::deselect_all);

	ClassDB::bind_method(D_METHOD("set_columns", "amount"), &FileSystemTree::set_columns);
	ClassDB::bind_method(D_METHOD("get_columns"), &FileSystemTree::get_columns);

	ClassDB::bind_method(D_METHOD("get_edited"), &FileSystemTree::get_edited);
	ClassDB::bind_method(D_METHOD("edit_selected", "force_edit"), &FileSystemTree::edit_selected, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_custom_popup_rect"), &FileSystemTree::get_custom_popup_rect);
	ClassDB::bind_method(D_METHOD("get_item_area_rect", "item", "column", "button_index"), &FileSystemTree::get_item_rect, DEFVAL(-1), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_item_at_position", "position"), &FileSystemTree::get_item_at_position);
	ClassDB::bind_method(D_METHOD("get_column_at_position", "position"), &FileSystemTree::get_column_at_position);
	ClassDB::bind_method(D_METHOD("get_drop_section_at_position", "position"), &FileSystemTree::get_drop_section_at_position);

	ClassDB::bind_method(D_METHOD("ensure_cursor_is_visible"), &FileSystemTree::ensure_cursor_is_visible);

	ClassDB::bind_method(D_METHOD("set_column_titles_visible", "visible"), &FileSystemTree::set_column_titles_visible);
	ClassDB::bind_method(D_METHOD("are_column_titles_visible"), &FileSystemTree::are_column_titles_visible);

	ClassDB::bind_method(D_METHOD("set_column_title", "column", "title"), &FileSystemTree::set_column_title);
	ClassDB::bind_method(D_METHOD("get_column_title", "column"), &FileSystemTree::get_column_title);

	ClassDB::bind_method(D_METHOD("set_column_title_tooltip_text", "column", "tooltip_text"), &FileSystemTree::set_column_title_tooltip_text);
	ClassDB::bind_method(D_METHOD("get_column_title_tooltip_text", "column"), &FileSystemTree::get_column_title_tooltip_text);

	ClassDB::bind_method(D_METHOD("set_column_title_alignment", "column", "title_alignment"), &FileSystemTree::set_column_title_alignment);
	ClassDB::bind_method(D_METHOD("get_column_title_alignment", "column"), &FileSystemTree::get_column_title_alignment);

	ClassDB::bind_method(D_METHOD("set_column_title_direction", "column", "direction"), &FileSystemTree::set_column_title_direction);
	ClassDB::bind_method(D_METHOD("get_column_title_direction", "column"), &FileSystemTree::get_column_title_direction);

	ClassDB::bind_method(D_METHOD("set_column_title_language", "column", "language"), &FileSystemTree::set_column_title_language);
	ClassDB::bind_method(D_METHOD("get_column_title_language", "column"), &FileSystemTree::get_column_title_language);

	ClassDB::bind_method(D_METHOD("get_scroll"), &FileSystemTree::get_scroll);
	ClassDB::bind_method(D_METHOD("scroll_to_item", "item", "center_on_item"), &FileSystemTree::scroll_to_item, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("set_h_scroll_enabled", "h_scroll"), &FileSystemTree::set_h_scroll_enabled);
	ClassDB::bind_method(D_METHOD("is_h_scroll_enabled"), &FileSystemTree::is_h_scroll_enabled);

	ClassDB::bind_method(D_METHOD("set_v_scroll_enabled", "h_scroll"), &FileSystemTree::set_v_scroll_enabled);
	ClassDB::bind_method(D_METHOD("is_v_scroll_enabled"), &FileSystemTree::is_v_scroll_enabled);

	ClassDB::bind_method(D_METHOD("set_scroll_hint_mode", "scroll_hint_mode"), &FileSystemTree::set_scroll_hint_mode);
	ClassDB::bind_method(D_METHOD("get_scroll_hint_mode"), &FileSystemTree::get_scroll_hint_mode);

	ClassDB::bind_method(D_METHOD("set_tile_scroll_hint", "tile_scroll_hint"), &FileSystemTree::set_tile_scroll_hint);
	ClassDB::bind_method(D_METHOD("is_scroll_hint_tiled"), &FileSystemTree::is_scroll_hint_tiled);

	ClassDB::bind_method(D_METHOD("set_hide_folding", "hide"), &FileSystemTree::set_hide_folding);
	ClassDB::bind_method(D_METHOD("is_folding_hidden"), &FileSystemTree::is_folding_hidden);

	ClassDB::bind_method(D_METHOD("set_enable_recursive_folding", "enable"), &FileSystemTree::set_enable_recursive_folding);
	ClassDB::bind_method(D_METHOD("is_recursive_folding_enabled"), &FileSystemTree::is_recursive_folding_enabled);

	ClassDB::bind_method(D_METHOD("set_enable_drag_unfolding", "enable"), &FileSystemTree::set_enable_drag_unfolding);
	ClassDB::bind_method(D_METHOD("is_drag_unfolding_enabled"), &FileSystemTree::is_drag_unfolding_enabled);

	ClassDB::bind_method(D_METHOD("set_drop_mode_flags", "flags"), &FileSystemTree::set_drop_mode_flags);
	ClassDB::bind_method(D_METHOD("get_drop_mode_flags"), &FileSystemTree::get_drop_mode_flags);

	ClassDB::bind_method(D_METHOD("set_allow_rmb_select", "allow"), &FileSystemTree::set_allow_rmb_select);
	ClassDB::bind_method(D_METHOD("get_allow_rmb_select"), &FileSystemTree::get_allow_rmb_select);

	ClassDB::bind_method(D_METHOD("set_allow_reselect", "allow"), &FileSystemTree::set_allow_reselect);
	ClassDB::bind_method(D_METHOD("get_allow_reselect"), &FileSystemTree::get_allow_reselect);

	ClassDB::bind_method(D_METHOD("set_allow_search", "allow"), &FileSystemTree::set_allow_search);
	ClassDB::bind_method(D_METHOD("get_allow_search"), &FileSystemTree::get_allow_search);

	ClassDB::bind_method(D_METHOD("set_auto_tooltip", "enable"), &FileSystemTree::set_auto_tooltip);
	ClassDB::bind_method(D_METHOD("is_auto_tooltip_enabled"), &FileSystemTree::is_auto_tooltip_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "columns"), "set_columns", "get_columns");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "column_titles_visible"), "set_column_titles_visible", "are_column_titles_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_reselect"), "set_allow_reselect", "get_allow_reselect");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_rmb_select"), "set_allow_rmb_select", "get_allow_rmb_select");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_search"), "set_allow_search", "get_allow_search");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_folding"), "set_hide_folding", "is_folding_hidden");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_recursive_folding"), "set_enable_recursive_folding", "is_recursive_folding_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_drag_unfolding"), "set_enable_drag_unfolding", "is_drag_unfolding_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_root"), "set_hide_root", "is_root_hidden");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "drop_mode_flags", PROPERTY_HINT_FLAGS, "On Item,In Between"), "set_drop_mode_flags", "get_drop_mode_flags");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_tooltip"), "set_auto_tooltip", "is_auto_tooltip_enabled");
	ADD_GROUP("Scroll", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_horizontal_enabled"), "set_h_scroll_enabled", "is_h_scroll_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_vertical_enabled"), "set_v_scroll_enabled", "is_v_scroll_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "scroll_hint_mode", PROPERTY_HINT_ENUM, "Disabled,Both,Top,Bottom"), "set_scroll_hint_mode", "get_scroll_hint_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tile_scroll_hint"), "set_tile_scroll_hint", "is_scroll_hint_tiled");

	ADD_SIGNAL(MethodInfo("item_selected", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "FileSystemTreeItem"), PropertyInfo(Variant::BOOL, "selected")));
	ADD_SIGNAL(MethodInfo("item_mouse_selected", PropertyInfo(Variant::VECTOR2, "mouse_position"), PropertyInfo(Variant::INT, "mouse_button_index")));
	ADD_SIGNAL(MethodInfo("empty_clicked", PropertyInfo(Variant::VECTOR2, "click_position"), PropertyInfo(Variant::INT, "mouse_button_index")));
	ADD_SIGNAL(MethodInfo("item_edited"));
	ADD_SIGNAL(MethodInfo("custom_item_clicked", PropertyInfo(Variant::INT, "mouse_button_index")));
	ADD_SIGNAL(MethodInfo("item_icon_double_clicked"));
	ADD_SIGNAL(MethodInfo("item_collapsed", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "FileSystemTreeItem")));
	ADD_SIGNAL(MethodInfo("check_propagated_to_item", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "FileSystemTreeItem"), PropertyInfo(Variant::INT, "column")));
	ADD_SIGNAL(MethodInfo("custom_popup_edited", PropertyInfo(Variant::BOOL, "arrow_clicked")));
	ADD_SIGNAL(MethodInfo("item_activated"));
	ADD_SIGNAL(MethodInfo("column_title_clicked", PropertyInfo(Variant::INT, "column"), PropertyInfo(Variant::INT, "mouse_button_index")));
	ADD_SIGNAL(MethodInfo("nothing_selected"));

	BIND_ENUM_CONSTANT(DROP_MODE_DISABLED);
	BIND_ENUM_CONSTANT(DROP_MODE_ON_ITEM);
	BIND_ENUM_CONSTANT(DROP_MODE_INBETWEEN);

	BIND_ENUM_CONSTANT(SCROLL_HINT_MODE_DISABLED);
	BIND_ENUM_CONSTANT(SCROLL_HINT_MODE_BOTH);
	BIND_ENUM_CONSTANT(SCROLL_HINT_MODE_TOP);
	BIND_ENUM_CONSTANT(SCROLL_HINT_MODE_BOTTOM);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, panel_style, "panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, focus_style, "focus");

	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT, FileSystemTree, font);
	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT_SIZE, FileSystemTree, font_size);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT, FileSystemTree, tb_font, "title_button_font");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT_SIZE, FileSystemTree, tb_font_size, "title_button_font_size");

	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, hovered);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, hovered_dimmed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, hovered_selected);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, hovered_selected_focus);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, selected);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, selected_focus);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, cursor);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, cursor_unfocus, "cursor_unfocused");
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, button_hover);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, button_pressed);

	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, checked);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, unchecked);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, checked_disabled);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, unchecked_disabled);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, indeterminate);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, indeterminate_disabled);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, arrow);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, arrow_collapsed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, arrow_collapsed_mirrored);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, select_arrow);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, updown);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, scroll_hint);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, scroll_hint_color);

	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, arrow_down);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FileSystemTree, arrow_up);

	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, custom_button);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, custom_button_hover);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, custom_button_pressed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, custom_button_font_highlight);

	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_hovered_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_hovered_dimmed_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_hovered_selected_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_selected_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_disabled_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, drop_position_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, h_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, v_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, inner_item_margin_bottom);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, inner_item_margin_left);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, inner_item_margin_right);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, inner_item_margin_top);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, item_margin);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, check_h_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, icon_h_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, button_margin);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, icon_max_width);

	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, font_outline_color);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, font_outline_size, "outline_size");

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, draw_guides);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, guide_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, draw_relationship_lines);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, relationship_line_width);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, parent_hl_line_width);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, children_hl_line_width);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, parent_hl_line_margin);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, relationship_line_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, parent_hl_line_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, children_hl_line_color);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, dragging_unfold_wait_msec);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scroll_border);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scroll_speed);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_margin_top);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_margin_right);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_margin_bottom);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_margin_left);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_h_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FileSystemTree, scrollbar_v_separation);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, title_button, "title_button_normal");
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, title_button_pressed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FileSystemTree, title_button_hover);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FileSystemTree, title_button_color);

	ADD_CLASS_DEPENDENCY("HScrollBar");
	ADD_CLASS_DEPENDENCY("HSlider");
	ADD_CLASS_DEPENDENCY("LineEdit");
	ADD_CLASS_DEPENDENCY("Popup");
	ADD_CLASS_DEPENDENCY("TextEdit");
	ADD_CLASS_DEPENDENCY("Timer");
	ADD_CLASS_DEPENDENCY("VScrollBar");
}

FileSystemTree::FileSystemTree() {
	for (int i = 0; i < default_column_count; i++) {
		column_settings[i] = default_column_settings[i];
	}

	columns.resize(1);

	set_focus_mode(FOCUS_ALL);

	RenderingServer *rs = RenderingServer::get_singleton();

	content_ci = rs->canvas_item_create();
	rs->canvas_item_set_parent(content_ci, get_canvas_item());
	rs->canvas_item_set_use_parent_material(content_ci, true);

	header_ci = rs->canvas_item_create();
	rs->canvas_item_set_parent(header_ci, get_canvas_item());
	rs->canvas_item_set_use_parent_material(header_ci, true);

	popup_editor = memnew(Popup);
	add_child(popup_editor, false, INTERNAL_MODE_FRONT);

	popup_editor_vb = memnew(VBoxContainer);
	popup_editor_vb->add_theme_constant_override("separation", 0);
	popup_editor_vb->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	popup_editor->add_child(popup_editor_vb);

	line_editor = memnew(LineEdit);
	line_editor->set_theme_type_variation("TreeLineEdit");
	line_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	line_editor->hide();
	popup_editor_vb->add_child(line_editor);

	text_editor = memnew(TextEdit);
	text_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	text_editor->hide();
	popup_editor_vb->add_child(text_editor);

	h_scroll = memnew(HScrollBar);
	v_scroll = memnew(VScrollBar);

	add_child(h_scroll, false, INTERNAL_MODE_FRONT);
	add_child(v_scroll, false, INTERNAL_MODE_FRONT);

	dropping_unfold_timer = memnew(Timer);
	dropping_unfold_timer->set_one_shot(true);
	dropping_unfold_timer->connect("timeout", callable_mp(this, &FileSystemTree::_on_dropping_unfold_timer_timeout));
	add_child(dropping_unfold_timer);

	h_scroll->connect(SceneStringName(value_changed), callable_mp(this, &FileSystemTree::_scroll_moved));
	v_scroll->connect(SceneStringName(value_changed), callable_mp(this, &FileSystemTree::_scroll_moved));
	line_editor->connect(SceneStringName(text_submitted), callable_mp(this, &FileSystemTree::_line_editor_submit));
	text_editor->connect(SceneStringName(gui_input), callable_mp(this, &FileSystemTree::_text_editor_gui_input));
	popup_editor->connect("popup_hide", callable_mp(this, &FileSystemTree::_text_editor_popup_modal_close));

	set_notify_transform(true);

	set_mouse_filter(MOUSE_FILTER_STOP);

	set_clip_contents(true);

	callable_mp(this, &FileSystemTree::_update_display_mode).call_deferred();
}

FileSystemTree::~FileSystemTree() {
	to_select.clear();

	if (root) {
		memdelete(root);
	}
	RenderingServer::get_singleton()->free_rid(content_ci);
	RenderingServer::get_singleton()->free_rid(header_ci);
}
