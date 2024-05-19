#pragma once

#include <QDialog>
#include "ui_DC_Dlg.h"



class DC_Dlg : public QDialog
{
	Q_OBJECT

public:
	DC_Dlg(QWidget *parent = nullptr);
	~DC_Dlg();

private:
	Ui::DC_DlgClass ui;

	//testCRCS* testCRCSInstance_;

private slots:
	void Rimage(); //ԭʼӰ��
	void Parameter(); //��������
	void DEM(); //DEM
	void Output(); //���
	bool containsChinese(const QString& path);
	void DC();

signals:
	void progressChanged(int value);
};
