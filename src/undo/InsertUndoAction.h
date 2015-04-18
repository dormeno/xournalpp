/*
 * Xournal++
 *
 * Undo action for insert (write text, draw stroke...)
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv3
 */

#pragma once

#include "UndoAction.h"

class Layer;
class Element;
class Redrawable;

class InsertUndoAction : public UndoAction
{
public:
	InsertUndoAction(PageRef page, Layer* layer, Element* element);
	virtual ~InsertUndoAction();

public:
	virtual bool undo(Control* control);
	virtual bool redo(Control* control);

	virtual string getText();

private:
	XOJ_TYPE_ATTRIB;

	Layer* layer;
	Element* element;
};

class InsertsUndoAction : public UndoAction
{
public:
	InsertsUndoAction(PageRef page, Layer* layer, ElementVector elements);
	virtual ~InsertsUndoAction();

public:
	virtual bool undo(Control* control);
	virtual bool redo(Control* control);

	virtual string getText();

private:
	XOJ_TYPE_ATTRIB;

	Layer* layer;
	ElementVector elements;

};
