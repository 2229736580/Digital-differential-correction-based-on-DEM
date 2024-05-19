#include "testCRCS.h"

//包含gdal头文件
#include <gdal_priv.h>

#include <QMessageBox>
#include <QGraphicsLineItem>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDockwidget>
#include <QStandardItemModel>
#include "MyMessageBox.h"

#include <QDebug>
#include <QScrollBar>
#include "DC_Dlg.h"
#include "IM_Dlg.h"
#include "UC_Dlg.h"


CrosshairItem::CrosshairItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
}

QRectF CrosshairItem::boundingRect() const  // 
{
    return QRectF(-10, -10, 20, 20);
}

void CrosshairItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPen pen(Qt::red, 1);
    pen.setCosmetic(true);  // 设置为 cosmetic pen，其宽度不会随视图变换而改变
    painter->setPen(pen);

    // 获取 QGraphicsView 的大小
    int width = scene()->views().first()->width();
    int height = scene()->views().first()->height();

    // 画十字丝
    painter->drawLine(QPointF(-width / 2, 0), QPointF(width / 2, 0));
    painter->drawLine(QPointF(0, -height / 2), QPointF(0, height / 2));
}

testCRCS::testCRCS(QWidget *parent)
    : QMainWindow(parent),
      fileTreeView(new QTreeView(this)),
      fileSystemModel(new QFileSystemModel(this)),
      imageView(new MyGraphicsView(this)),
      mousePositionLabel(new QLabel(this))
{
    //ui界面初始化
    ui.setupUi(this);

    // 初始化geoTransform
    for (int i = 0; i < 6; i++) {
		geoTransform[i] = 0;
	}

    setupMainInterface(); // 设置主界面
    CenterView(); // 设置中心视图
    treeFile(); // 设置文件树

    // 设置窗口大小
    resize(1200, 700);

    // 设置窗口标题
    setWindowTitle(QStringLiteral("CRCS"));

    // 设置窗口图标
    setWindowIcon(QIcon(":/testCRCS/icon/CRCS1.png"));
}

testCRCS::~testCRCS()
{}

void testCRCS::DC_DLG() {

    DC_Dlg *DCdlg = new DC_Dlg;

    // 连接信号和槽
    connect(DCdlg, &DC_Dlg::progressChanged, this, &testCRCS::setProgressBarValue);

    // 当对话框关闭时，将进度条的值设置为0
    connect(DCdlg, &QDialog::finished, this, [this](int result) {
        Q_UNUSED(result);
        setProgressBarValue(0);
        });

    //DCdlg->show();
    DCdlg->exec();
}

void testCRCS::IM_DLG() {

	IM_Dlg *IMdlg = new IM_Dlg;

	IMdlg->exec();
}

void testCRCS::UC_DLG() {
    
	UC_Dlg *UCdlg = new UC_Dlg;

	UCdlg->exec();
}

