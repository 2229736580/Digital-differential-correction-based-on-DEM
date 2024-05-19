#include "IM_Dlg.h"

#include <QFileDialog>
#include <QMessageBox>
#include "DC_Dlg.h"
#include "testIM.h"

IM_Dlg::IM_Dlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	// �����󻯰�ť
	setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
	// �����С����ť
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	// ���ô��ڱ���
	setWindowTitle(QStringLiteral("Ӱ��ƴ��"));

	// ���ô�С�̶�
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
	// ���ļ����б�ת��Ϊһ���ַ������ļ���֮���ö��ŷָ�
	QString allFileNames = fileNames.join(", ");

	// ���ļ�������Ϊ�༭�������
	ui.Edit_in->setText(allFileNames);
}

void IM_Dlg::OutDir() {
	// �ļ��Ի�������ļ�
	QString file = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Image Files(*.tif)"));
	if (file.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a file"));
		return;
	}
	// ���ļ�������Ϊ�༭�������
	ui.Edit_out->setText(file);
}

void IM_Dlg::IM()
{
	// ��ȡ�༭�������
	QStringList imagePaths = ui.Edit_in->text().split(", ");

	// �� QStringList ת��Ϊ std::vector<std::string>
	std::vector<std::string> imagePathsStd;
	for (const auto& path : imagePaths) {
		imagePathsStd.push_back(path.toStdString());
	}

	// ��ȡ����ļ���·��������
	QString outputFile = ui.Edit_out->text();
	if (outputFile.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a file"));
		return;
	}

	// ���� testIM ���ʵ��
	testIM imProcessor(imagePathsStd, outputFile.toStdString());

	// ���� process ����
	imProcessor.process();

	// ��ʾ������ɵ���Ϣ
	QMessageBox::information(this, tr("Information"), tr("Image stitching completed"));
}