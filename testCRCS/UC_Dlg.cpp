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

	// ���ô��ڱ���
	setWindowTitle(QStringLiteral("Ӱ����ɫ"));

	// ���ô�С�̶�
	setFixedSize(430, 210);

	// �رհ�����ť
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui.Rawimage, &QPushButton::clicked, this, &UC_Dlg::Rimage);
	connect(ui.pb_output, &QPushButton::clicked, this, &UC_Dlg::Output);
	connect(ui.ok, &QPushButton::clicked, this, &UC_Dlg::UC);
}

UC_Dlg::~UC_Dlg()
{}


void UC_Dlg::Rimage() {
    // �����ļ��Ի���
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Images"), ".", tr("Image Files(*.tif)"));
    if (fileNames.isEmpty()) {
        QMessageBox::information(this, tr("Error"), tr("Please select image files"));
        return;
    }
    // ���ļ����б�ת��Ϊһ���ַ������ļ���֮���ö��ŷָ�
    QString allFileNames = fileNames.join(", ");
    // ���ַ�������Ϊ�༭�������
    ui.ImageEdit->setText(allFileNames);
}

void UC_Dlg::Output()
{
	// �ļ��Ի�������ļ���
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ".", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a directory"));
		return;
	}
	// ���ļ���·������Ϊ�༭�������
	ui.Edit_output->setText(dir);
}

void UC_Dlg::UC()
{
	// ��ȡ�༭�������
	QStringList images = ui.ImageEdit->text().split(", ");
	QString outputpath = ui.Edit_output->text();

	// ע�����е�����
	GDALAllRegister();

	std::vector<std::string> imagePaths;
	for (const QString& path : images) {
		imagePaths.push_back(path.toStdString());
	}

	// ��ȡ��һ��Ӱ����Ϊ�ο�Ӱ��
	GDALDataset* poRefDataset = (GDALDataset*)GDALOpen(imagePaths[0].c_str(), GA_ReadOnly);
	if (poRefDataset == NULL) {
		// std::cout << "Open failed" << std::endl;
		return;
	}

	// ѭ������ÿһ����У��Ӱ��
	for (int imageIndex = 1; imageIndex < imagePaths.size(); ++imageIndex) {
		// ��ȡ��У��Ӱ��
		GDALDataset* poDataset = (GDALDataset*)GDALOpen(imagePaths[imageIndex].c_str(), GA_ReadOnly);
		if (poDataset == NULL) {
			// std::cout << "Open failed" << std::endl;
			return;
		}

		processImages(poRefDataset, poDataset, imagePaths[imageIndex], outputpath.toStdString());

		// У����ɺ󣬹رղο�Ӱ��
		GDALClose(poRefDataset);

		// ��У�����Ӱ������Ϊ��һ�ε����Ĳο�Ӱ��
		poRefDataset = poDataset;
	}
	// �ر����һ��Ӱ��
	GDALClose(poRefDataset);

	// �ͷ��ڴ�
	GDALDestroyDriverManager();

	// ��ʾ������ɵ���Ϣ
	QMessageBox::information(this, tr("Information"), tr("Processing completed"));
	return;
}