void testCRCS::setupMainInterface() {
    // 在这里添加设置主界面的代码

    // 设置窗口标题
    setWindowTitle("testCRCS");


    // 菜单栏
    // 文件菜单
    QMenu* fileMenu = ui.menuBar->addMenu(QStringLiteral("文件(F)"));

    QIcon openIcon(":/testCRCS/icon/open.png");
    QAction* openAction = fileMenu->addAction(openIcon,QStringLiteral("打开(O)"));
    connect(openAction, &QAction::triggered, this, &testCRCS::openFile);
    //fileMenu->addAction(QStringLiteral("保存(S)"));

    // 功能菜单
    QMenu* functionMenu = ui.menuBar->addMenu(QStringLiteral("功能(D)"));

    // 微分纠正

    QIcon DCIcon(":/testCRCS/icon/dc1.png");
    QAction* DCAction = functionMenu->addAction(DCIcon,QStringLiteral("微分纠正"));
    connect(DCAction, &QAction::triggered, this, &testCRCS::DC_DLG);

    // 影像拼接
    QIcon imageMergeIcon(":/testCRCS/icon/im.png");
    QAction* IMAction = functionMenu->addAction(imageMergeIcon,QStringLiteral("影像拼接"));
    connect(IMAction, &QAction::triggered, this, &testCRCS::IM_DLG);

    // 影像匀色
    QIcon UCIcon(":/testCRCS/icon/uc1.png");
    QAction* UCAction = functionMenu->addAction(UCIcon,QStringLiteral("影像匀色"));
    connect(UCAction, &QAction::triggered, this, &testCRCS::UC_DLG);

    // 帮助菜单
    QMenu* helpMenu = ui.menuBar->addMenu(QStringLiteral("帮助(H)"));

    QIcon helpIcon(":/testCRCS/icon/about.png");
    QAction* About = helpMenu->addAction(QStringLiteral("关于(A)"));
    // 连接关于的槽函数
    connect(About, &QAction::triggered, this, &testCRCS::About);

    QAction* Help = helpMenu->addAction(helpIcon,QStringLiteral("帮助"));
    // 连接帮助的槽函数
    connect(Help, &QAction::triggered, this, &testCRCS::Help);


    // 工具栏
    // 设置工具栏不可移动
    ui.mainToolBar->setMovable(false);

    ui.mainToolBar->setIconSize(QSize(21, 21));


    // 打开文件
    openAction = ui.mainToolBar->addAction(QStringLiteral("打开"));
     
    openAction->setIcon(openIcon);
    connect(openAction, &QAction::triggered, this, &testCRCS::openFile);



    // 查看信息
    QAction* infoAction = ui.mainToolBar->addAction(QStringLiteral("查看"));
    QIcon icon(":/testCRCS/icon/in.png");
    infoAction->setIcon(icon);

    connect(infoAction, &QAction::triggered, this, &testCRCS::onInfoActionTriggered);

    // 放大
    QAction* zoomInAction = ui.mainToolBar->addAction(QStringLiteral("放大"));
    QIcon zoomInIcon(":/testCRCS/icon/zoom_in.png");  
    zoomInAction->setIcon(zoomInIcon);

    connect(zoomInAction, &QAction::triggered, this, &testCRCS::onZoomInActionTriggered);

    // 缩小
    QAction* zoomOutAction = ui.mainToolBar->addAction(QStringLiteral("缩小"));
    QIcon zoomOutIcon(":/testCRCS/icon/zoom_out.png");  
    zoomOutAction->setIcon(zoomOutIcon);

    connect(zoomOutAction, &QAction::triggered, this, &testCRCS::onZoomOutActionTriggered);

    // 平移
    //QAction* panAction = ui.mainToolBar->addAction(QStringLiteral("平移"));
    //QIcon panIcon(":/testCRCS/icon/pan.png");
    //panAction->setIcon(panIcon);
    //panAction->setCheckable(true);  // 设置为可选中的，这样当用户点击平移按钮后，按钮会保持按下状态，直到用户再次点击

    //connect(panAction, &QAction::triggered, this, &testCRCS::onPanActionTriggered);


    // 状态栏
    // 设置状态栏
	ui.statusBar->addWidget(mousePositionLabel);

    //设置标签的大小
    mousePositionLabel->setMinimumWidth(296);
    mousePositionLabel->setMinimumHeight(30);

    // 文本对齐方式为居中
    mousePositionLabel->setAlignment(Qt::AlignCenter);

    connect(imageView, &MyGraphicsView::mouseMoved, this, &testCRCS::onMouseMoved);

    // 添加进度条到状态栏
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100); // 设置进度条的范围
    progressBar->setValue(0); // 设置进度条的初始值
    ui.statusBar->addPermanentWidget(progressBar); // 将控件添加到状态栏的右侧

    //connect(dcDlg, &DC_Dlg::progressChanged, this, &testCRCS::setProgressBarValue);

}

void testCRCS::setProgressBarValue(int value)
{
    progressBar->setValue(value);

    // qDebug() << "value: " << value;

    // 如果进度条的值为100，表示进度条已经完成，将进度条隐藏
    /*if (value == 100) {
		progressBar->hide();
	}
    else {
		progressBar->show();
	}*/
}

void testCRCS::onMouseMoved(QPointF position)
{
    // 将像素坐标转换为地理坐标
    double geoX = geoTransform[0] + position.x() * geoTransform[1] + position.y() * geoTransform[2];
    double geoY = geoTransform[3] + position.x() * geoTransform[4] + position.y() * geoTransform[5];

    // 检查geoTransform是否都为0
    bool isZero = true;
    for (int i = 0; i < 6; i++) {
        if (geoTransform[i] != 0) {
            isZero = false;
            break;
        }
    }

    if (isZero) {
        // 如果geoTransform都为0，只显示像素坐标
        mousePositionLabel->setText(QString::number(position.x(), 'f', 6) + ", " + QString::number(position.y(), 'f', 6));
    }
    else {
        // 否则，显示地理坐标
        mousePositionLabel->setText(QString::number(geoX, 'f', 6) + ", " + QString::number(geoY, 'f', 6));
    }
}

