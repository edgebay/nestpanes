#pragma once

#include "scene/gui/dialogs.h"

class Label;
class RichTextLabel;
class TextureRect;
class Tree;

class AppAbout : public AcceptDialog {
	GDCLASS(AppAbout, AcceptDialog);

private:
	enum SectionFlags {
		FLAG_SINGLE_COLUMN = 1 << 0,
	};

	void _license_tree_selected();

	Label *_about_text_label = nullptr;
	Tree *_tpl_tree = nullptr;
	RichTextLabel *license_text_label = nullptr;
	RichTextLabel *_tpl_text = nullptr;
	TextureRect *_logo = nullptr;

protected:
	void _notification(int p_what);

public:
	AppAbout();
};
