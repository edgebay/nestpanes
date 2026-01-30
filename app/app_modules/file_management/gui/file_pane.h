#pragma once

#include "app/gui/pane_base.h"

class FilePane : public PaneBase {
	GDCLASS(FilePane, PaneBase);

private:
	virtual String _get_pane_title() const override;
	virtual Ref<Texture2D> _get_pane_icon() const override;

public:
	FilePane();
};