void testCRCS::treeFile()
{
    // 创建一个新的 QDockWidget 并将 fileTreeView 设置为其内容
    QDockWidget* treeWidget = new QDockWidget(this);
    treeWidget->setWidget(fileTreeView);
    // 设置treeWidget不可移动
    treeWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    // 去掉标题栏
    treeWidget->setTitleBarWidget(new QWidget());
    // 设置最小宽度
    treeWidget->setMinimumWidth(300);
    // 将 dockWidget 添加到主窗口的左侧
    addDockWidget(Qt::LeftDockWidgetArea, treeWidget);
    // 禁止双击时进入编辑模式
    fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置右键菜单策略
    fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTreeView, &QTreeView::customContextMenuRequested, this, &testCRCS::showContextMenu);
}

void testCRCS::showContextMenu(const QPoint& pos)
{
    QModelIndex index = fileTreeView->indexAt(pos);
    if (index.isValid()) {  // 检查
        QMenu contextMenu;
        QAction* deleteAction = contextMenu.addAction(QStringLiteral("删除"));
        connect(deleteAction, &QAction::triggered, this, &testCRCS::onDeleteActionTriggered);
        contextMenu.exec(fileTreeView->mapToGlobal(pos));
    }
}

void testCRCS::onDeleteActionTriggered()
{
    QModelIndex index = fileTreeView->currentIndex();
    if (index.isValid()) {
        // 获取选中的项的文本
        QString fileName = index.data().toString();

        // 查找对应的文件路径
        if (openedFiles.contains(fileName)) {
            QString filePath = openedFiles.value(fileName);

            // 如果当前显示的是这个图像，清除图像视图
            if (filePath == currentImagePath) {
                imageView->scene()->clear();
                currentImagePath.clear();
            }

            // 从 openedFiles 中移除这个文件
            openedFiles.remove(fileName);
        }

        // 从 fileTreeView 中移除这个节点
        fileTreeView->model()->removeRow(index.row(), index.parent());
    }
}

void testCRCS::CenterView()
{
    // 将imageView添加到主窗口的中央
    QGraphicsScene* scene = new QGraphicsScene(this);
    imageView->setScene(scene);
    setCentralWidget(imageView);

    //connect(imageView, &MyGraphicsView::wheelScrolled, this, &testCRCS::onWheelScrolled);
}


void testCRCS::openFile()
{
    // 打开文件对话框
    QStringList filePaths = QFileDialog::getOpenFileNames(this, QStringLiteral("打开文件"), "", QStringLiteral("Images (*.png *.tif *.jpg)"));
    if (filePaths.isEmpty()) {
        return;
    }

    populateTreeView(filePaths);
}


void testCRCS::fileView(const QModelIndex& index)
{
    // 获取双击的项的文本
    QString fileName = index.data().toString();

    // 查找对应的文件路径
    if (openedFiles.contains(fileName)) {
        QString filePath = openedFiles.value(fileName);

        // 将文件路径保存到 currentImagePath 中
        currentImagePath = filePath;

        std::string strFilePath = filePath.toStdString();
        const char* pszFile = strFilePath.c_str();

        // 注册所有的驱动
        GDALAllRegister();

        // 打开影像文件
        GDALDataset* dataset = (GDALDataset*)GDALOpen(pszFile, GA_ReadOnly);
        if (dataset != nullptr) {
            // 获取地理变换参数
            double geoTransform[6];
            if (dataset->GetGeoTransform(geoTransform) == CE_None) {
                // 将地理变换参数保存到一个成员变量中
                for (int i = 0; i < 6; i++) {
                    this->geoTransform[i] = geoTransform[i];
                }
            }

            // 关闭影像文件
            GDALClose(dataset);
        }

        // 释放资源
        GDALDestroyDriverManager();

        // 显示图片
        QGraphicsScene* scene = new QGraphicsScene(this);
        QImage image(filePath);
        QPixmap pixmap = QPixmap::fromImage(image);
        scene->addPixmap(pixmap);
        scene->setSceneRect(pixmap.rect());
        imageView->setScene(scene);
        imageView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
}


void testCRCS::populateTreeView(const QStringList& filePaths)
{
    // 获取 fileTreeView 的模型
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(fileTreeView->model());

    // 如果模型不存在，创建一个新的 QStandardItemModel
    if (!model) {
        model = new QStandardItemModel(this);
        model->setHorizontalHeaderLabels(QStringList() << QStringLiteral("视图"));  // 设置根节点的名称
        fileTreeView->setModel(model);
    }

    QStandardItem* rootItem = model->invisibleRootItem();

    // 对于每个选中的文件，创建一个新的 QStandardItem 作为子节点，并设置其文本为文件的名称
    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QStandardItem* fileItem = new QStandardItem(fileInfo.fileName());
        rootItem->appendRow(fileItem);

        // 将文件的路径添加到 openedFiles 中
        openedFiles.insert(fileInfo.fileName(), filePath);
    }

    // 更新 fileTreeView
    fileTreeView->setModel(model);

    connect(fileTreeView, &QTreeView::doubleClicked, this, &testCRCS::fileView);
}

