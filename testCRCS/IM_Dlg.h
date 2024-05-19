#pragma once

#include <QDialog>
#include "ui_IM_Dlg.h"

class IM_Dlg : public QDialog
{
	Q_OBJECT
public:
	IM_Dlg(QWidget *parent = nullptr);
	~IM_Dlg();

private slots:
	void InDir();
	void OutDir();

	void IM();
private:
	Ui::IM_DlgClass ui;

//signals:
	// void progressChanged(int progress);
};
