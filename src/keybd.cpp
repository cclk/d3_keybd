#include "keybd.h"

#include <timeapi.h>
#include <fstream>
#include <iostream>
#include <QMessageBox>
#include <QTimer>

#include <json/json.h>
#include <glog/logger.h>

namespace
{
enum KEY_EXCEPT_INDEX
{
    KEY_EMPTY = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_Q,
    KEY_G,
    KEY_T,
    KEY_MAX
};

static HHOOK sKeyboardHook = nullptr;
static int sKeyExceptTime[KEY_MAX] = { 0 }; // 配置的例外按键超期时间
static DWORD sExceptTick = 0; // 当前预期超期时间

std::string GetAppPath()
{
    std::string appPath;
#ifdef _WIN32
    char szAppPath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, szAppPath, MAX_PATH);
    (strrchr(szAppPath, '\\'))[0] = 0;		//结尾无斜杠
                                            //(strrchr(szAppPath, '\\'))[1] = 0;	// 结尾有斜杠
    appPath = szAppPath;
#else
    char szAppPath[1024] = { 0 };
    int rslt = readlink("/proc/self/exe", szAppPath, 1023);
    if (rslt < 0 || (rslt >= 1023))
    {
        appPath = "";
    }
    else
    {
        szAppPath[rslt] = '\0';
        for (int i = rslt; i >= 0; i--)
        {
            if (szAppPath[i] == '/')
            {
                szAppPath[i] = '\0';

                appPath = szAppPath;
                break;
            }
        }
    }
#endif

    return appPath;
}
};

keybd::keybd(QWidget *parent)
    : QMainWindow(parent)
{
    _ui.setupUi(this);
    InitLogger("d3_keybd");
    LOG_INFO << "========================";

    // 初始化时间种子
    qsrand(GetTickCount());

    // config
    loadConfig();

    // init
    initTray();
    initUiMember();
    initUiData();

    // connect
    connect(_ui.btnStart, SIGNAL(clicked()), this, SLOT(onBtnStart()));
    connect(_ui.btnStop, SIGNAL(clicked()), this, SLOT(onBtnStop()));
    connect(_ui.btnSave, SIGNAL(clicked()), this, SLOT(onBtnSave()));
    connect(_ui.comboBoxKey, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxHotkeyChanged(int)));

    // hotkey
    if (!::RegisterHotKey((HWND)this->winId(), HOTKEY_START, NULL, VK_F1 + _config.index))
    {
        QMessageBox::warning(NULL, "warning", "RegisterHotKey faild");
    }

    // 设置精度为1毫秒
    timeBeginPeriod(1);
}

keybd::~keybd()
{
    ::UnregisterHotKey((HWND)this->winId(), HOTKEY_START);

    onBtnStop();

    // 结束精度设置
    timeEndPeriod(1);
}

void keybd::onBtnStart()
{
    _started = true;

    // timer
    for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
    {
        _hotkeyTimeId[i] = startTimer(1);
    }

    // hook
    sKeyboardHook = ::SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);

    changeState(true);
}

void keybd::onBtnStop()
{
    _started = false;

    // timer
    for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
    {
        killTimer(_hotkeyTimeId[i]);
    }

    // hook
    if (sKeyboardHook != nullptr)
    {
        UnhookWindowsHookEx(sKeyboardHook);
        sKeyboardHook = nullptr;
    }

    changeState(false);
}

void keybd::onBtnSave()
{
    saveConfig();
}

void keybd::onComboBoxHotkeyChanged(int index)
{
    ::UnregisterHotKey((HWND)this->winId(), HOTKEY_START);

    if (!::RegisterHotKey((HWND)this->winId(), HOTKEY_START, NULL, VK_F1 + index))
    {
        QMessageBox::warning(NULL, "warning", "RegisterHotKey faild");
    }
}

