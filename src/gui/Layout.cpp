#include "Layout.h"

#include "XournalView.h"

#include "control/Control.h"
#include "pageposition/PagePositionHandler.h"
#include "widgets/XournalWidget.h"
#include "gui/scroll/ScrollHandling.h"
#include "gui/LayoutMapper.h"


/**
 * Padding outside the pages, including shadow
 */
const int XOURNAL_PADDING = 10;

/**
 * Padding outside the pages, if additional padding is set
 */
const int XOURNAL_PADDING_FREE_SPACE = 150;

/**
 * Allowance for shadow between page pairs in paired page mode
 */
const int XOURNAL_ROOM_FOR_SHADOW = 3;

/**
 * Padding between the pages
 */
const int XOURNAL_PADDING_BETWEEN = 15;


Layout::Layout(XournalView* view, ScrollHandling* scrollHandling)
 : view(view),
   scrollHandling(scrollHandling)
{
	XOJ_INIT_TYPE(Layout);

	g_signal_connect(scrollHandling->getHorizontal(), "value-changed", G_CALLBACK(
		+[](GtkAdjustment* adjustment, Layout* layout)
		{
			XOJ_CHECK_TYPE_OBJ(layout, Layout);
			layout->checkScroll(adjustment, layout->lastScrollHorizontal);
			layout->updateCurrentPage();
			layout->scrollHandling->scrollChanged();
		}), this);

	g_signal_connect(scrollHandling->getVertical(), "value-changed", G_CALLBACK(
		+[](GtkAdjustment* adjustment, Layout* layout)
		{
			XOJ_CHECK_TYPE_OBJ(layout, Layout);
			layout->checkScroll(adjustment, layout->lastScrollVertical);
			layout->updateCurrentPage();
			layout->scrollHandling->scrollChanged();
		}), this);

	lastScrollHorizontal = gtk_adjustment_get_value(scrollHandling->getHorizontal());
	lastScrollVertical = gtk_adjustment_get_value(scrollHandling->getVertical());
}

Layout::~Layout()
{
	XOJ_RELEASE_TYPE(Layout);
}

void Layout::checkScroll(GtkAdjustment* adjustment, double& lastScroll)
{
	XOJ_CHECK_TYPE(Layout);

	lastScroll = gtk_adjustment_get_value(adjustment);
}

/**
 * Check which page should be selected
 */
void Layout::updateCurrentPage()
{
	XOJ_CHECK_TYPE(Layout);

	Rectangle visRect = getVisibleRect();

	Control* control = this->view->getControl();

	bool pairedPages = control->getSettings()->isShowPairedPages();

	if (visRect.y < 1)
	{
		if (pairedPages && this->view->viewPagesLen > 1 &&
		    this->view->viewPages[1]->isSelected())
		{
			// page 2 already selected
		}
		else
		{
			control->firePageSelected(0);
		}
		return;
	}

	size_t mostPageNr = 0;
	double mostPagePercent = 0;

	for (size_t page = 0; page < this->view->viewPagesLen; page++)
	{
		XojPageView* p = this->view->viewPages[page];

		Rectangle currentRect = p->getRect();

		// if we are already under the visible rectangle
		// then everything below will not be visible...
		if(currentRect.y > visRect.y + visRect.height)
		{
			p->setIsVisible(false);
			for (; page < this->view->viewPagesLen; page++)
			{
				p = this->view->viewPages[page];
				p->setIsVisible(false);
			}

			break;
		}

		// if the condition is satisfied we know that
		// the rectangles intersect vertically
		if (currentRect.y + currentRect.height >= visRect.y)
		{

			double percent =
				currentRect.intersect(visRect).area() / currentRect.area();

			if (percent > mostPagePercent)
			{
				mostPagePercent = percent;
				mostPageNr = page;
			}

			p->setIsVisible(true);
		}
		else
		{
			p->setIsVisible(false);
		}
	}

	if (pairedPages && mostPageNr < this->view->viewPagesLen - 1)
	{
		int y1 = this->view->viewPages[mostPageNr]->getY();
		int y2 = this->view->viewPages[mostPageNr + 1]->getY();

		if (y1 != y2 || !this->view->viewPages[mostPageNr + 1]->isSelected())
		{
			// if the second page is selected DON'T select the first page.
			// Only select the first page if none is selected
			control->firePageSelected(mostPageNr);
		}
	}
	else
	{
		control->firePageSelected(mostPageNr);
	}
}

