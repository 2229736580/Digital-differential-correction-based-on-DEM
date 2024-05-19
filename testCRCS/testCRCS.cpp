#include "testCRCS.h"

//����gdalͷ�ļ�
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
    pen.setCosmetic(true);  // ����Ϊ cosmetic pen�����Ȳ�������ͼ�任���ı�
    painter->setPen(pen);

    // ��ȡ QGraphicsView �Ĵ�С
    int width = scene()->views().first()->width();
    int height = scene()->views().first()->height();

    // ��ʮ��˿
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
    //ui�����ʼ��
    ui.setupUi(this);

    // ��ʼ��geoTransform
    for (int i = 0; i < 6; i++) {
		geoTransform[i] = 0;
	}

    setupMainInterface(); // ����������
    CenterView(); // ����������ͼ
    treeFile(); // �����ļ���

    // ���ô��ڴ�С
    resize(1200, 700);

    // ���ô��ڱ���
    setWindowTitle(QStringLiteral("CRCS"));

    // ���ô���ͼ��
    setWindowIcon(QIcon(":/testCRCS/icon/CRCS1.png"));
}

testCRCS::~testCRCS()
{}

void testCRCS::DC_DLG() {

    DC_Dlg *DCdlg = new DC_Dlg;

    // �����źźͲ�
    connect(DCdlg, &DC_Dlg::progressChanged, this, &testCRCS::setProgressBarValue);

    // ���Ի���ر�ʱ������������ֵ����Ϊ0
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
    // �������������������Ĵ���

    // ���ô��ڱ���
    setWindowTitle("testCRCS");


    // �˵���
    // �ļ��˵�
    QMenu* fileMenu = ui.menuBar->addMenu(QStringLiteral("�ļ�(F)"));

    QIcon openIcon(":/testCRCS/icon/open.png");
    QAction* openAction = fileMenu->addAction(openIcon,QStringLiteral("��(O)"));
    connect(openAction, &QAction::triggered, this, &testCRCS::openFile);
    //fileMenu->addAction(QStringLiteral("����(S)"));

    // ���ܲ˵�
    QMenu* functionMenu = ui.menuBar->addMenu(QStringLiteral("����(D)"));

    // ΢�־���

    QIcon DCIcon(":/testCRCS/icon/dc1.png");
    QAction* DCAction = functionMenu->addAction(DCIcon,QStringLiteral("΢�־���"));
    connect(DCAction, &QAction::triggered, this, &testCRCS::DC_DLG);

    // Ӱ��ƴ��
    QIcon imageMergeIcon(":/testCRCS/icon/im.png");
    QAction* IMAction = functionMenu->addAction(imageMergeIcon,QStringLiteral("Ӱ��ƴ��"));
    connect(IMAction, &QAction::triggered, this, &testCRCS::IM_DLG);

    // Ӱ����ɫ
    QIcon UCIcon(":/testCRCS/icon/uc1.png");
    QAction* UCAction = functionMenu->addAction(UCIcon,QStringLiteral("Ӱ����ɫ"));
    connect(UCAction, &QAction::triggered, this, &testCRCS::UC_DLG);

    // �����˵�
    QMenu* helpMenu = ui.menuBar->addMenu(QStringLiteral("����(H)"));

    QIcon helpIcon(":/testCRCS/icon/about.png");
    QAction* About = helpMenu->addAction(QStringLiteral("����(A)"));
    // ���ӹ��ڵĲۺ���
    connect(About, &QAction::triggered, this, &testCRCS::About);

    QAction* Help = helpMenu->addAction(helpIcon,QStringLiteral("����"));
    // ���Ӱ����Ĳۺ���
    connect(Help, &QAction::triggered, this, &testCRCS::Help);


    // ������
    // ���ù����������ƶ�
    ui.mainToolBar->setMovable(false);

    ui.mainToolBar->setIconSize(QSize(21, 21));


    // ���ļ�
    openAction = ui.mainToolBar->addAction(QStringLiteral("��"));
     
    openAction->setIcon(openIcon);
    connect(openAction, &QAction::triggered, this, &testCRCS::openFile);



    // �鿴��Ϣ
    QAction* infoAction = ui.mainToolBar->addAction(QStringLiteral("�鿴"));
    QIcon icon(":/testCRCS/icon/in.png");
    infoAction->setIcon(icon);

    connect(infoAction, &QAction::triggered, this, &testCRCS::onInfoActionTriggered);

    // �Ŵ�
    QAction* zoomInAction = ui.mainToolBar->addAction(QStringLiteral("�Ŵ�"));
    QIcon zoomInIcon(":/testCRCS/icon/zoom_in.png");  
    zoomInAction->setIcon(zoomInIcon);

    connect(zoomInAction, &QAction::triggered, this, &testCRCS::onZoomInActionTriggered);

    // ��С
    QAction* zoomOutAction = ui.mainToolBar->addAction(QStringLiteral("��С"));
    QIcon zoomOutIcon(":/testCRCS/icon/zoom_out.png");  
    zoomOutAction->setIcon(zoomOutIcon);

    connect(zoomOutAction, &QAction::triggered, this, &testCRCS::onZoomOutActionTriggered);

    // ƽ��
    //QAction* panAction = ui.mainToolBar->addAction(QStringLiteral("ƽ��"));
    //QIcon panIcon(":/testCRCS/icon/pan.png");
    //panAction->setIcon(panIcon);
    //panAction->setCheckable(true);  // ����Ϊ��ѡ�еģ��������û����ƽ�ư�ť�󣬰�ť�ᱣ�ְ���״̬��ֱ���û��ٴε��

    //connect(panAction, &QAction::triggered, this, &testCRCS::onPanActionTriggered);


    // ״̬��
    // ����״̬��
	ui.statusBar->addWidget(mousePositionLabel);

    //���ñ�ǩ�Ĵ�С
    mousePositionLabel->setMinimumWidth(296);
    mousePositionLabel->setMinimumHeight(30);

    // �ı����뷽ʽΪ����
    mousePositionLabel->setAlignment(Qt::AlignCenter);

    connect(imageView, &MyGraphicsView::mouseMoved, this, &testCRCS::onMouseMoved);

    // ��ӽ�������״̬��
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100); // ���ý������ķ�Χ
    progressBar->setValue(0); // ���ý������ĳ�ʼֵ
    ui.statusBar->addPermanentWidget(progressBar); // ���ؼ���ӵ�״̬�����Ҳ�

    //connect(dcDlg, &DC_Dlg::progressChanged, this, &testCRCS::setProgressBarValue);

}

