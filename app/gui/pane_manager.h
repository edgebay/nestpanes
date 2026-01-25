#pragma once

#include "core/object/object.h"

#include "app/gui/pane_base.h"
#include "core/templates/hash_map.h"

class PaneManager : public Object {
	GDCLASS(PaneManager, Object);

public:
	typedef PaneBase *(*CreatePaneFunc)();

private:
	static PaneManager *singleton;
	static HashMap<StringName, CreatePaneFunc> create_func;

protected:
	template <typename T>
	static PaneBase *_create_pane() {
		return memnew(T);
	}

public:
	static PaneManager *get_singleton() { return singleton; }

	template <typename T>
	static void register_pane(const StringName &p_type) {
		ERR_FAIL_COND_MSG(create_func.has(p_type), vformat("Pane already registered: '%s'.", p_type));
		create_func.insert(p_type, _create_pane<T>);
	}

	static PaneBase *create_pane(const StringName &p_type) {
		PaneBase *pane = nullptr;
		if (create_func.has(p_type)) {
			pane = create_func.get(p_type)();
		}
		return pane;
	}

	PaneManager();
	~PaneManager();
};
