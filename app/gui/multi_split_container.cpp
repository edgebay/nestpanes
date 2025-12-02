#include "multi_split_container.h"

#include "scene/gui/texture_rect.h"
#include "scene/main/viewport.h"
#include "scene/theme/theme_db.h"

void MultiSplitContainerDragger::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	MultiSplitContainer *sc = Object::cast_to<MultiSplitContainer>(get_parent());

	if (sc->get_child_count(false) <= 0 || !sc->dragging_enabled) {
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				dragging = true;
				sc->emit_signal(SNAME("drag_started"));
				drag_ofs = split_offset;
				if (sc->vertical) {
					drag_from = get_transform().xform(mb->get_position()).y;
				} else {
					drag_from = get_transform().xform(mb->get_position()).x;
				}
			} else {
				dragging = false;
				queue_redraw();
				sc->emit_signal(SNAME("drag_ended"));
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		if (!dragging) {
			return;
		}

		if (prev_offset < 0 || next_offset <= prev_offset) {
			return;
		}

		int min_offset = prev_offset + sc->min_spacing;
		int max_offset = next_offset - sc->min_spacing;
		if (max_offset <= min_offset) {
			return;
		}

		Vector2i in_parent_pos = get_transform().xform(mm->get_position());
		split_offset = drag_ofs + ((sc->vertical ? in_parent_pos.y : in_parent_pos.x) - drag_from);
		split_offset = CLAMP(split_offset, min_offset, max_offset);
		sc->queue_sort();
		sc->emit_signal(SNAME("dragged"), split_offset);
	}
}

Control::CursorShape MultiSplitContainerDragger::get_cursor_shape(const Point2 &p_pos) const {
	MultiSplitContainer *sc = Object::cast_to<MultiSplitContainer>(get_parent());
	return (sc->vertical ? CURSOR_VSPLIT : CURSOR_HSPLIT);
}

void MultiSplitContainerDragger::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_MOUSE_ENTER: {
			mouse_inside = true;
			MultiSplitContainer *sc = Object::cast_to<MultiSplitContainer>(get_parent());
			if (sc->theme_cache.autohide) {
				queue_redraw();
			}
		} break;
		case NOTIFICATION_MOUSE_EXIT: {
			mouse_inside = false;
			MultiSplitContainer *sc = Object::cast_to<MultiSplitContainer>(get_parent());
			if (sc->theme_cache.autohide) {
				queue_redraw();
			}
		} break;
		case NOTIFICATION_DRAW: {
			MultiSplitContainer *sc = Object::cast_to<MultiSplitContainer>(get_parent());
			draw_style_box(sc->theme_cache.split_bar_background, split_bar_rect);
			if (dragging || mouse_inside || !sc->theme_cache.autohide) {
				Ref<Texture2D> tex = sc->_get_grabber_icon();
				float available_size = sc->vertical ? (sc->get_size().x - tex->get_size().x) : (sc->get_size().y - tex->get_size().y);
				if (available_size - sc->drag_area_margin_begin - sc->drag_area_margin_end > 0) { // Draw the grabber only if it fits.
					draw_texture(tex, (split_bar_rect.get_position() + (split_bar_rect.get_size() - tex->get_size()) * 0.5));
				}
			}
			if (sc->show_drag_area && Engine::get_singleton()->is_editor_hint()) {
				draw_rect(Rect2(Vector2(0, 0), get_size()), sc->dragging_enabled ? Color(1, 1, 0, 0.3) : Color(1, 0, 0, 0.3));
			}
		} break;
	}
}

MultiSplitContainerDragger::MultiSplitContainerDragger() {
	set_focus_mode(FOCUS_ACCESSIBILITY);
}

Ref<Texture2D> MultiSplitContainer::_get_grabber_icon() const {
	if (vertical) {
		return theme_cache.grabber_icon_v;
	} else {
		return theme_cache.grabber_icon_h;
	}
}

