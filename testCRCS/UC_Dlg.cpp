#include "UC_Dlg.h"

#include <QFileDialog>
#include <QMessageBox>
#include "gdal_priv.h"
#include <vector>

#include "testUC.h"

#include "testUC1.h"
#include "testUC2.h"

UC_Dlg::UC_Dlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	setWindowIcon(QIcon(":/testCRCS/icon/uc1.png"));

	// 设置窗口标题
	setWindowTitle(QStringLiteral("影像匀色"));

	// 设置大小固定
	setFixedSize(430, 210);

	// 关闭帮助按钮
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui.Rawimage, &QPushButton::clicked, this, &UC_Dlg::Rimage);
	connect(ui.pb_output, &QPushButton::clicked, this, &UC_Dlg::Output);
	connect(ui.ok, &QPushButton::clicked, this, &UC_Dlg::UC);
}

UC_Dlg::~UC_Dlg()
{}


void UC_Dlg::Rimage() {
    // 弹出文件对话框
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Images"), ".", tr("Image Files(*.tif)"));
    if (fileNames.isEmpty()) {
        QMessageBox::information(this, tr("Error"), tr("Please select image files"));
        return;
    }
    // 将文件名列表转换为一个字符串，文件名之间用逗号分隔
    QString allFileNames = fileNames.join(", ");
    // 将字符串设置为编辑框的内容
    ui.ImageEdit->setText(allFileNames);
}

void UC_Dlg::Output()
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

void UC_Dlg::UC()
{
	// 获取编辑框的内容
	QStringList images = ui.ImageEdit->text().split(", ");
	QString outputpath = ui.Edit_output->text();

	// 注册所有的驱动
	GDALAllRegister();

	std::vector<std::string> imagePaths;
	for (const QString& path : images) {
		imagePaths.push_back(path.toStdString());
	}

	// 读取第一幅影像作为参考影像
	GDALDataset* poRefDataset = (GDALDataset*)GDALOpen(imagePaths[0].c_str(), GA_ReadOnly);
	if (poRefDataset == NULL) {
		// std::cout << "Open failed" << std::endl;
		return;
	}

	// 循环处理每一幅待校正影像
	for (int imageIndex = 1; imageIndex < imagePaths.size(); ++imageIndex) {
		// 读取待校正影像
		GDALDataset* poDataset = (GDALDataset*)GDALOpen(imagePaths[imageIndex].c_str(), GA_ReadOnly);
		if (poDataset == NULL) {
			// std::cout << "Open failed" << std::endl;
			return;
		}

		processImages(poRefDataset, poDataset, imagePaths[imageIndex], outputpath.toStdString());

		// 校正完成后，关闭参考影像
		GDALClose(poRefDataset);

		// 将校正后的影像设置为下一次迭代的参考影像
		poRefDataset = poDataset;
	}
	// 关闭最后一幅影像
	GDALClose(poRefDataset);

	// 释放内存
	GDALDestroyDriverManager();

	// 显示处理完成的消息
	QMessageBox::information(this, tr("Information"), tr("Processing completed"));
	return;
}
