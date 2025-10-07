#pragma once

// #include "scene/gui/container.h"
#include "scene/gui/popup.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"

class Button;

class AppTabContainer : public TabContainer {
	GDCLASS(AppTabContainer, TabContainer);

public:
	enum MenuOptions {
		// Scene menu.
		FILE_NEW_SCENE,
	};

private:
	Button *tab_add = nullptr;

	bool new_tab_enabled = false;

	int current_menu_option = 0;

	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _on_tab_closed(int p_tab);
	void _on_resized();

	void _reposition_active_tab(int p_to_index);

protected:
	void _notification(int p_what);

	// virtual void add_child_notify(Node *p_child) override;
	// virtual void move_child_notify(Node *p_child) override;

	static void _bind_methods();

public:
	void set_new_tab_enabled(bool p_enabled);
	bool get_new_tab_enabled() const;

	void trigger_menu_option(int p_option, bool p_confirmed);

	AppTabContainer();
	~AppTabContainer();
};