void MultiSplitContainer::_compute_split_offset() {
	int axis_index = vertical ? 1 : 0;
	int sep = _get_separation();

	if (!split_map.is_empty()) {
		for (KeyValue<Control *, MultiSplitContainerDragger *> &E : split_map) {
			int split_offset = (E.key->get_size()[axis_index] - sep) * 0.5;
			E.value->split_offset = E.key->get_position()[axis_index] + split_offset;
			print_line("compute: ", E.key->get_rect(), E.value->split_offset);
		}
		split_map.clear();

		draggers.sort_custom<DraggerComparator>();
	}

	int prev_offset = 0;
	int next_offset = get_size()[axis_index];
	int dragger_count = draggers.size();
	if (dragger_count == 0) {
		return;
	} else if (dragger_count == 1) {
		MultiSplitContainerDragger *dragger = draggers[0];
		dragger->prev_offset = prev_offset;
		dragger->next_offset = next_offset;
	} else {
		MultiSplitContainerDragger *dragger = nullptr;
		for (int i = 1; i < dragger_count; i++) {
			MultiSplitContainerDragger *prev_dragger = draggers[i - 1];
			dragger = draggers[i];
			prev_dragger->prev_offset = prev_offset;
			prev_dragger->next_offset = dragger->split_offset;

			prev_offset = prev_dragger->split_offset;
		}
		dragger->prev_offset = prev_offset;
		dragger->next_offset = next_offset;
	}
}

int MultiSplitContainer::_get_separation() const {
	Ref<Texture2D> g = _get_grabber_icon();
	return MAX(theme_cache.separation, vertical ? g->get_height() : g->get_width());
}

void MultiSplitContainer::_resort() {
	// TODO: check child_count and draggers.size()

	int child_count = get_child_count(false); // TODO: sortable
	print_line("child count: ", itos(child_count), get_size());

	if (child_count == 0) {
		_clear_draggers();
		return;
	} else if (child_count == 1) {
		Control *control = as_sortable_control(get_child(0, false), SortableVisibilityMode::VISIBLE);
		fit_child_in_rect(control, Rect2(Point2(), get_size()));
		return;
	}

	// More than one child.
	ERR_FAIL_COND_MSG(child_count != draggers.size() + 1, vformat("Invalid child count %d, %d.", child_count, draggers.size()));

	_compute_split_offset();

	int sep = _get_separation();
	const int dragger_ctrl_size = MAX(sep, theme_cache.minimum_grab_thickness);
	float split_bar_offset = (dragger_ctrl_size - sep) * 0.5;

	int offset = 0;
	Rect2 rect;

	// Move the children and draggers.
	for (int i = 0; i < draggers.size(); i++) {
		MultiSplitContainerDragger *dragger = draggers[i];
		Control *child = as_sortable_control(get_child(i, false), SortableVisibilityMode::VISIBLE);

		if (dragger->split_offset >= 0) { // TODO
			// dragger->set_mouse_filter(dragging_enabled ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE);

			int computed_split_offset = dragger->split_offset;
			if (vertical) { // 亖
				Rect2 split_bar_rect = Rect2(drag_area_margin_begin, computed_split_offset, get_size().width - drag_area_margin_begin - drag_area_margin_end, sep);
				dragger->set_rect(Rect2(split_bar_rect.position.x, split_bar_rect.position.y - split_bar_offset + drag_area_offset, split_bar_rect.size.x, dragger_ctrl_size));
				dragger->split_bar_rect = Rect2(Vector2(0.0, int(split_bar_offset) - drag_area_offset), split_bar_rect.size);

				rect = Rect2(Point2(0, offset), Size2(get_size().width, computed_split_offset - offset));
				fit_child_in_rect(child, rect);
			} else { // ||||
				Rect2 split_bar_rect = Rect2(computed_split_offset, drag_area_margin_begin, sep, get_size().height - drag_area_margin_begin - drag_area_margin_end);
				dragger->set_rect(Rect2(split_bar_rect.position.x - split_bar_offset + drag_area_offset, split_bar_rect.position.y, dragger_ctrl_size, split_bar_rect.size.y));
				dragger->split_bar_rect = Rect2(Vector2(int(split_bar_offset) - drag_area_offset, 0.0), split_bar_rect.size);

				rect = Rect2(Point2(offset, 0), Size2(computed_split_offset - offset, get_size().height));
				fit_child_in_rect(child, rect);
			}

			print_line(itos(i), ", offset ", computed_split_offset, rect, child);

			offset = computed_split_offset + sep;

			dragger->queue_redraw();
		}
	}

	// Move the last child.
	Control *last = as_sortable_control(get_child(-1, false), SortableVisibilityMode::VISIBLE);
	ERR_FAIL_COND_MSG(last == nullptr, vformat("Invalid next."));
	if (vertical) {
		rect = Rect2(Point2(0, offset), Size2(get_size().width, get_size().height - offset));
		fit_child_in_rect(last, rect);
	} else {
		rect = Rect2(Point2(offset, 0), Size2(get_size().width - offset, get_size().height));
		fit_child_in_rect(last, rect);
	}
	print_line("rect: ", rect);

	queue_redraw();
}

