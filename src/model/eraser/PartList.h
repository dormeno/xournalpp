/*
 * Xournal++
 *
 * A list with parts
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv3
 */

#pragma once

#include <gtk/gtk.h>
#include <XournalType.h>

class EraseableStrokePart;

class PartList
{
public:
	PartList();
	virtual ~PartList();

private:
	PartList(const PartList& list);
	void operator=(const PartList& list);

public:
	void add(EraseableStrokePart* part);
	PartList* clone();

private:
	XOJ_TYPE_ATTRIB;

	GList* data;

	friend class EraseableStroke;
};