void testCRCS::setProgressBarValue(int value)
{
    progressBar->setValue(value);

    // qDebug() << "value: " << value;

    // �����������ֵΪ100����ʾ�������Ѿ���ɣ�������������
    /*if (value == 100) {
		progressBar->hide();
	}
    else {
		progressBar->show();
	}*/
}

void testCRCS::onMouseMoved(QPointF position)
{
    // ����������ת��Ϊ��������
    double geoX = geoTransform[0] + position.x() * geoTransform[1] + position.y() * geoTransform[2];
    double geoY = geoTransform[3] + position.x() * geoTransform[4] + position.y() * geoTransform[5];

    // ���geoTransform�Ƿ�Ϊ0
    bool isZero = true;
    for (int i = 0; i < 6; i++) {
        if (geoTransform[i] != 0) {
            isZero = false;
            break;
        }
    }

    if (isZero) {
        // ���geoTransform��Ϊ0��ֻ��ʾ��������
        mousePositionLabel->setText(QString::number(position.x(), 'f', 6) + ", " + QString::number(position.y(), 'f', 6));
    }
    else {
        // ������ʾ��������
        mousePositionLabel->setText(QString::number(geoX, 'f', 6) + ", " + QString::number(geoY, 'f', 6));
    }
}

void testCRCS::treeFile()
{
    // ����һ���µ� QDockWidget ���� fileTreeView ����Ϊ������
    QDockWidget* treeWidget = new QDockWidget(this);
    treeWidget->setWidget(fileTreeView);
    // ����treeWidget�����ƶ�
    treeWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    // ȥ��������
    treeWidget->setTitleBarWidget(new QWidget());
    // ������С���
    treeWidget->setMinimumWidth(300);
    // �� dockWidget ��ӵ������ڵ����
    addDockWidget(Qt::LeftDockWidgetArea, treeWidget);
    // ��ֹ˫��ʱ����༭ģʽ
    fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // �����Ҽ��˵�����
    fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTreeView, &QTreeView::customContextMenuRequested, this, &testCRCS::showContextMenu);
}

void testCRCS::showContextMenu(const QPoint& pos)
{
    QModelIndex index = fileTreeView->indexAt(pos);
    if (index.isValid()) {  // ���
        QMenu contextMenu;
        QAction* deleteAction = contextMenu.addAction(QStringLiteral("ɾ��"));
        connect(deleteAction, &QAction::triggered, this, &testCRCS::onDeleteActionTriggered);
        contextMenu.exec(fileTreeView->mapToGlobal(pos));
    }
}