void MultiSplitContainer::_create_sub_split(Control *p_control, Control *p_from, SplitDirection p_direction) {
	// Create a child split container to replace the target child node.
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);
	split_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	int control_index = p_from->get_index(false);
	remove_child(p_from);
	add_child(split_container);
	move_child(split_container, control_index);
	print_line("- sub split ", control_index, p_from, p_control);

	Node *owner = get_owner();
	split_container->set_owner(owner);
	split_container->split(p_control, p_from, p_direction);

	// Reset owner.
	p_from->set_owner(owner);
	// TODO: check child type?
	for (int i = 0; i < p_from->get_child_count(false); i++) {
		Node *child = p_from->get_child(i, false);
		child->set_owner(owner);
	}
}

void MultiSplitContainer::_clear_draggers() {
	if (draggers.is_empty()) {
		return;
	}

	for (auto dragger : draggers) {
		dragger->queue_free();
	}
	draggers.clear();
}

void MultiSplitContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_SORT_CHILDREN: {
			_resort();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			update_minimum_size();
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
			queue_sort();
		} break;
	}
}

void MultiSplitContainer::set_vertical(bool p_vertical) {
	// if (vertical == p_vertical) {
	// 	return;
	// }
	vertical = p_vertical;
	// update_minimum_size();
	// _resort();
}

bool MultiSplitContainer::is_vertical() const {
	return vertical;
}

void MultiSplitContainer::set_split_offsets(TypedArray<int> p_offsets) {
	_clear_draggers();

	for (int i = 0; i < p_offsets.size(); i++) {
		MultiSplitContainerDragger *dragging_area_control = memnew(MultiSplitContainerDragger);
		add_child(dragging_area_control, false, Node::INTERNAL_MODE_BACK);
		draggers.push_back(dragging_area_control);

		dragging_area_control->set_split_offset(p_offsets[i]);
	}
	queue_redraw();
}

TypedArray<int> MultiSplitContainer::get_split_offsets() const {
	TypedArray<int> offsets;
	for (auto dragger : draggers) {
		offsets.push_back(dragger->get_split_offset());
	}
	return offsets;
}

Size2 MultiSplitContainer::get_minimum_size() const {
	Size2i minimum;
	int sep = _get_separation();

	bool first = true;

	for (int i = 0; i < get_child_count(false); i++) {
		Control *c = as_sortable_control(get_child(i, false), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}

		Size2i size = c->get_combined_minimum_size();

		if (vertical) { /* VERTICAL */
			minimum.height += size.height + (first ? 0 : sep);
			minimum.width = MAX(minimum.width, size.width);
		} else { /* HORIZONTAL */
			minimum.width += size.width + (first ? 0 : sep);
			minimum.height = MAX(minimum.height, size.height);
		}

		first = false;
	}

	return minimum;
}