void testCRCS::onInfoActionTriggered()
{
    // 更改鼠标的图标为十字丝
    imageView->setCursor(Qt::CrossCursor);

    connect(imageView, &MyGraphicsView::mouseClicked, this, &testCRCS::onImageClicked);
}

void testCRCS::onImageClicked(QPointF position)
{
    // 将像素坐标转换为地理坐标
    double geoX = geoTransform[0] + position.x() * geoTransform[1] + position.y() * geoTransform[2];
    double geoY = geoTransform[3] + position.x() * geoTransform[4] + position.y() * geoTransform[5];

    // 获取像素坐标
    double pixelX = position.x();
    double pixelY = position.y();

    // 如果十字丝已经存在，删除它
    if (crosshairItem) {
        imageView->scene()->removeItem(crosshairItem);
        delete crosshairItem;
        crosshairItem = nullptr;
    }

    // 创建一个新的十字丝并将其添加到场景中
    crosshairItem = new CrosshairItem();
    crosshairItem->setPos(position);  // 将十字丝的位置设置为鼠标点击的位置
    imageView->scene()->addItem(crosshairItem);

    // 使用 GDAL 打开当前显示的影像

    // 注册所有的驱动
    GDALAllRegister();

    GDALDataset* dataset = (GDALDataset*)GDALOpen(currentImagePath.toStdString().c_str(), GA_ReadOnly);
    if (dataset != nullptr) {
        // 获取投影信息
        const char* projectionRef = dataset->GetProjectionRef();

        // 创建一个 OGRSpatialReference 对象
        OGRSpatialReference oSRS;
        char* pszProjection = const_cast<char*>(projectionRef);

        // 导入投影信息
        oSRS.importFromWkt(&pszProjection);

        // 获取 PROJCS，GEOGCS，DATUM，PROJECTION
        const char* projcs = oSRS.GetAttrValue("PROJCS");
        const char* geogcs = oSRS.GetAttrValue("GEOGCS");
        const char* datum = oSRS.GetAttrValue("DATUM");
        //const char* projection = oSRS.GetAttrValue("PROJECTION");

        // 获取红绿蓝三通道的值
        GDALRasterBand* redBand = dataset->GetRasterBand(1);
        GDALRasterBand* greenBand = dataset->GetRasterBand(2);
        GDALRasterBand* blueBand = dataset->GetRasterBand(3);

        int red, green, blue;
        redBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &red, 1, 1, GDT_Int32, 0, 0);
        greenBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &green, 1, 1, GDT_Int32, 0, 0);
        blueBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &blue, 1, 1, GDT_Int32, 0, 0);

        // 创建一个自定义的消息框
        MyMessageBox* messageBox = new MyMessageBox(this);
        messageBox->setIcon(QMessageBox::Information);
        messageBox->setWindowTitle(QStringLiteral("信息"));
        messageBox->setText(QStringLiteral("PROJCS: %1\nGEOGCS: %2\nDATUM: %3\n地理坐标: (%4, %5)\n像素坐标: (%6, %7)\nRGB: (%8, %9, %10)")
            .arg(projcs)
            .arg(geogcs)
            .arg(datum)
            .arg(QString::number(geoX, 'f', 6))  // 地理坐标保留六位小数
            .arg(QString::number(geoY, 'f', 6))  // 地理坐标保留六位小数
            .arg(QString::number(pixelX, 'f', 3))  // 像素坐标保留三位小数
            .arg(QString::number(pixelY, 'f', 3))  // 像素坐标保留三位小数
            .arg(red)
            .arg(green)
            .arg(blue));

        // 当消息框关闭时，断开连接
        connect(messageBox, &MyMessageBox::closed, this, [this]() {
            disconnect(imageView, &MyGraphicsView::mouseClicked, this, &testCRCS::onImageClicked);
            
            // 如果十字丝存在，删除它
            if (crosshairItem) {
                imageView->scene()->removeItem(crosshairItem);
                delete crosshairItem;
                crosshairItem = nullptr;
            }

            // 更改鼠标的图标为箭头
            imageView->setCursor(Qt::ArrowCursor);
        });

        messageBox->show();

        // 关闭影像文件
        GDALClose(dataset);
    }

    // 释放资源
    GDALDestroyDriverManager();
}


