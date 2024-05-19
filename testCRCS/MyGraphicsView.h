// MyGraphicsView.h

#include <QGraphicsView>
#include <QWheelEvent>

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MyGraphicsView(QWidget* parent = nullptr);

    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void zoomIn();
    void zoomOut();
    void zoom(float scaleFactor);

    void setPanning(bool panning);
private:
    float scale_ = 1.0;  // 记录当前缩放比例
    bool m_panning = false;
    QPoint m_lastMousePos;

signals:
    void wheelScrolled(QWheelEvent* event);
    void mouseMoved(QPointF position);
    void mouseClicked(QPointF position);
};