Vector<int> MultiSplitContainer::get_allowed_size_flags_horizontal() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (!vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> MultiSplitContainer::get_allowed_size_flags_vertical() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

/*
 * 1. no child
 * 2. rect 1 -> rect 1 | rect 2
 * 3. rect 1 | rect 2 -> rect 1 | rect 2 | rect 3
 */
void MultiSplitContainer::split(Control *p_control, Control *p_from, SplitDirection p_direction) {
	int prev_child_count = get_child_count(false);
	print_line("split: ", prev_child_count, p_control, p_from, p_direction);

	// No child.
	if (prev_child_count == 0) {
		if (p_from) {
			add_child(p_from);
			split(p_control, p_from, p_direction);
		} else {
			add_child(p_control);
		}
		return;
	} else if (prev_child_count == 1) {
		// Set vertical.
		if (p_direction == SPLIT_UP || p_direction == SPLIT_DOWN) {
			vertical = true;
		} else if (p_direction == SPLIT_LEFT || p_direction == SPLIT_RIGHT) {
			vertical = false;
		}
	}
	ERR_FAIL_COND_MSG(p_from == nullptr, vformat("Split failed, from 0x%08X, new 0x%08X.", p_from, p_control));

	// Add control and dragger.
	int control_index = -1;
	Rect2 rect = Rect2(Vector2(0, 0), get_size());
	for (int i = 0; i < get_child_count(false); i++) {
		Control *c = as_sortable_control(get_child(i, false), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}
		if (c == p_from) {
			rect = p_from->get_rect();
			if (vertical) {
				if (p_direction == SPLIT_UP) {
					control_index = i;
				} else if (p_direction == SPLIT_DOWN) {
					control_index = i + 1;
				} else {
					_create_sub_split(p_control, p_from, p_direction);
					return;
				}
			} else {
				if (p_direction == SPLIT_LEFT) {
					control_index = i;
				} else if (p_direction == SPLIT_RIGHT) {
					control_index = i + 1;
				} else {
					_create_sub_split(p_control, p_from, p_direction);
					return;
				}
			}
		}
	}
	ERR_FAIL_COND_MSG(control_index < 0, vformat("Split failed, from 0x%08X, new 0x%08X.", p_from, p_control));

	add_child(p_control);
	// p_control->connect();	// TODO: handle free signal

	if (control_index >= 0 && control_index < prev_child_count) {
		move_child(p_control, control_index);
	}

	MultiSplitContainerDragger *dragging_area_control = memnew(MultiSplitContainerDragger);
	add_child(dragging_area_control, false, Node::INTERNAL_MODE_BACK);
	draggers.push_back(dragging_area_control);

	split_map.insert(p_from, dragging_area_control);

	queue_redraw();
}

void MultiSplitContainer::remove(Control *p_control) {
	int child_count = get_child_count(false); // TODO: sortable
	print_line("remove child, count ", child_count, p_control);
	if (child_count == 1) {
		// No dragger
		remove_child(p_control);
		print_line("remove last child ", this);

		// TODO: use child_order_changed
		emit_signal(SNAME("emptied"));
		return;
	} else if (child_count == 2) {
		// Replace itself with its last child in the parent split container.
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(get_parent());
		if (split_container) {
			remove_child(p_control);

			Control *c = as_sortable_control(get_child(0, false), SortableVisibilityMode::VISIBLE);
			remove_child(c);

			int index_in_parent = get_index(false);
			split_container->remove_child(this);
			split_container->add_child(c);
			split_container->move_child(c, index_in_parent);

			// Reset owner.
			Node *owner = split_container->get_owner();
			c->set_owner(owner);
			// TODO: check child type?
			for (int i = 0; i < c->get_child_count(false); i++) {
				Node *child = c->get_child(i, false);
				child->set_owner(owner);
			}
			queue_free();

			print_line("- sub child ", index_in_parent, this, c);
			return;
		}
	}

	for (int i = 0; i < child_count; i++) {
		Control *c = as_sortable_control(get_child(i, false), SortableVisibilityMode::VISIBLE);
		if (c == p_control) {
			int dragger_index = i;
			if (dragger_index >= draggers.size()) {
				dragger_index = draggers.size() - 1;
			}
			MultiSplitContainerDragger *dragger = draggers[dragger_index];
			remove_child(dragger);
			dragger->queue_free();
			draggers.remove_at(dragger_index);

			remove_child(c);
			break;
		}
	}
}

void MultiSplitContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_vertical", "vertical"), &MultiSplitContainer::set_vertical);
	ClassDB::bind_method(D_METHOD("is_vertical"), &MultiSplitContainer::is_vertical);

	ClassDB::bind_method(D_METHOD("set_split_offsets", "offsets"), &MultiSplitContainer::set_split_offsets);
	ClassDB::bind_method(D_METHOD("get_split_offsets"), &MultiSplitContainer::get_split_offsets);

	ADD_SIGNAL(MethodInfo("dragged", PropertyInfo(Variant::INT, "offset")));
	ADD_SIGNAL(MethodInfo("drag_started"));
	ADD_SIGNAL(MethodInfo("drag_ended"));
	ADD_SIGNAL(MethodInfo("emptied"));

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vertical"), "set_vertical", "is_vertical");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "offsets"), "set_split_offsets", "get_split_offsets");

	// ADD_GROUP("Drag Area", "drag_area_");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "drag_area_margin_begin", PROPERTY_HINT_NONE, "suffix:px"), "set_drag_area_margin_begin", "get_drag_area_margin_begin");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "drag_area_margin_end", PROPERTY_HINT_NONE, "suffix:px"), "set_drag_area_margin_end", "get_drag_area_margin_end");
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "drag_area_offset", PROPERTY_HINT_NONE, "suffix:px"), "set_drag_area_offset", "get_drag_area_offset");

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MultiSplitContainer, separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MultiSplitContainer, minimum_grab_thickness);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MultiSplitContainer, autohide);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, MultiSplitContainer, grabber_icon_h, "h_grabber");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_ICON, MultiSplitContainer, grabber_icon_v, "v_grabber");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, MultiSplitContainer, split_bar_background, "split_bar_background");
}

MultiSplitContainer::MultiSplitContainer(bool p_vertical) {
	vertical = p_vertical;
}

MultiSplitContainer::~MultiSplitContainer() {
	draggers.clear();
}
