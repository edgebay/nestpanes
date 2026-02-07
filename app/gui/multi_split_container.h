#pragma once

#include "scene/gui/split_container.h"

class MultiSplitContainer : public SplitContainer {
	GDCLASS(MultiSplitContainer, SplitContainer);

public:
	enum SplitDirection {
		SPLIT_UP,
		SPLIT_DOWN,
		SPLIT_LEFT,
		SPLIT_RIGHT,
	};

private:
	void _create_sub_split(Control *p_control, Control *p_from, SplitDirection p_direction);
	void _remove_sub_split(Control *p_control, Control *p_parent);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	static MultiSplitContainer *get_root_split_container(MultiSplitContainer *p_split);

	void split(Control *p_control, Control *p_from = nullptr, SplitDirection p_direction = SPLIT_RIGHT);
	void remove(Control *p_control);

	MultiSplitContainer();
	virtual ~MultiSplitContainer();
};