Rectangle Layout::getVisibleRect()
{
	XOJ_CHECK_TYPE(Layout);

	return Rectangle(gtk_adjustment_get_value(scrollHandling->getHorizontal()),
	                 gtk_adjustment_get_value(scrollHandling->getVertical()),
	                 gtk_adjustment_get_page_size(scrollHandling->getHorizontal()),
	                 gtk_adjustment_get_page_size(scrollHandling->getVertical()));
}

/**
 * Returns the height of the entire Layout
 */
double Layout::getLayoutHeight()
{
	XOJ_CHECK_TYPE(Layout);

	return layoutHeight;
}

/**
 * Returns the width of the entire Layout
 */
double Layout::getLayoutWidth()
{
	XOJ_CHECK_TYPE(Layout);

	return layoutWidth;
}

void Layout::layoutPages()
{
	XOJ_CHECK_TYPE(Layout);

	int len = this->view->viewPagesLen;

	Settings* settings = this->view->getControl()->getSettings();

	// obtain rows, cols, paired and layout from view settings
	LayoutMapper mapper(len, settings);

	// get from mapper (some may have changed to accomodate paired setting etc.)
	bool isPairedPages = mapper.getPairedPages();
	int rows = mapper.getRows();
	int columns = mapper.getColumns();

	int sizeCol[columns];
	// Needs dynamic initialisation, else clang will not compile...
	memset(sizeCol, 0, columns * sizeof(int));

	int sizeRow[rows];
	memset(sizeRow, 0, rows * sizeof(int));

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < columns; c++)
		{
			int k = mapper.map(c, r);
			if (k >= 0)
			{

				XojPageView* v = this->view->viewPages[k];

				if (sizeCol[c] < v->getDisplayWidth())
				{
					sizeCol[c] = v->getDisplayWidth();
				}
				if (sizeRow[r] < v->getDisplayHeight())
				{
					sizeRow[r] = v->getDisplayHeight();
				}
			}

		}
	}

	
	
	
	//add space around the entire page area to accomodate older Wacom tablets with limited sense area.
	int borderPrefX =  XOURNAL_PADDING;
	if (settings->getAddHorizontalSpace() )
	{
		borderPrefX += XOURNAL_PADDING_FREE_SPACE;	// this adds extra space to the left and right 
	}
	
	int borderPrefY = XOURNAL_PADDING;
	if (settings->getAddVerticalSpace() )
	{
		borderPrefY += XOURNAL_PADDING_FREE_SPACE;	// this adds space to the top and bottom
	}	

	
	
	//Calculate border offset which will center pages in viewing area
	int visibleWidth = gtk_adjustment_get_page_size(scrollHandling->getHorizontal());
	int minRequiredWidth = XOURNAL_PADDING_BETWEEN * (columns-1);
	for( int c = 0 ; c< columns; c++ )
	{
		minRequiredWidth += sizeCol[c];
	}
	int centeringXBorder = ( visibleWidth - minRequiredWidth )/2;	// this will center if all pages fit on screen.

	
	int visibleHeight = gtk_adjustment_get_page_size(scrollHandling->getVertical());
	int minRequiredHeight = XOURNAL_PADDING_BETWEEN * (rows-1);
	for( int r = 0 ; r< rows; r++ )
	{
		minRequiredHeight += sizeRow[r];
	}
	int centeringYBorder = ( visibleHeight - minRequiredHeight )/2;	// this will center if all pages fit on screen vertically.

	
		
	int borderX = MAX ( borderPrefX , centeringXBorder);
	int borderY = MAX ( borderPrefY , centeringYBorder);
	
	
	// initialize here and x again in loop below.
	int x = borderX;
	int y = borderY;


	// Iterate over ALL possible rows and columns. 
	//We don't know which page, if any,  is to be displayed in each row, column -  ask the mapper object!
	//Then assign that page coordinates with center, left or right justify within row,column grid cell as required.
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < columns; c++)
		{
			int pageAtRowCol = mapper.map(c, r);

			if (pageAtRowCol >= 0)
			{

				XojPageView* v = this->view->viewPages[pageAtRowCol];
				int vDisplayWidth = v->getDisplayWidth();
				{
					int paddingLeft;
					int paddingRight;
					int columnPadding = 0;
					columnPadding = (sizeCol[c] - vDisplayWidth);

					if (isPairedPages && len > 1)
					{
						// pair pages mode
						if (c % 2 == 0)
						{
							//align right
							paddingLeft = XOURNAL_PADDING_BETWEEN - XOURNAL_ROOM_FOR_SHADOW + columnPadding;
							paddingRight = XOURNAL_ROOM_FOR_SHADOW;
						}
						else
						{								//align left
							paddingLeft = XOURNAL_ROOM_FOR_SHADOW;
							paddingRight = XOURNAL_PADDING_BETWEEN - XOURNAL_ROOM_FOR_SHADOW + columnPadding;
						}
					}
					else
					{	// not paired page mode - center
						paddingLeft = XOURNAL_PADDING_BETWEEN / 2 + columnPadding / 2;      //center justify
						paddingRight = XOURNAL_PADDING_BETWEEN - paddingLeft + columnPadding / 2;
					}

					x += paddingLeft;

					v->layout.setX(x);
					v->layout.setY(y);

					x += vDisplayWidth + paddingRight;

				}
			}
			else
			{
				x += sizeCol[c] + XOURNAL_PADDING_BETWEEN;
			}
		}
		x = borderX;
		y += sizeRow[r] + XOURNAL_PADDING_BETWEEN;

	}

	int totalWidth = borderX * 2  + XOURNAL_PADDING_BETWEEN * (columns-1);
	for (int c = 0; c < columns; c++)
	{
		totalWidth += sizeCol[c];	// this includes paddingLeft and paddingRight
	}

	int totalHeight = borderY * 2 + XOURNAL_PADDING_BETWEEN * (rows-1);
	for (int r = 0; r < rows; r++)
	{
		totalHeight += sizeRow[r];
	}


	this->setLayoutSize(totalWidth, totalHeight);
	this->view->pagePosition->update(this->view->viewPages, len, totalHeight);
}