void keybd::initUiMember()
{
    _comboBoxHotkey[0] = _ui.comboBoxHotkey1;
    _comboBoxHotkey[1] = _ui.comboBoxHotkey2;
    _comboBoxHotkey[2] = _ui.comboBoxHotkey3;
    _comboBoxHotkey[3] = _ui.comboBoxHotkey4;
    _comboBoxHotkey[4] = _ui.comboBoxHotkey5;

    _lineEditHotkey[0] = _ui.lineEditHotkey1;
    _lineEditHotkey[1] = _ui.lineEditHotkey2;
    _lineEditHotkey[2] = _ui.lineEditHotkey3;
    _lineEditHotkey[3] = _ui.lineEditHotkey4;
    _lineEditHotkey[4] = _ui.lineEditHotkey5;

    _checkBoxHotkey[0] = _ui.checkBoxHotkey1;
    _checkBoxHotkey[1] = _ui.checkBoxHotkey2;
    _checkBoxHotkey[2] = _ui.checkBoxHotkey3;
    _checkBoxHotkey[3] = _ui.checkBoxHotkey4;
    _checkBoxHotkey[4] = _ui.checkBoxHotkey5;

    _comboBoxExcept[0] = _ui.comboBoxExcept1;
    _comboBoxExcept[1] = _ui.comboBoxExcept2;
    _comboBoxExcept[2] = _ui.comboBoxExcept3;
    _comboBoxExcept[3] = _ui.comboBoxExcept4;
    _comboBoxExcept[4] = _ui.comboBoxExcept5;

    _lineEditExcept[0] = _ui.lineEditExcept1;
    _lineEditExcept[1] = _ui.lineEditExcept2;
    _lineEditExcept[2] = _ui.lineEditExcept3;
    _lineEditExcept[3] = _ui.lineEditExcept4;
    _lineEditExcept[4] = _ui.lineEditExcept5;
}

void keybd::initUiData()
{
    QStringList keys;
    keys << "F1" << "F2" << "F3" << "F4" << "F5" << "F6" << "F7" << "F8" << "F9" << "F10" << "F11" << "F12";
    _ui.comboBoxKey->addItems(keys);
    _ui.comboBoxKey->setCurrentIndex(_config.index);

    QStringList hotkeys;
    hotkeys << " " << "1" << "2" << "3" << "4" << "Q" << "G" << "T";

    for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
    {
        _comboBoxHotkey[i]->addItems(hotkeys);
        _comboBoxHotkey[i]->setCurrentIndex(_config.keyAuto[i].index);
        _lineEditHotkey[i]->setText(QString::number(_config.keyAuto[i].span));
        _checkBoxHotkey[i]->setChecked(_config.keyAuto[i].isRightStop);

        _comboBoxExcept[i]->addItems(hotkeys);
        _comboBoxExcept[i]->setCurrentIndex(_config.keyExcept[i].index);
        _lineEditExcept[i]->setText(QString::number(_config.keyExcept[i].time));
    }
}

