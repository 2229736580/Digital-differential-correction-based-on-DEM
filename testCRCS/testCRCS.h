#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_testCRCS.h"
#include "MyGraphicsView.h"
#include "DC_Dlg.h"


#include<QTreeView>
#include<QFileSystemModel>
#include<QLabel>
#include<QGraphicsLineItem>
#include<QProgressBar>



class CrosshairItem : public QGraphicsItem
{
public:
    CrosshairItem(QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};


class testCRCS : public QMainWindow
{
    Q_OBJECT

public:
    testCRCS(QWidget *parent = nullptr);
    ~testCRCS();
 
    CrosshairItem* crosshairItem = nullptr;

    QTreeView* fileTreeView;
    QFileSystemModel* fileSystemModel;
    MyGraphicsView* imageView;

    void setProgressBarValue(int value);

public:
    //void addOpenedFile(const QString& fileName, const QString& filePath);

    void populateTreeView(const QStringList& filePaths);

private:

    Ui::testCRCSClass ui;

    QProgressBar* progressBar;

    void setupMainInterface();

    void treeFile();

    void showContextMenu(const QPoint& pos);

    void onDeleteActionTriggered();

    void CenterView();

    QLabel* mousePositionLabel;

    void openFile();

    void fileView(const QModelIndex& index);

    QMap<QString, QString> openedFiles;  // 用于存储打开的文件的路径

private:
    double geoTransform[6];  

    QString currentImagePath;  // 当前显示的影像的文件路径

    QGraphicsLineItem* verticalLine = nullptr;
    QGraphicsLineItem* horizontalLine = nullptr;

public slots:

    void onMouseMoved(QPointF position);

    void onInfoActionTriggered();

    void onImageClicked(QPointF position);

    void onZoomInActionTriggered();

    void onZoomOutActionTriggered();

    void onPanActionTriggered(bool checked);

    void Help();

    void About();

private slots:

    void DC_DLG();

    void IM_DLG();

    void UC_DLG();
};