void Layout::setLayoutSize(int width, int height)
{
	XOJ_CHECK_TYPE(Layout);

	this->layoutHeight = height;
	this->layoutWidth = width;

	this->scrollHandling->setLayoutSize(width, height);
}

void Layout::setSize(int widgetWidth, int widgetHeight)
{
	XOJ_CHECK_TYPE(Layout);

	if (this->lastWidgetWidth != widgetWidth)
	{
		this->layoutPages();
		this->lastWidgetWidth = widgetWidth;
	}
	else
	{
		this->setLayoutSize(this->layoutWidth, this->layoutHeight);
	}

	this->view->getControl()->calcZoomFitSize();
}

void Layout::scrollRelativ(int x, int y)
{
	XOJ_CHECK_TYPE(Layout);

	gtk_adjustment_set_value(scrollHandling->getHorizontal(), gtk_adjustment_get_value(scrollHandling->getHorizontal()) + x);
	gtk_adjustment_set_value(scrollHandling->getVertical(), gtk_adjustment_get_value(scrollHandling->getVertical()) + y);
}

void Layout::scrollAbs(int x, int y)
{
	XOJ_CHECK_TYPE(Layout);

	gtk_adjustment_set_value(scrollHandling->getHorizontal(), x);
	gtk_adjustment_set_value(scrollHandling->getVertical(), y);
}


void Layout::ensureRectIsVisible(int x, int y, int width, int height)
{
	XOJ_CHECK_TYPE(Layout);

	gtk_adjustment_clamp_page(scrollHandling->getHorizontal(), x - 5, x + width + 10);
	gtk_adjustment_clamp_page(scrollHandling->getVertical(), y - 5, y + height + 10);
}



