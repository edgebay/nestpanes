#pragma once

#include "scene/main/node.h"

#include "scene/resources/texture.h"

class Control;
class PaneBase;

class FileSystem;

class PaneManager : public Node {
	GDCLASS(PaneManager, Node);

private:
	static PaneManager *singleton;

	HashMap<StringName, List<PaneBase *>> pane_map;

	PaneBase *current_pane = nullptr;
	PaneBase *prev_pane = nullptr;

	StringName pane_type = "";

	void _gui_focus_changed(Control *p_control);

	// File management.
	FileSystem *file_system;

	void _on_navigation_pane_create(PaneBase *p_pane);
	void _on_file_pane_create(PaneBase *p_pane);

	void _on_tree_item_activated(const String &p_path, bool is_dir);
	void _on_tree_item_selected(const String &p_path, bool is_dir);

protected:
	void _notification(int p_what);

public:
	static PaneManager *get_singleton() { return singleton; }

	template <typename T>
	void register_pane(const StringName &p_type, const Ref<Texture2D> &p_icon, const Callable &p_create_callback = Callable());

	PaneBase *create(const StringName &p_type);

	void set_current_pane(PaneBase *p_pane);
	void clear_current_pane();
	PaneBase *get_current_pane() const;
	PaneBase *get_prev_pane() const;

	PaneManager();
	~PaneManager();
};
