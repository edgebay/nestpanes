#pragma once

#include "scene/gui/tree.h"

class FileSystemTree : public Tree {
	GDCLASS(FileSystemTree, Tree);

private:
protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	FileSystemTree();
	~FileSystemTree();
};