void keybd::initTray()
{
    // 托盘
    // 创建托盘图标
    QIcon icon = QIcon(":/keybd/keybd.ico");
    _trayIcon = new QSystemTrayIcon(this);
    _trayIcon->setIcon(icon);
    _trayIcon->setToolTip(tr("d3_keybd"));
    QString titlec = tr("d3_keybd");
    QString textc = tr("keyboard for d3");
    _trayIcon->show();

    // 弹出气泡提示
    // m_trayIcon->showMessage(titlec, textc, QSystemTrayIcon::Information, 100);

    // 添加单/双击鼠标相应
    connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayiconActivated(QSystemTrayIcon::ActivationReason)));

    // 创建监听行为
    _minimizeAction = new QAction(tr(u8"最小化 (&I)"), this);
    connect(_minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    _restoreAction = new QAction(tr(u8"还原 (&R)"), this);
    connect(_restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    _quitAction = new QAction(tr(u8"退出 (&Q)"), this);
    connect(_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    // 创建右键弹出菜单
    _trayIconMenu = new QMenu(this);
    _trayIconMenu->addAction(_minimizeAction);
    _trayIconMenu->addAction(_restoreAction);
    _trayIconMenu->addSeparator();
    _trayIconMenu->addAction(_quitAction);

    _trayIcon->setContextMenu(_trayIconMenu);
}

void keybd::loadConfig()
{
    std::string file = GetAppPath() + "\\config.json";
    std::ifstream f(file, std::ios::binary);
    if (!f.is_open())
    {
        std::cout << "LoadConfig file open failed" << std::endl;
        return;
    }

    Json::Reader r;
    Json::Value root;

    if (!r.parse(f, root))
    {
        std::cout << "LoadConfig json parse failed" << std::endl;
        return;
    }

    auto index = root["index"];
    auto keyAuto = root["keyAuto"];
    auto keyExcept = root["keyExcept"];

    try
    {
        _config.index = index.asInt();

        for (int i = 0; i < min(MAX_HOTKEY_SIZE, keyAuto.size()); ++i)
        {
            _config.keyAuto[i].index = keyAuto[i]["index"].asInt();
            _config.keyAuto[i].span = keyAuto[i]["span"].asInt();
            _config.keyAuto[i].isRightStop = keyAuto[i]["isRightStop"].asBool();
        }

        for (int i = 0; i < min(MAX_HOTKEY_SIZE, keyExcept.size()); ++i)
        {
            _config.keyExcept[i].index = keyExcept[i]["index"].asInt();
            _config.keyExcept[i].time = keyExcept[i]["time"].asInt();

            sKeyExceptTime[_config.keyExcept[i].index] = _config.keyExcept[i].time;
        }
    }
    catch (...)
    {
    }
}

void keybd::saveConfig()
{
    // data
    {
        _config.index = _ui.comboBoxKey->currentIndex();

        for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
        {
            _config.keyAuto[i].index = _comboBoxHotkey[i]->currentIndex();
            _config.keyAuto[i].span = _lineEditHotkey[i]->text().toInt();
            _config.keyAuto[i].isRightStop = _checkBoxHotkey[i]->isChecked();

            _config.keyExcept[i].index = _comboBoxExcept[i]->currentIndex();
            _config.keyExcept[i].time = _lineEditExcept[i]->text().toInt();

            sKeyExceptTime[_config.keyExcept[i].index] = _config.keyExcept[i].time;
        }
    }

    // json
    {
        // index
        Json::Value root;
        root["index"] = _config.index;

        // keyAuto
        Json::Value keyAuto;
        for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
        {
            Json::Value item;
            item[i]["index"] = _config.keyAuto[i].index;
            item[i]["span"] = _config.keyAuto[i].span;
            item[i]["isRightStop"] = _config.keyAuto[i].isRightStop;

            keyAuto.append(item[i]);
        }
        root["keyAuto"] = keyAuto;

        // keyExcept
        Json::Value keyExcept;
        for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
        {
            Json::Value item;
            item[i]["index"] = _config.keyExcept[i].index;
            item[i]["time"] = _config.keyExcept[i].time;

            keyExcept.append(item[i]);
        }
        root["keyExcept"] = keyExcept;

        // write
        Json::FastWriter fw;
        std::ofstream os;
        std::string file = GetAppPath() + "\\config.json";
        os.open(file);
        os << fw.write(root);
        os.close();
    }
}

void keybd::changeState(bool startClick)
{
    _ui.comboBoxKey->setEnabled(!startClick);
    _ui.btnStart->setEnabled(!startClick);
    _ui.btnStop->setEnabled(startClick);
    _ui.btnSave->setEnabled(!startClick);

    for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
    {
        _comboBoxHotkey[i]->setEnabled(!startClick);
        _lineEditHotkey[i]->setEnabled(!startClick);
        _checkBoxHotkey[i]->setEnabled(!startClick);

        _comboBoxExcept[i]->setEnabled(!startClick);
        _lineEditExcept[i]->setEnabled(!startClick);
    }
}

int keybd::getRealHotkey(int index)
{
    switch (index)
    {
    case 1:
        return '1';
    case 2:
        return '2';
    case 3:
        return '3';
    case 4:
        return '4';
    case 5:
        return 'Q';
    case 6:
        return 'G';
    case 7:
        return 'T';

    default:
        return -1;
    }
}

void keybd::doRealHotkey(int timeId)
{
    int index = -1;
    for (int i = 0; i < MAX_HOTKEY_SIZE; ++i)
    {
        if (_hotkeyTimeId[i] == timeId)
        {
            index = i;
            break;
        }
    }

    if (index < 0 || index >= MAX_HOTKEY_SIZE)
    {
        return;
    }

    int key = getRealHotkey(_config.keyAuto[index].index);
    if (-1 == key)
    {
        return;
    }

    bool rbDown = GetAsyncKeyState(VK_RBUTTON) & 0x8000; // 鼠标右键按下
    bool rbRightStop = _config.keyAuto[index].isRightStop;
    if (rbDown && rbRightStop)
    {
        return;
    }

    DWORD tick = GetTickCount();
    if (tick > _hotkeyExpireTime[index])
    {
        keybd_event(key, 0, 0, 0); // keydown
        keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
        LOG_INFO << "keybd_event key: " << char(key);

        _hotkeyExpireTime[index] = tick + _config.keyAuto[index].span;
    }
}

bool keybd::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
    {
        MSG* pMsg = reinterpret_cast<MSG*>(message);
        if (pMsg->message == WM_HOTKEY)
        {
            if (pMsg->wParam == HOTKEY_START)
            {
                if (_started)
                {
                    onBtnStop();
                }
                else
                {
                    onBtnStart();
                }

                return true;
            }
        }
    }

    return false;
}

void keybd::timerEvent(QTimerEvent *e)
{
    // 例外按键时间范围内，不执行
    DWORD tick = GetTickCount();
    if (tick < sExceptTick)
    {
        return;
    }

    // 处理对应hotkey
    doRealHotkey(e->timerId());

    // 随机休眠，防止完全一致
    int sleepMSec = qrand() % 10; // 10 ms
    Sleep(sleepMSec);
}

void keybd::closeEvent(QCloseEvent *e)
{
    e->ignore();
    this->hide();
}

void keybd::changeEvent(QEvent *e)
{
    if ((e->type() == QEvent::WindowStateChange) && this->isMinimized())
    {
        // 定时器单次有效，只计时一次而不重复计时
        QTimer::singleShot(100, this, SLOT(close()));
    }
}

void keybd::trayiconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger: // 单击托盘图标
    case QSystemTrayIcon::DoubleClick: // 双击托盘图标
        this->showNormal();
        this->raise();
        ::SetForegroundWindow((HWND)this->winId());
        break;

    default:
        break;
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT*)lParam;

    if (nCode == HC_ACTION)
    {
        // if (pkbhs->vkCode == VK_BACK && GetAsyncKeyState(VK_CONTROL) & 0x8000)
        // else if(pkbhs->vkCode == VK_LWIN || pkbhs->vkCode == VK_RWIN)

        if (pkbhs->vkCode == '1' && sKeyExceptTime[KEY_1] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_1];
            LOG_INFO << "pressed key[1], sKeyExceptTime: " << sKeyExceptTime[KEY_1];
        }
        else if (pkbhs->vkCode == '2' && sKeyExceptTime[KEY_2] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_2];
            LOG_INFO << "pressed key[2], sKeyExceptTime: " << sKeyExceptTime[KEY_2];
        }
        else if (pkbhs->vkCode == '3' && sKeyExceptTime[KEY_3] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_3];
            LOG_INFO << "pressed key[3], sKeyExceptTime: " << sKeyExceptTime[KEY_3];
        }
        else if (pkbhs->vkCode == '4' && sKeyExceptTime[KEY_4] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_4];
            LOG_INFO << "pressed key[4], sKeyExceptTime: " << sKeyExceptTime[KEY_4];
        }
        else if (pkbhs->vkCode == 'Q' && sKeyExceptTime[KEY_Q] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_Q];
            LOG_INFO << "pressed key[Q], sKeyExceptTime: " << sKeyExceptTime[KEY_Q];
        }
        else if (pkbhs->vkCode == 'G' && sKeyExceptTime[KEY_G] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_G];
            LOG_INFO << "pressed key[G], sKeyExceptTime: " << sKeyExceptTime[KEY_G];
        }
        else if (pkbhs->vkCode == 'T' && sKeyExceptTime[KEY_T] > 0)
        {
            sExceptTick = GetTickCount() + sKeyExceptTime[KEY_T];
            LOG_INFO << "pressed key[T], sKeyExceptTime: " << sKeyExceptTime[KEY_T];
        }

        return 0; // 返回1表示截取消息不再传递，返回0表示不作处理,消息继续传递
    }

    return CallNextHookEx(sKeyboardHook, nCode, wParam, lParam);
}