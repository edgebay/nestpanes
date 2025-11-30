#pragma once

#include "scene/gui/container.h"

class TextureRect;

class MultiSplitContainerDragger : public Control {
	GDCLASS(MultiSplitContainerDragger, Control);
	friend class MultiSplitContainer;
	Rect2 split_bar_rect;
	int split_offset = -1;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

private:
	bool dragging = false;
	int drag_from = 0;
	int drag_ofs = 0;
	bool mouse_inside = false;

public:
	virtual CursorShape get_cursor_shape(const Point2 &p_pos = Point2i()) const override;

	int get_offset() const { return split_offset; }

	MultiSplitContainerDragger();
};

class MultiSplitContainer : public Container {
	GDCLASS(MultiSplitContainer, Container);
	friend class MultiSplitContainerDragger;

public:
	enum SplitDirection {
		SPLIT_UP,
		SPLIT_DOWN,
		SPLIT_LEFT,
		SPLIT_RIGHT
	};

private:
	int show_drag_area = false;
	int drag_area_margin_begin = 0;
	int drag_area_margin_end = 0;
	int drag_area_offset = 0;
	bool vertical = false;
	bool dragging_enabled = true;

	bool splitting = false;

	Vector<MultiSplitContainerDragger *> draggers;
	HashMap<Control *, MultiSplitContainerDragger *> split_map;

	struct DraggerComparator {
		bool operator()(const MultiSplitContainerDragger *p_a, const MultiSplitContainerDragger *p_b) const {
			return p_a->get_offset() < p_b->get_offset();
		}
	};

	struct ThemeCache {
		int separation = 0;
		int minimum_grab_thickness = 0;
		bool autohide = false;
		Ref<Texture2D> grabber_icon_h;
		Ref<Texture2D> grabber_icon_v;
		float base_scale = 1.0;
		Ref<StyleBox> split_bar_background;
	} theme_cache;

	Ref<Texture2D> _get_grabber_icon() const;
	void _compute_split_offset();
	int _get_separation() const;
	void _resort();

protected:
	void _notification(int p_what);
	void _validate_property(PropertyInfo &p_property) const;
	static void _bind_methods();

public:
	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	virtual Size2 get_minimum_size() const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	void split(Control *p_control, Control *p_from = nullptr, SplitDirection p_direction = SPLIT_RIGHT);
	void remove(Control *p_control);

	MultiSplitContainer(bool p_vertical = false);
	~MultiSplitContainer();
};
