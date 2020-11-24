#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_keybd.h"

class keybd : public QMainWindow
{
    Q_OBJECT

public:
    keybd(QWidget *parent = Q_NULLPTR);

private:
    Ui::keybdClass ui;
};
