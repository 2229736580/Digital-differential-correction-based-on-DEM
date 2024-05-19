#pragma once

#include <QDialog>
#include "ui_UC_Dlg.h"

class UC_Dlg : public QDialog
{
	Q_OBJECT

public:
	UC_Dlg(QWidget *parent = nullptr);
	~UC_Dlg();

	void Rimage();

	void Output();

	void UC();

private:
	Ui::UC_DlgClass ui;
};