void testCRCS::onDeleteActionTriggered()
{
    QModelIndex index = fileTreeView->currentIndex();
    if (index.isValid()) {
        // ��ȡѡ�е�����ı�
        QString fileName = index.data().toString();

        // ���Ҷ�Ӧ���ļ�·��
        if (openedFiles.contains(fileName)) {
            QString filePath = openedFiles.value(fileName);

            // �����ǰ��ʾ�������ͼ�����ͼ����ͼ
            if (filePath == currentImagePath) {
                imageView->scene()->clear();
                currentImagePath.clear();
            }

            // �� openedFiles ���Ƴ�����ļ�
            openedFiles.remove(fileName);
        }

        // �� fileTreeView ���Ƴ�����ڵ�
        fileTreeView->model()->removeRow(index.row(), index.parent());
    }
}

void testCRCS::CenterView()
{
    // ��imageView��ӵ������ڵ�����
    QGraphicsScene* scene = new QGraphicsScene(this);
    imageView->setScene(scene);
    setCentralWidget(imageView);

    //connect(imageView, &MyGraphicsView::wheelScrolled, this, &testCRCS::onWheelScrolled);
}


void testCRCS::openFile()
{
    // ���ļ��Ի���
    QStringList filePaths = QFileDialog::getOpenFileNames(this, QStringLiteral("���ļ�"), "", QStringLiteral("Images (*.png *.tif *.jpg)"));
    if (filePaths.isEmpty()) {
        return;
    }

    populateTreeView(filePaths);
}


void testCRCS::fileView(const QModelIndex& index)
{
    // ��ȡ˫��������ı�
    QString fileName = index.data().toString();

    // ���Ҷ�Ӧ���ļ�·��
    if (openedFiles.contains(fileName)) {
        QString filePath = openedFiles.value(fileName);

        // ���ļ�·�����浽 currentImagePath ��
        currentImagePath = filePath;

        std::string strFilePath = filePath.toStdString();
        const char* pszFile = strFilePath.c_str();

        // ע�����е�����
        GDALAllRegister();

        // ��Ӱ���ļ�
        GDALDataset* dataset = (GDALDataset*)GDALOpen(pszFile, GA_ReadOnly);
        if (dataset != nullptr) {
            // ��ȡ����任����
            double geoTransform[6];
            if (dataset->GetGeoTransform(geoTransform) == CE_None) {
                // ������任�������浽һ����Ա������
                for (int i = 0; i < 6; i++) {
                    this->geoTransform[i] = geoTransform[i];
                }
            }

            // �ر�Ӱ���ļ�
            GDALClose(dataset);
        }

        // �ͷ���Դ
        GDALDestroyDriverManager();

        // ��ʾͼƬ
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
    // ��ȡ fileTreeView ��ģ��
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(fileTreeView->model());

    // ���ģ�Ͳ����ڣ�����һ���µ� QStandardItemModel
    if (!model) {
        model = new QStandardItemModel(this);
        model->setHorizontalHeaderLabels(QStringList() << QStringLiteral("��ͼ"));  // ���ø��ڵ������
        fileTreeView->setModel(model);
    }

    QStandardItem* rootItem = model->invisibleRootItem();

    // ����ÿ��ѡ�е��ļ�������һ���µ� QStandardItem ��Ϊ�ӽڵ㣬���������ı�Ϊ�ļ�������
    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QStandardItem* fileItem = new QStandardItem(fileInfo.fileName());
        rootItem->appendRow(fileItem);

        // ���ļ���·����ӵ� openedFiles ��
        openedFiles.insert(fileInfo.fileName(), filePath);
    }

    // ���� fileTreeView
    fileTreeView->setModel(model);

    connect(fileTreeView, &QTreeView::doubleClicked, this, &testCRCS::fileView);
}

void testCRCS::onInfoActionTriggered()
{
    // ��������ͼ��Ϊʮ��˿
    imageView->setCursor(Qt::CrossCursor);

    connect(imageView, &MyGraphicsView::mouseClicked, this, &testCRCS::onImageClicked);
}

