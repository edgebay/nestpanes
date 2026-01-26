#pragma once

#include "app/gui/pane_base.h"

class WelcomePane : public PaneBase {
	GDCLASS(WelcomePane, PaneBase);

private:
	String _get_pane_title() const override;
	Ref<Texture2D> _get_pane_icon() const override;

public:
	static const StringName &get_type_static();
	// static Ref<Texture2D> get_pane_icon();

	WelcomePane();
};
