#pragma once

#include "scene/gui/control.h"

#include "scene/resources/texture.h"

class PaneBase : public Control {
	GDCLASS(PaneBase, Control);

private:
	StringName type;

	String title;
	Ref<Texture2D> icon;

protected:
	void _notification(int p_what);

	virtual String _get_pane_title() const;
	virtual Ref<Texture2D> _get_pane_icon() const;

	static void _bind_methods();

public:
	StringName get_type() const;

	void set_title(const String &p_title);
	String get_title() const;

	void set_icon(const Ref<Texture2D> &p_icon);
	Ref<Texture2D> get_icon() const;

	PaneBase(const StringName &p_type);
	virtual ~PaneBase();
};