void testCRCS::onImageClicked(QPointF position)
{
    // ����������ת��Ϊ��������
    double geoX = geoTransform[0] + position.x() * geoTransform[1] + position.y() * geoTransform[2];
    double geoY = geoTransform[3] + position.x() * geoTransform[4] + position.y() * geoTransform[5];

    // ��ȡ��������
    double pixelX = position.x();
    double pixelY = position.y();

    // ���ʮ��˿�Ѿ����ڣ�ɾ����
    if (crosshairItem) {
        imageView->scene()->removeItem(crosshairItem);
        delete crosshairItem;
        crosshairItem = nullptr;
    }

    // ����һ���µ�ʮ��˿��������ӵ�������
    crosshairItem = new CrosshairItem();
    crosshairItem->setPos(position);  // ��ʮ��˿��λ������Ϊ�������λ��
    imageView->scene()->addItem(crosshairItem);

    // ʹ�� GDAL �򿪵�ǰ��ʾ��Ӱ��

    // ע�����е�����
    GDALAllRegister();

    GDALDataset* dataset = (GDALDataset*)GDALOpen(currentImagePath.toStdString().c_str(), GA_ReadOnly);
    if (dataset != nullptr) {
        // ��ȡͶӰ��Ϣ
        const char* projectionRef = dataset->GetProjectionRef();

        // ����һ�� OGRSpatialReference ����
        OGRSpatialReference oSRS;
        char* pszProjection = const_cast<char*>(projectionRef);

        // ����ͶӰ��Ϣ
        oSRS.importFromWkt(&pszProjection);

        // ��ȡ PROJCS��GEOGCS��DATUM��PROJECTION
        const char* projcs = oSRS.GetAttrValue("PROJCS");
        const char* geogcs = oSRS.GetAttrValue("GEOGCS");
        const char* datum = oSRS.GetAttrValue("DATUM");
        //const char* projection = oSRS.GetAttrValue("PROJECTION");

        // ��ȡ��������ͨ����ֵ
        GDALRasterBand* redBand = dataset->GetRasterBand(1);
        GDALRasterBand* greenBand = dataset->GetRasterBand(2);
        GDALRasterBand* blueBand = dataset->GetRasterBand(3);

        int red, green, blue;
        redBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &red, 1, 1, GDT_Int32, 0, 0);
        greenBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &green, 1, 1, GDT_Int32, 0, 0);
        blueBand->RasterIO(GF_Read, int(pixelX), int(pixelY), 1, 1, &blue, 1, 1, GDT_Int32, 0, 0);

        // ����һ���Զ������Ϣ��
        MyMessageBox* messageBox = new MyMessageBox(this);
        messageBox->setIcon(QMessageBox::Information);
        messageBox->setWindowTitle(QStringLiteral("��Ϣ"));
        messageBox->setText(QStringLiteral("PROJCS: %1\nGEOGCS: %2\nDATUM: %3\n��������: (%4, %5)\n��������: (%6, %7)\nRGB: (%8, %9, %10)")
            .arg(projcs)
            .arg(geogcs)
            .arg(datum)
            .arg(QString::number(geoX, 'f', 6))  // �������걣����λС��
            .arg(QString::number(geoY, 'f', 6))  // �������걣����λС��
            .arg(QString::number(pixelX, 'f', 3))  // �������걣����λС��
            .arg(QString::number(pixelY, 'f', 3))  // �������걣����λС��
            .arg(red)
            .arg(green)
            .arg(blue));

        // ����Ϣ��ر�ʱ���Ͽ�����
        connect(messageBox, &MyMessageBox::closed, this, [this]() {
            disconnect(imageView, &MyGraphicsView::mouseClicked, this, &testCRCS::onImageClicked);
            
            // ���ʮ��˿���ڣ�ɾ����
            if (crosshairItem) {
                imageView->scene()->removeItem(crosshairItem);
                delete crosshairItem;
                crosshairItem = nullptr;
            }

            // ��������ͼ��Ϊ��ͷ
            imageView->setCursor(Qt::ArrowCursor);
        });

        messageBox->show();

        // �ر�Ӱ���ļ�
        GDALClose(dataset);
    }

    // �ͷ���Դ
    GDALDestroyDriverManager();
}


void testCRCS::onZoomInActionTriggered()
{
    // ���÷Ŵ�����
    double scaleFactor = 1.1;  // �Ŵ�10%
    imageView->scale(scaleFactor, scaleFactor);
}

void testCRCS::onZoomOutActionTriggered()
{
    // ������С����
    double scaleFactor = 0.9;  // ��С10%
    imageView->scale(scaleFactor, scaleFactor);
}

void testCRCS::onPanActionTriggered(bool checked)
{
    imageView->setPanning(checked);
}

