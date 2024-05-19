// MyGraphicsView.cpp
#include "MyGraphicsView.h"

#include <QWheelEvent>
#include <QScrollBar>

MyGraphicsView::MyGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
	setMouseTracking(true);
}

void MyGraphicsView::wheelEvent(QWheelEvent* event)
{
    QPoint scrollAmount = event->angleDelta();
    scrollAmount.y() > 0 ? zoomIn() : zoomOut();
}

void MyGraphicsView::zoomIn()
{
    zoom(1 + 0.1);
}

void MyGraphicsView::zoomOut()
{
    zoom(1 - 0.1);
}

void MyGraphicsView::zoom(float scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.02 || factor > 100)
        return;

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scale(scaleFactor, scaleFactor);
    scale_ *= scaleFactor;
}

void MyGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::MiddleButton) {
        // ��������ƶ��ľ���
        QPointF diff = mapToScene(m_lastMousePos) - mapToScene(event->pos());
        m_lastMousePos = event->pos();

        // �ı���ͼ�����ĵ�
        centerOn(mapToScene(viewport()->rect().center()) + diff);
    }
    else {
        emit mouseMoved(mapToScene(event->pos()));
        QGraphicsView::mouseMoveEvent(event);
    }
}

void MyGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    else {
        if (event->button() == Qt::LeftButton) {
            QPointF position = mapToScene(event->pos());
            emit mouseClicked(position);
        }
        QGraphicsView::mousePressEvent(event);
    }
}

void MyGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        setCursor(Qt::ArrowCursor);
    }
    else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void MyGraphicsView::setPanning(bool panning)
{
    m_panning = panning;
    if (panning) {
        setCursor(Qt::OpenHandCursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }
}