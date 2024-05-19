#include <QMessageBox>

class MyMessageBox : public QMessageBox
{
    Q_OBJECT

public:
    MyMessageBox(QWidget* parent = nullptr) : QMessageBox(parent) {}

signals:
    void closed();

protected:
    void closeEvent(QCloseEvent* event) override {
        emit closed();
        QMessageBox::closeEvent(event);
    }
};