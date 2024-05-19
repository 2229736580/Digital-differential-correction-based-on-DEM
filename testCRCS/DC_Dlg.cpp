
#include "DC_Dlg.h"
#include"testDC.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTime>

#include <QDebug>

DC_Dlg::DC_Dlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	// 添加最大化按钮
	setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
	// 添加最小化按钮
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    // 设置窗口标题
    setWindowTitle(QStringLiteral("分幅微分纠正"));

	// 设置大小固定
	setFixedSize(520, 350);

	// 设置窗口图标
	setWindowIcon(QIcon(":/testCRCS/icon/dc1.png"));


	connect(ui.Rawimage, &QPushButton::clicked, this, &DC_Dlg::Rimage);
    connect(ui.pb_par, &QPushButton::clicked, this, &DC_Dlg::Parameter);
    connect(ui.pb_dem, &QPushButton::clicked, this, &DC_Dlg::DEM);
	connect(ui.pb_output, &QPushButton::clicked, this, &DC_Dlg::Output);
	connect(ui.pb_DC, &QPushButton::clicked, this, &DC_Dlg::DC);
}

DC_Dlg::~DC_Dlg()
{}

void DC_Dlg::Rimage() {
	// 弹出文件对话框
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Images"), ".", tr("Image Files(*.jpg *.png)"));
    if (fileNames.isEmpty()) {
        QMessageBox::information(this, tr("Error"), tr("Please select image files"));
        return;
    }
    // 将文件名列表转换为一个字符串，文件名之间用逗号分隔
    QString allFileNames = fileNames.join(", ");
    // 将字符串设置为编辑框的内容
    ui.ImageEdit->setText(allFileNames);
}

void DC_Dlg::Parameter()
{
    // 文件对话框，CSV文件
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CSV File"), ".", tr("CSV Files(*.csv)"));
    if (fileName.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a CSV file"));
		return;
	}
    // 将文件名设置为编辑框的内容
	ui.Edit_par->setText(fileName);
}

void DC_Dlg::DEM()
{ 
	// 文件对话框，DEM文件
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open DEM File"), ".", tr("DEM Files(*.tif)"));
    if (fileName.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a DEM file"));
		return;
	}
	// 将文件名设置为编辑框的内容
	ui.Edit_dem->setText(fileName);
}

void DC_Dlg::Output()
{
	// 文件对话框，输出文件夹
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ".", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a directory"));
		return;
	}
	// 将文件夹路径设置为编辑框的内容
	ui.Edit_output->setText(dir);
}

bool DC_Dlg::containsChinese(const QString& path) {
	QRegExp rx("[\\x4e00-\\x9fa5]");
	return rx.indexIn(path) >= 0;
}

void DC_Dlg::DC()
{
	// 获取编辑框的内容
	QStringList imagePaths = ui.ImageEdit->text().split(", ");
	QString csvPath = ui.Edit_par->text();
	QString demPath = ui.Edit_dem->text();
	QString outputpath = ui.Edit_output->text();

	QString selectedText = ui.comboBox->currentText();
	double value = selectedText.toDouble();


	// 检查路径是否为空
	if (imagePaths.isEmpty() || csvPath.isEmpty() || demPath.isEmpty() || outputpath.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please make sure all fields are filled"));
		return;
	}
	else
	{
		// 检查路径是否包含中文字符
		for (const QString& path : imagePaths) {
			if (containsChinese(path)) {
				QMessageBox::warning(this, tr("Warning"), tr("The image path contains Chinese characters, which may cause problems."));
				return;
			}
		}
		if (containsChinese(csvPath)) {
			QMessageBox::warning(this, tr("Warning"), tr("The CSV file path contains Chinese characters, which may cause problems."));
			return;
		}
		if (containsChinese(demPath)) {
			QMessageBox::warning(this, tr("Warning"), tr("The DEM file path contains Chinese characters, which may cause problems."));
			return;
		}
		if (containsChinese(outputpath)) {
			QMessageBox::warning(this, tr("Warning"), tr("The output path contains Chinese characters, which may cause problems."));
			return;
		}
	}

	QStringList correctedImagePaths;

	int t;

	QTime timer;
	timer.start();

	int totalImages = imagePaths.size();
	int processedImages = 0;

	// 创建testDC对象
	for (const QString& imagePath : imagePaths) {

		testDC testdc(imagePath.toStdString(), csvPath.toStdString(), demPath.toStdString(), outputpath.toStdString());
		// 调用process方法
		testdc.process(value);

		// 更新进度条的值
		processedImages++;
		int progressValue = qRound((double)processedImages * 100 / totalImages);
		emit progressChanged(progressValue);

		// qDebug() << "progressValue: " << progressValue;
	}

	t = timer.elapsed();
	// 将处理后的影像显示在testCRCS中
	// testCRCSInstance_->populateTreeView(correctedImagePaths);

	// 提示处理完成，并显示处理时间
	QMessageBox::information(this, tr("Information"), tr("Processing completed, time: %1 ms").arg(t));
}