void testCRCS::Help()
{
    // ����һ���µ� QDialog
    QDialog* dialog = new QDialog(this);
    dialog->setFixedSize(400, 300);

    // ����һ�� QVBoxLayout
    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // ȥ���ʺŰ�ť
    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ������ǩ����ӵ�������
    QLabel* label1 = new QLabel(dialog);
    label1->setWordWrap(true);
    label1->setAlignment(Qt::AlignJustify);
    label1->setText(QStringLiteral("<b>����һ��</b> ����DEM��EO�ļ����������ⷨ���������Ӱ�������ķ�Χ,"
        "�ٴӾ�����Ӱ����������������÷��ⷨ��ԭʼӰ���ڲ���ô���Ԫ��ֵ��ͬʱ���������Ӱ������ֵ��ͶӰ��Ϣ��"));

    QLabel* label2 = new QLabel(dialog);
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignJustify);
    label2->setText(QStringLiteral("<b>���ܶ���</b> ���ھ�������ķַ�΢�־���Ӱ���ȸ�������Ӱ������ƴ�Ӻ�Χ��������ÿ��"
        "Ӱ���ͶӰ�������ɶ�Ӧ��άŵͼ�����ɶ�Ӧ����Ĥ�ļ�����ÿ��Ӱ������Ĥ֮���ٽ�ÿ����Ĥ����������ս���У��õ�����Ӱ��"));

    QLabel* label3 = new QLabel(dialog);
    label3->setWordWrap(true);
    label3->setAlignment(Qt::AlignJustify);
    label3->setText(QStringLiteral("<b>��������</b> ���ھ�������ķַ�΢�־���Ӱ�����ҳ�����Ӱ����ص������ٴ��ص������и����ȡ�㣬"
        "ͨ��ȡ����ͬ����Ե�����ֵ�������Ӧϵ�������ø�ϵ���Դ�У��Ӱ���������õ�У�������Ĭ�ϵ���ĵ�һ��Ӱ��Ϊ�ο�Ӱ�񣬲���ǰһ�Ž����У�������"
        "�Ժ�һ��Ӱ�����У��,�Դ�һ�δ������Ӱ��"));

    layout->addWidget(label1);
    layout->addWidget(label2);
    layout->addWidget(label3);

    // ���öԻ���ı���
    dialog->setWindowTitle(QStringLiteral("����"));

    // ��ʾ�Ի���
    dialog->exec();
}

void testCRCS::About()
{
	// ����һ���µ� QDialog
	QDialog* dialog = new QDialog(this);
	dialog->setFixedSize(400, 200);

	// ����һ�� QVBoxLayout
	QVBoxLayout* layout = new QVBoxLayout(dialog);

	// ȥ���ʺŰ�ť
	dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ������ǩ����ӵ�������
    QLabel* label1 = new QLabel(dialog);
    label1->setMaximumSize(500, 70); 
    label1->setWordWrap(true);
    label1->setAlignment(Qt::AlignJustify);
    label1->setAlignment(Qt::AlignCenter); // �����������¾��ж���
    //label1->setContentsMargins(0, 0, 0, 0); // �������ݱ߾�
    label1->setText(QStringLiteral("<b>CRCS</b> ��һ������GDAL��ң��Ӱ���������"
        "�ṩ��΢�־�����Ӱ��ƴ�ӡ�Ӱ����ɫ�ȹ��ܡ�"));

    QLabel* label2 = new QLabel(dialog);
    label2->setMaximumSize(500, 70); 
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignJustify);
    label2->setAlignment(Qt::AlignCenter);
    //label2->setContentsMargins(0, 0, 0, 0); // �������ݱ߾�
    label2->setText(QStringLiteral("�汾��3.5.2"));

    QLabel* label3 = new QLabel(dialog);
    label3->setMaximumSize(500, 70); 
    label3->setWordWrap(true);
    label3->setAlignment(Qt::AlignJustify);
    label3->setAlignment(Qt::AlignCenter);
    //label3->setContentsMargins(0, 0, 0, 0); // �������ݱ߾�
    label3->setText(QStringLiteral("���ߣ�������ѧ2021��������Ӱ��������С��"));

    layout->addWidget(label1);
    layout->addWidget(label2);
    layout->addWidget(label3);
    //layout->setSpacing(10);


	// ���öԻ���ı���
	dialog->setWindowTitle(QStringLiteral("����"));

	// ��ʾ�Ի���
	dialog->exec();
}