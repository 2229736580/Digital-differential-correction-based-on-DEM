#include "IM_Dlg.h"

#include <QFileDialog>
#include <QMessageBox>
#include "DC_Dlg.h"
#include "testIM.h"

IM_Dlg::IM_Dlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	// 添加最大化按钮
	setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
	// 添加最小化按钮
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	// 设置窗口标题
	setWindowTitle(QStringLiteral("影像拼接"));

	// 设置大小固定
	setFixedSize(450, 170);

	setWindowIcon(QIcon(":/testCRCS/icon/im.png"));

	connect(ui.pb_in, &QPushButton::clicked, this, &IM_Dlg::InDir);
	connect(ui.pb_out, &QPushButton::clicked, this, &IM_Dlg::OutDir);
	connect(ui.pb_IM, &QPushButton::clicked, this, &IM_Dlg::IM);
}

IM_Dlg::~IM_Dlg()
{}

void IM_Dlg::InDir() {
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Images"), ".", tr("Image Files(*.tif)"));
	if (fileNames.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select image files"));
		return;
	}
	// 将文件名列表转换为一个字符串，文件名之间用逗号分隔
	QString allFileNames = fileNames.join(", ");

	// 将文件名设置为编辑框的内容
	ui.Edit_in->setText(allFileNames);
}

void IM_Dlg::OutDir() {
	// 文件对话框，输出文件
	QString file = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Image Files(*.tif)"));
	if (file.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a file"));
		return;
	}
	// 将文件名设置为编辑框的内容
	ui.Edit_out->setText(file);
}

void IM_Dlg::IM()
{
	// 获取编辑框的内容
	QStringList imagePaths = ui.Edit_in->text().split(", ");

	// 将 QStringList 转换为 std::vector<std::string>
	std::vector<std::string> imagePathsStd;
	for (const auto& path : imagePaths) {
		imagePathsStd.push_back(path.toStdString());
	}

	// 获取输出文件的路径和名称
	QString outputFile = ui.Edit_out->text();
	if (outputFile.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a file"));
		return;
	}

	// 创建 testIM 类的实例
	testIM imProcessor(imagePathsStd, outputFile.toStdString());

	// 调用 process 方法
	imProcessor.process();

	// 显示处理完成的消息
	QMessageBox::information(this, tr("Information"), tr("Image stitching completed"));
}