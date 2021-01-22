#pragma once

#include <windows.h>
#include <QtWidgets/QMainWindow>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QtMultimedia/QSound>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QAbstractNativeEventFilter>

#include "defines.h"
#include "ui_keybd.h"

class keybd : public QMainWindow, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    keybd(QWidget *parent = Q_NULLPTR);
    ~keybd();

private:
    Q_SLOT void onBtnStart();
    Q_SLOT void onBtnStop();
    Q_SLOT void onBtnSave();
    Q_SLOT void onComboBoxHotkeyChanged(int index);

private:
    void initUiMember();
    void initUiData();
    void initTray();
    void loadConfig();
    void saveConfig();

    void changeState(bool startClick);

    int getRealHotkey(int index);  // 根据combobox索引找到真实hotkey
    void doRealHotkey(int timeId); // 根据定时器Id处理对应hotkey

protected:
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
    virtual void timerEvent(QTimerEvent *e);

private:
    Ui::keybdClass _ui;

    QComboBox *_comboBoxHotkey[MAX_HOTKEY_SIZE];
    QLineEdit *_lineEditHotkey[MAX_HOTKEY_SIZE];
    QCheckBox *_checkBoxHotkey[MAX_HOTKEY_SIZE];

    QComboBox *_comboBoxExcept[MAX_HOTKEY_SIZE];
    QLineEdit *_lineEditExcept[MAX_HOTKEY_SIZE];

    bool _started = false;
    Config _config;

    int _hotkeyTimeId[MAX_HOTKEY_SIZE] = { 0 };     // 按键定时器
    int _hotkeyExpireTime[MAX_HOTKEY_SIZE] = { 0 }; // 按键下次预期按键时间

    // 托盘
private:
    void closeEvent(QCloseEvent *e);// 关闭到托盘
    void changeEvent(QEvent *e);    // 最小化到托盘
    Q_SLOT void trayiconActivated(QSystemTrayIcon::ActivationReason reason);

    QSystemTrayIcon *_trayIcon;
    QAction *_minimizeAction;
    QAction *_restoreAction;
    QAction *_quitAction;
    QMenu   *_trayIconMenu;

    QSound _soundStart;	// 开启成功声音
    QSound _soundStop;
};