void testCRCS::onZoomInActionTriggered()
{
    // 设置放大因子
    double scaleFactor = 1.1;  // 放大10%
    imageView->scale(scaleFactor, scaleFactor);
}

void testCRCS::onZoomOutActionTriggered()
{
    // 设置缩小因子
    double scaleFactor = 0.9;  // 缩小10%
    imageView->scale(scaleFactor, scaleFactor);
}

void testCRCS::onPanActionTriggered(bool checked)
{
    imageView->setPanning(checked);
}

void testCRCS::Help()
{
    // 创建一个新的 QDialog
    QDialog* dialog = new QDialog(this);
    dialog->setFixedSize(400, 300);

    // 创建一个 QVBoxLayout
    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // 去除问号按钮
    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 创建标签并添加到布局中
    QLabel* label1 = new QLabel(dialog);
    label1->setWordWrap(true);
    label1->setAlignment(Qt::AlignJustify);
    label1->setText(QStringLiteral("<b>功能一：</b> 基于DEM和EO文件，利用正解法先求出单张影像纠正后的范围,"
        "再从纠正后影像出发，逐像素利用反解法从原始影像内插出该处像元的值，同时赋予纠正后影像坐标值与投影信息。"));

    QLabel* label2 = new QLabel(dialog);
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignJustify);
    label2->setText(QStringLiteral("<b>功能二：</b> 基于具有坐标的分幅微分纠正影像，先根据输入影像计算出拼接后范围，再利用每张"
        "影像的投影中心生成对应的维诺图并生成对应的掩膜文件，对每张影像做掩膜之后，再将每幅掩膜结果导入最终结果中，得到最终影像。"));

    QLabel* label3 = new QLabel(dialog);
    label3->setWordWrap(true);
    label3->setAlignment(Qt::AlignJustify);
    label3->setText(QStringLiteral("<b>功能三：</b> 基于具有坐标的分幅微分纠正影像，先找出两张影像的重叠区域，再从重叠区域中隔间距取点，"
        "通过取出的同名点对的像素值，拟合相应系数，并用该系数对待校正影像做改正得到校正结果。默认导入的第一张影像为参考影像，并用前一张结果的校正结果，"
        "对后一张影像进行校正,以此一次处理多张影像。"));

    layout->addWidget(label1);
    layout->addWidget(label2);
    layout->addWidget(label3);

    // 设置对话框的标题
    dialog->setWindowTitle(QStringLiteral("帮助"));

    // 显示对话框
    dialog->exec();
}

void testCRCS::About()
{
	// 创建一个新的 QDialog
	QDialog* dialog = new QDialog(this);
	dialog->setFixedSize(400, 200);

	// 创建一个 QVBoxLayout
	QVBoxLayout* layout = new QVBoxLayout(dialog);

	// 去除问号按钮
	dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 创建标签并添加到布局中
    QLabel* label1 = new QLabel(dialog);
    label1->setMaximumSize(500, 70); 
    label1->setWordWrap(true);
    label1->setAlignment(Qt::AlignJustify);
    label1->setAlignment(Qt::AlignCenter); // 设置内容上下居中对齐
    //label1->setContentsMargins(0, 0, 0, 0); // 设置内容边距
    label1->setText(QStringLiteral("<b>CRCS</b> 是一个基于GDAL的遥感影像处理软件，"
        "提供了微分纠正、影像拼接、影像匀色等功能。"));

    QLabel* label2 = new QLabel(dialog);
    label2->setMaximumSize(500, 70); 
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignJustify);
    label2->setAlignment(Qt::AlignCenter);
    //label2->setContentsMargins(0, 0, 0, 0); // 设置内容边距
    label2->setText(QStringLiteral("版本：3.5.2"));

    QLabel* label3 = new QLabel(dialog);
    label3->setMaximumSize(500, 70); 
    label3->setWordWrap(true);
    label3->setAlignment(Qt::AlignJustify);
    label3->setAlignment(Qt::AlignCenter);
    //label3->setContentsMargins(0, 0, 0, 0); // 设置内容边距
    label3->setText(QStringLiteral("作者：长安大学2021级数字摄影测量第三小组"));

    layout->addWidget(label1);
    layout->addWidget(label2);
    layout->addWidget(label3);
    //layout->setSpacing(10);


	// 设置对话框的标题
	dialog->setWindowTitle(QStringLiteral("关于"));

	// 显示对话框
	dialog->exec();
}