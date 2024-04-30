#include "qt_spygraphics.h"

CViewSpyTree::CViewSpyTree(QWidget* parent /*= nullptr*/)
{

}

void CViewSpyTree::setTargetView(QGraphicsView* target)
{
	m_viewTarget = target;
}

bool CViewSpyTree::eventFilter()
{

}
