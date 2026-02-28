#include "multi_split_container.h"

void MultiSplitContainer::_create_sub_split(Control *p_control, Control *p_from, SplitDirection p_direction) {
	// Create a child split container to replace the target child node.
	MultiSplitContainer *split_container = memnew(MultiSplitContainer);
	split_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	split_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	int control_index = p_from->get_index(false);
	remove_child(p_from);
	add_child(split_container);
	move_child(split_container, control_index);

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

void MultiSplitContainer::_remove_sub_split(Control *p_control, Control *p_parent) {
	remove_child(p_control);

	Control *c = as_sortable_control(get_child(0, false), SortableVisibilityMode::VISIBLE);
	remove_child(c);

	int index_in_parent = get_index(false);
	p_parent->remove_child(this);
	p_parent->add_child(c);
	p_parent->move_child(c, index_in_parent);

	// Reset owner.
	Node *owner = p_parent->get_owner();
	c->set_owner(owner);
	// TODO: check child type?
	for (int i = 0; i < c->get_child_count(false); i++) {
		Node *child = c->get_child(i, false);
		child->set_owner(owner);
	}
	queue_free();
}

MultiSplitContainer *MultiSplitContainer::get_root_split_container(MultiSplitContainer *p_split) {
	MultiSplitContainer *root = p_split;
	Control *parent = p_split->get_parent_control();
	while (parent) {
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(parent);
		if (!split_container) {
			return root;
		}
		root = split_container;
		parent = parent->get_parent_control();
	}
	return root;
}

/*
 * 1. no child
 * 2. rect 1 -> rect 1 | rect 2
 * 3. rect 1 | rect 2 -> rect 1 | rect 2 | rect 3
 */
void MultiSplitContainer::split(Control *p_control, Control *p_from, SplitDirection p_direction) {
	int prev_child_count = get_child_count(false);

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
			set_vertical(true);
		} else if (p_direction == SPLIT_LEFT || p_direction == SPLIT_RIGHT) {
			set_vertical(false);
		}
	}
	ERR_FAIL_COND_MSG(p_from == nullptr, vformat("Split failed, from 0x%08X, new 0x%08X.", p_from, p_control));

	int control_index = -1;
	for (int i = 0; i < get_child_count(false); i++) {
		Control *c = as_sortable_control(get_child(i, false), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}
		if (c == p_from) {
			if (is_vertical()) {
				if (p_direction == SPLIT_UP) {
					control_index = i;
				} else if (p_direction == SPLIT_DOWN) {
					control_index = i + 1;
				} else {
					_create_sub_split(p_control, p_from, p_direction); // TODO: is_fixed, vertical
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
}

void MultiSplitContainer::remove(Control *p_control) {
	int child_count = get_child_count(false); // TODO: sortable
	if (child_count == 1) {
		remove_child(p_control);

		// TODO: use child_order_changed
		emit_signal(SNAME("emptied"));
		return;
	} else if (child_count == 2) {
		// Replace itself with its last child in the parent split container.
		MultiSplitContainer *split_container = Object::cast_to<MultiSplitContainer>(get_parent());
		if (split_container) {
			_remove_sub_split(p_control, split_container);
			return;
		}
	}

	remove_child(p_control);
}

void MultiSplitContainer::_bind_methods() {
	ADD_SIGNAL(MethodInfo("emptied"));
}

MultiSplitContainer::MultiSplitContainer() {
}

MultiSplitContainer::~MultiSplitContainer() {
}
