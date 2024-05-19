
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

	// �����󻯰�ť
	setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
	// �����С����ť
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    // ���ô��ڱ���
    setWindowTitle(QStringLiteral("�ַ�΢�־���"));

	// ���ô�С�̶�
	setFixedSize(520, 350);

	// ���ô���ͼ��
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
	// �����ļ��Ի���
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Images"), ".", tr("Image Files(*.jpg *.png)"));
    if (fileNames.isEmpty()) {
        QMessageBox::information(this, tr("Error"), tr("Please select image files"));
        return;
    }
    // ���ļ����б�ת��Ϊһ���ַ������ļ���֮���ö��ŷָ�
    QString allFileNames = fileNames.join(", ");
    // ���ַ�������Ϊ�༭�������
    ui.ImageEdit->setText(allFileNames);
}

void DC_Dlg::Parameter()
{
    // �ļ��Ի���CSV�ļ�
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CSV File"), ".", tr("CSV Files(*.csv)"));
    if (fileName.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a CSV file"));
		return;
	}
    // ���ļ�������Ϊ�༭�������
	ui.Edit_par->setText(fileName);
}

void DC_Dlg::DEM()
{ 
	// �ļ��Ի���DEM�ļ�
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open DEM File"), ".", tr("DEM Files(*.tif)"));
    if (fileName.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please select a DEM file"));
		return;
	}
	// ���ļ�������Ϊ�༭�������
	ui.Edit_dem->setText(fileName);
}

void DC_Dlg::Output()
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

bool DC_Dlg::containsChinese(const QString& path) {
	QRegExp rx("[\\x4e00-\\x9fa5]");
	return rx.indexIn(path) >= 0;
}

void DC_Dlg::DC()
{
	// ��ȡ�༭�������
	QStringList imagePaths = ui.ImageEdit->text().split(", ");
	QString csvPath = ui.Edit_par->text();
	QString demPath = ui.Edit_dem->text();
	QString outputpath = ui.Edit_output->text();

	QString selectedText = ui.comboBox->currentText();
	double value = selectedText.toDouble();


	// ���·���Ƿ�Ϊ��
	if (imagePaths.isEmpty() || csvPath.isEmpty() || demPath.isEmpty() || outputpath.isEmpty()) {
		QMessageBox::information(this, tr("Error"), tr("Please make sure all fields are filled"));
		return;
	}
	else
	{
		// ���·���Ƿ���������ַ�
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

	// ����testDC����
	for (const QString& imagePath : imagePaths) {

		testDC testdc(imagePath.toStdString(), csvPath.toStdString(), demPath.toStdString(), outputpath.toStdString());
		// ����process����
		testdc.process(value);

		// ���½�������ֵ
		processedImages++;
		int progressValue = qRound((double)processedImages * 100 / totalImages);
		emit progressChanged(progressValue);

		// qDebug() << "progressValue: " << progressValue;
	}

	t = timer.elapsed();
	// ��������Ӱ����ʾ��testCRCS��
	// testCRCSInstance_->populateTreeView(correctedImagePaths);

	// ��ʾ������ɣ�����ʾ����ʱ��
	QMessageBox::information(this, tr("Information"), tr("Processing completed, time: %1 ms").arg(t));
}