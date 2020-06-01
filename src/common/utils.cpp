/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 * Maintainer: rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include "../views/operationconfirmdlg.h"
#include "warnningdlg.h"
#include "termwidget.h"

#include <DLog>
#include <DMessageBox>
#include <DLineEdit>
#include <DFileDialog>

#include <QDBusMessage>
#include <QDBusConnection>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QMimeType>
#include <QApplication>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QImageReader>
#include <QPixmap>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QTextLayout>
#include <QTime>
#include <QFontMetrics>
#include "terminputdialog.h"

QHash<QString, QPixmap> Utils::m_imgCacheHash;
QHash<QString, QString> Utils::m_fontNameCache;

Utils::Utils(QObject *parent) : QObject(parent)
{
}

Utils::~Utils()
{
}

QString Utils::getQssContent(const QString &filePath)
{
    QFile file(filePath);
    QString qss;

    if (file.open(QIODevice::ReadOnly)) {
        qss = file.readAll();
    }

    return qss;
}

QString Utils::getConfigPath()
{
    QDir dir(
        QDir(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first()).filePath(qApp->organizationName()));

    return dir.filePath(qApp->applicationName());
}

QString Utils::suffixList()
{
    return QString("Font Files (*.ttf *.ttc *.otf)");
}

QPixmap Utils::renderSVG(const QString &filePath, const QSize &size)
{
    if (m_imgCacheHash.contains(filePath)) {
        return m_imgCacheHash.value(filePath);
    }

    QImageReader reader;
    QPixmap pixmap;

    reader.setFileName(filePath);

    if (reader.canRead()) {
        const qreal ratio = qApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    } else {
        pixmap.load(filePath);
    }

    m_imgCacheHash.insert(filePath, pixmap);

    return pixmap;
}

QString Utils::loadFontFamilyFromFiles(const QString &fontFileName)
{
    if (m_fontNameCache.contains(fontFileName)) {
        return m_fontNameCache.value(fontFileName);
    }

    QString fontFamilyName = "";

    QFile fontFile(fontFileName);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Open font file error";
        return fontFamilyName;
    }

    int loadedFontID = QFontDatabase::addApplicationFontFromData(fontFile.readAll());
    QStringList loadedFontFamilies = QFontDatabase::applicationFontFamilies(loadedFontID);
    if (!loadedFontFamilies.empty()) {
        fontFamilyName = loadedFontFamilies.at(0);
    }
    fontFile.close();

    m_fontNameCache.insert(fontFileName, fontFamilyName);
    return fontFamilyName;
}

QString Utils::getElidedText(QFont font, QString text, int MaxWith)
{
    if (text.isEmpty()) {
        return "";
    }

    QFontMetrics fontWidth(font);

    // 计算字符串宽度
    int width = fontWidth.width(text);

    // 当字符串宽度大于最大宽度时进行转换
    if (width >= MaxWith) {
        // 右部显示省略号
        text = fontWidth.elidedText(text, Qt::ElideRight, MaxWith);
    }

    return text;
}

const QString Utils::holdTextInRect(const QFont &font, QString text, const QSize &size)
{
    QFontMetrics fm(font);
    QTextLayout layout(text);

    layout.setFont(font);

    QStringList lines;
    QTextOption &text_option = *const_cast<QTextOption *>(&layout.textOption());

    text_option.setWrapMode(QTextOption::WordWrap);
    text_option.setAlignment(Qt::AlignTop | Qt::AlignLeft);

    layout.beginLayout();

    QTextLine line = layout.createLine();
    int height = 0;
    int lineHeight = fm.height();

    while (line.isValid()) {
        height += lineHeight;

        if (height + lineHeight > size.height()) {
            const QString &end_str = fm.elidedText(text.mid(line.textStart()), Qt::ElideRight, size.width());

            layout.endLayout();
            layout.setText(end_str);

            text_option.setWrapMode(QTextOption::NoWrap);
            layout.beginLayout();
            line = layout.createLine();
            line.setLineWidth(size.width() - 1);
            text = end_str;
        } else {
            line.setLineWidth(size.width());
        }

        lines.append(text.mid(line.textStart(), line.textLength()));

        if (height + lineHeight > size.height())
            break;

        line = layout.createLine();
    }

    layout.endLayout();

    return lines.join("");
}

QString Utils::convertToPreviewString(QString fontFilePath, QString srcString)
{
    if (fontFilePath.isEmpty()) {
        return srcString;
    }

    QString strFontPreview = srcString;

    QRawFont rawFont(fontFilePath, 0, QFont::PreferNoHinting);
    bool isSupport = rawFont.supportsCharacter(QChar('a'));
    bool isSupportF = rawFont.supportsCharacter(QChar('a' | 0xf000));
    if ((!isSupport && isSupportF)) {
        QChar *chArr = new QChar[srcString.length() + 1];
        for (int i = 0; i < srcString.length(); i++) {
            int ch = srcString.at(i).toLatin1();
            //判断字符ascii在32～126范围内(共95个)
            if (ch >= 32 && ch <= 126) {
                ch |= 0xf000;
                chArr[i] = QChar(ch);
            } else {
                chArr[i] = srcString.at(i);
            }
        }
        chArr[srcString.length()] = '\0';
        QString strResult(chArr);
        strFontPreview = strResult;
        delete[] chArr;
    }

    return strFontPreview;
}

QString Utils::getRandString()
{
    int max = 6;  //字符串长度
    QString tmp = QString("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    QString str;
    QTime t;
    t = QTime::currentTime();
    qsrand(t.msec() + t.second() * 1000);
    for (int i = 0; i < max; i++) {
        int len = qrand() % tmp.length();
        str[i] = tmp.at(len);
    }
    return QString(str);
}

QString Utils::showDirDialog(QWidget *widget)
{
    QString curPath = QDir::currentPath();
    QString dlgTitle = QObject::tr("Select a directory to save the file");
    return DFileDialog::getExistingDirectory(widget, dlgTitle, curPath, DFileDialog::DontConfirmOverwrite);
}

QStringList Utils::showFilesSelectDialog(QWidget *widget)
{
    QString curPath = QDir::currentPath();
    QString dlgTitle = QObject::tr("Select file to upload");
    return DFileDialog::getOpenFileNames(widget, dlgTitle, curPath);
}

bool Utils::showExitConfirmDialog(CloseType type, int count)
{
    /******** Modify by m000714 daizhengwen 2020-04-17: 统一使用dtk Dialog****************/
#ifndef USE_DTK
    OperationConfirmDlg optDlg;
    optDlg.setFixedSize(380, 160);
    optDlg.setOperatTypeName(QObject::tr("Programs are still running in terminal"));
    optDlg.setTipInfo(QObject::tr("Are you sure you want to exit?"));
    optDlg.setOKCancelBtnText(QObject::tr("Exit"), QObject::tr("Cancel"));
    optDlg.exec();

    return (optDlg.getConfirmResult() == QDialog::Accepted);
#else
    // count < 1 不提示
    if (count < 1) {
        return true;
    }
    QString title;
    QString txt;
    if (type != CloseType_Window) {
        // 默认的count = 1的提示
        title = QObject::tr("Close this terminal?");
        txt = QObject::tr("There is still a process running in this terminal. "
                          "Closing the terminal will kill it.");
        // count > 1 提示
        if (count > 1) {
            txt = QObject::tr("There are still %1 processes running in this terminal. "
                              "Closing the terminal will kill all of them.")
                  .arg(count);
        }
    } else {
        title = QObject::tr("Close this window?");
        txt = QObject::tr("There are still processes running in this window. Closing the window will kill all of them.");
    }

    DDialog dlg(title, txt);
    dlg.setIcon(QIcon::fromTheme("deepin-terminal"));
    dlg.addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    /******** Modify by nt001000 renfeixiang 2020-05-21:修改Exit成Close Begin***************/
    dlg.addButton(QString(tr("Close")), true, DDialog::ButtonWarning);
    /******** Modify by nt001000 renfeixiang 2020-05-21:修改Exit成Close End***************/
    return (dlg.exec() == DDialog::Accepted);
#endif
    /********************* Modify by m000714 daizhengwen End ************************/
}

void Utils::getExitDialogText(CloseType type, QString &title, QString &txt, int count)
{
    // count < 1 不提示
    if (count < 1) {
        return ;
    }
    //QString title;
    //QString txt;
    if (type == CloseType_Window) {
        title = QObject::tr("Close this window?");
        txt = QObject::tr("There are still processes running in this window. Closing the window will kill all of them.");
    } else {
        // 默认的count = 1的提示
        title = QObject::tr("Close this terminal?");
        txt = QObject::tr("There is still a process running in this terminal. "
                          "Closing the terminal will kill it.");
        // count > 1 提示
        if (count > 1) {
            txt = QObject::tr("There are still %1 processes running in this terminal. "
                              "Closing the terminal will kill all of them.")
                  .arg(count);
        }
    }
}

bool Utils::showExitUninstallConfirmDialog()
{
    DDialog dlg(QObject::tr("Programs are still running in terminal"), QObject::tr("Are you sure you want to uninstall it?"));
    dlg.setIcon(QIcon::fromTheme("deepin-terminal"));
    dlg.addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dlg.addButton(QString(tr("OK")), true, DDialog::ButtonWarning);
    return (dlg.exec() == DDialog::Accepted);
}

bool Utils::showUnistallConfirmDialog(QString commandname)
{
#ifndef USE_DTK
    OperationConfirmDlg dlg;
    dlg.setFixedSize(380, 160);
    dlg.setOperatTypeName(QObject::tr("Are you sure you want to uninstall this application?"));
    dlg.setTipInfo(QObject::tr("You will not be able to use Terminal any longer."));
    dlg.setOKCancelBtnText(QObject::tr("OK"), QObject::tr("Cancel"));
    dlg.exec();

    return (dlg.getConfirmResult() == QDialog::Accepted);
#else
    /******** Modify by nt001000 renfeixiang 2020-05-27:修改 根据remove和purge卸载命令，显示不同的弹框信息 Begin***************/
    QString title = "", text = "";
    if (commandname == "remove") {
        title = QObject::tr("Are you sure you want to uninstall this application?");
        text = QObject::tr("You will not be able to use Terminal any longer.");
    } else if (commandname == "purge") {
        //后面根据产品提供的信息，修改此处purge命令卸载时的弹框信息
        title = QObject::tr("Are you sure you want to uninstall this application?");
        text = QObject::tr("You will not be able to use Terminal any longer.");
    }
    DDialog dlg(title, text);
    /******** Modify by nt001000 renfeixiang 2020-05-27:修改 根据remove和purge卸载命令，显示不同的弹框信息 Begin***************/
    dlg.setIcon(QIcon::fromTheme("dialog-warning"));
    dlg.addButton(QObject::tr("Cancel"), false, DDialog::ButtonNormal);
    dlg.addButton(QObject::tr("OK"), true, DDialog::ButtonWarning);
    return (dlg.exec() == DDialog::Accepted);
#endif
}

/*******************************************************************************
 1. @函数:    showShortcutConflictDialog
 2. @作者:    n014361 王培利
 3. @日期:    2020-03-31
 4. @说明:    快捷键冲突框显示
*******************************************************************************/
bool Utils::showShortcutConflictDialog(QString conflictkey)
{
    QString str = qApp->translate("DSettingsDialog", "This shortcut conflicts with %1")
                  .arg(QString("<span style=\"color: rgba(255, 90, 90, 1);\">%1</span>").arg(conflictkey));

    OperationConfirmDlg optDlg;
    QPixmap warnning = QIcon::fromTheme("dialog-warning").pixmap(QSize(32, 32));
    optDlg.setIconPixmap(warnning);
    optDlg.setOperatTypeName(str);
    optDlg.setTipInfo(QObject::tr("Click on Add to make this shortcut effective immediately"));
    optDlg.setOKCancelBtnText(QObject::tr("Replace"), QObject::tr("Cancel"));
    optDlg.setFixedSize(380, 160);
    optDlg.exec();
    return optDlg.getConfirmResult() == QDialog::Accepted;
}

bool Utils::showShortcutConflictMsgbox(QString txt)
{
#ifndef USE_DTK
    WarnningDlg dlg;
    dlg.setOperatTypeName(txt);
    dlg.setTipInfo(QObject::tr("please set another one."));
    dlg.exec();
#else
    DDialog dlg;
//    dlg.setTitle(QString(txt ));
//    dlg.setMessage(QObject::tr(" please set another one."));
    dlg.setIcon(QIcon::fromTheme("dialog-warning"));
    dlg.setTitle(QString(txt + QObject::tr("please set another one.")));
    /***mod by ut001121 zhangmeng 20200521 将确认按钮设置为默认按钮 修复BUG26960***/
    dlg.addButton(QString(tr("OK")), true, DDialog::ButtonNormal);
    dlg.exec();
#endif
    return  true;
}

/*******************************************************************************
 1. @函数:     setSpaceInWord
 2. @作者:     m000714 戴正文
 3. @日期:     2020-04-10
 4. @说明:     为按钮两个中文之间添加空格
*******************************************************************************/
void Utils::setSpaceInWord(DPushButton *button)
{
    const QString &text = button->text();

    if (text.count() == 2) {
        for (const QChar &ch : text) {
            switch (ch.script()) {
            case QChar::Script_Han:
            case QChar::Script_Katakana:
            case QChar::Script_Hiragana:
            case QChar::Script_Hangul:
                break;
            default:
                return;
            }
        }

        button->setText(QString().append(text.at(0)).append(QChar::Nbsp).append(text.at(1)));
    }
}

void Utils::showRenameTitleDialog(QString oldTitle, QWidget *parentWidget)
{
    TermInputDialog *pDialog = new TermInputDialog(parentWidget);
    pDialog->setWindowModality(Qt::ApplicationModal);
    pDialog->setFixedSize(380, 180);
    pDialog->setIcon(QIcon::fromTheme("deepin-terminal"));
    pDialog->setFocusPolicy(Qt::NoFocus);
    pDialog->showDialog(oldTitle, parentWidget);
}

/*******************************************************************************
 1. @函数:    showSameNameDialog
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-04-16
 4. @说明:    当有相同名称时，弹出弹窗给用户确认
*******************************************************************************/
void Utils::showSameNameDialog(QWidget *parent, const QString &firstLine, const QString &secondLine)
{
    DDialog *dlg = new DDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setTitle(QString(firstLine + secondLine));
    dlg->setIcon(QIcon::fromTheme("dialog-warning"));
    dlg->addButton(QString(QObject::tr("OK")), true, DDialog::ButtonNormal);
    dlg->show();
    moveToCenter(dlg);
}
/*******************************************************************************
 1. @函数:    clearChildrenFocus
 2. @作者:    n014361 王培利
 3. @日期:    2020-05-08
 4. @说明:    清空控件内部所有子控件的焦点获取
 　　　　　　　安全考虑，不要全局使用．仅在个别控件中使用
*******************************************************************************/
void Utils::clearChildrenFocus(QObject *objParent)
{
    // 可以获取焦点的控件名称列表
    QStringList foucswidgetlist;
    foucswidgetlist << "QLineEdit" << "Konsole::TerminalDisplay";

    //qDebug() << "checkChildrenFocus start" << objParent->children().size();
    for (QObject *obj : objParent->children()) {
        if (!obj->isWidgetType()) {
            continue;
        }
        QWidget *widget = static_cast<QWidget *>(obj);
        if (Qt::NoFocus != widget->focusPolicy()) {
            qDebug() << widget << widget->focusPolicy() << widget->metaObject()->className();
            if (!foucswidgetlist.contains(widget->metaObject()->className())) {
                widget->setFocusPolicy(Qt::NoFocus);
            }
        }
        clearChildrenFocus(obj);
    }

    //qDebug() << "checkChildrenFocus over" << objParent->children().size();
}

TermProperties Utils::parseArgument(QStringList arguments)
{
    QCommandLineParser parser;

    QCommandLineOption optWorkDirectory({ "w", "work-directory" }, QObject::tr("Set the work directory"), "path");
    QCommandLineOption optWindowState({ "m", "window-mode" },
                                      QString(QObject::tr("Set the window mode on starting") + " (normal, maximize, fullscreen, splitscreen)"),
                                      "state-mode");
    QCommandLineOption optExecute({ "e", "execute" }, QObject::tr("Execute a command in the terminal "), "command");
    QCommandLineOption optScript({ "c", "run-script" }, QObject::tr("Run script string in the terminal"), "script");
    //QCommandLineOption optionExecute2({"x", "Execute" }, "Execute command in the terminal", "command");
    QCommandLineOption optQuakeMode({ "q", "quake-mode" }, QObject::tr("Run in quake mode"), "");
    QCommandLineOption optKeepOpen("keep-open", QObject::tr("Keep terminal open when command finishes"), "");
    //parser.addPositionalArgument("e",  "Execute command in the terminal", "command");
    parser.addOptions({ optWorkDirectory, optExecute, /*optionExecute2,*/ optQuakeMode, optWindowState, optKeepOpen, optScript});
    parser.parse(arguments);

    TermProperties firstTermProperties;

    if (parser.isSet(optExecute)) {
        firstTermProperties[Execute] = parser.value(optExecute);
    }
    if (parser.isSet(optWorkDirectory)) {
        firstTermProperties[WorkingDir] = parser.value(optWorkDirectory);
    }
    if (parser.isSet(optWindowState)) {
        firstTermProperties[StartWindowState] = parser.value(optWindowState);
        QStringList validString = {"maximize", "fullscreen", "splitscreen", "normal"};
        if (!validString.contains(parser.value(optWindowState))) {
            parser.showHelp();
            exit(0);
        }
    }
    if (parser.isSet(optKeepOpen)) {
        firstTermProperties[KeepOpen] = "";
    }
    if (parser.isSet(optScript)) {
        firstTermProperties[Script] = parser.value(optScript);;
    }

    if (parser.isSet(optQuakeMode)) {
        firstTermProperties[QuakeMode] = true;
    } else {
        firstTermProperties[QuakeMode] = false;
    }

    firstTermProperties[SingleFlag] = false;
    return firstTermProperties;
}

/*******************************************************************************
 1. @函数:    encodeList
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-05-27
 4. @说明:    获取编码列表
*******************************************************************************/
QList<QByteArray> Utils::encodeList()
{
    QList<QByteArray> all = QTextCodec::availableCodecs();
    QList<QByteArray> showEncodeList;
    // 这是ubuntu18.04支持的编码格式，按国家排列的
//    showEncodeList << "GB18030" << "GB2312" << "GBK"
//                   << "BIG5" << "BIG5-HKSCS" << "EUC-TW"
//                   << "EUC-JP" << "ISO-2022-JP" << "SHIFT_JIS"
//                   << "EUC-KR" << "ISO-2022-KR" << "UHC"
//                   << "IBM864" << "ISO-8859-6" << "MAC_ARABIC" << "WINDOWS-1256"
//                   << "ARMSCII-8"
//                   << "ISO-8859-13" << "ISO-8859-4" << "WINDOWS-1257"
//                   << "ISO-8859-14"
//                   << "IBM-852" << "ISO-8859-2" << "MAC_CE" << "WINDOWS-1250"
//                   << "MAC_CROATIAN"
//                   << "IBM855" << "ISO-8859-5" << "ISO-IR-111" << "ISO-IR-111" << "KOI8-R" << "MAC-CYRILLIC" << "WINDOWS-1251"
//                   << "CP866"
//                   << "KOI8-U" << "MAC_UKRAINIAN"
//                   << "GEORGIAN-PS"
//                   << "ISO-8859-7" << "MAC_GREEK" << "WINDOWS-1253"
//                   << "MAC_GUJARATI"
//                   << "MAC_GURMUKHI"
//                   << "IBM862" << "ISO-8859-8-I" << "MAC_HEBREW" << "WINDOWS-1255"
//                   << "ISO-8859-8"
//                   << "MAC_DEVANAGARI"
//                   << "MAC_ICELANDIC"
//                   << "ISO-8859-10"
//                   << "MAC_FARSI"
//                   << "ISO-8859-16" << "MAC_ROMANIAN"
//                   << "ISO-8859-3"
//                   << "TIS-620"
//                   << "IBM857" << "ISO-8859-9" << "MAC_TURKISH" << "WINDOWS-1254"
//                   << "TCVN" << "VISCII" << "WINDOWS-1258"
//                   << "IBM850" << "ISO-8859-1" << "ISO-8859-15" << "MAC_ROMAN" << "WINDOWS-1252";
    showEncodeList << "UTF-8" << "GB18030" << "GB2312" << "GBK" /*简体中文*/
                   << "BIG5" << "BIG5-HKSCS" //<< "EUC-TW"      /*繁体中文*/
                   << "EUC-JP"  << "SHIFT_JIS"  //<< "ISO-2022-JP"/*日语*/
                   << "EUC-KR" //<< "ISO-2022-KR" //<< "UHC"      /*韩语*/
                   << "IBM864" << "ISO-8859-6" << "ARABIC" << "WINDOWS-1256"   /*阿拉伯语*/
                   //<< "ARMSCII-8"    /*美国语*/
                   << "ISO-8859-13" << "ISO-8859-4" << "WINDOWS-1257"  /*波罗的海各国语*/
                   << "ISO-8859-14"    /*凯尔特语*/
                   << "IBM-852" << "ISO-8859-2" << "x-mac-CE" << "WINDOWS-1250" /*中欧*/
                   //<< "x-mac-CROATIAN"  /*克罗地亚*/
                   << "IBM855" << "ISO-8859-5"  << "KOI8-R" << "MAC-CYRILLIC" << "WINDOWS-1251" //<< "ISO-IR-111" /*西里尔语*/
                   << "CP866" /*西里尔语或俄语*/
                   << "KOI8-U" << "x-MacUkraine" /*西里尔语或乌克兰语*/
                   //<< "GEORGIAN-PS"
                   << "ISO-8859-7" << "x-mac-GREEK" << "WINDOWS-1253"  /*希腊语*/
                   //<< "x-mac-GUJARATI"
                   //<< "x-mac-GURMUKHI"
                   << "IBM862" << "ISO-8859-8-I" << "WINDOWS-1255"//<< "x-mac-HEBREW"  /*希伯来语*/
                   << "ISO-8859-8" /*希伯来语*/
                   //<< "x-mac-DEVANAGARI"
                   //<< "x-mac-ICELANDIC" /*冰岛语*/
                   << "ISO-8859-10"     /*北欧语*/
                   //<< "x-mac-FARSI"     /*波斯语*/
                   //<< "x-mac-ROMANIAN" //<< "ISO-8859-16" /*罗马尼亚语*/
                   << "ISO-8859-3"      /*西欧语*/
                   << "TIS-620"         /*泰语*/
                   << "IBM857" << "ISO-8859-9" << "x-mac-TURKISH" << "WINDOWS-1254" /*土耳其语*/
                   << "WINDOWS-1258" //<< "TCVN" << "VISCII"  /*越南语*/
                   << "IBM850" << "ISO-8859-1" << "ISO-8859-15" << "x-ROMAN8" << "WINDOWS-1252"; /*西方国家*/


    // meld提供的编码格式,按名称排列的
//    showEncodeList<<"UTF-8"
//                 <<"ISO-8859-1"
//                 <<"ISO-8859-2"
//                 <<"ISO-8859-3"
//                 <<"ISO-8859-4"
//                 <<"ISO-8859-5"
//                 <<"ISO-8859-6"
//                 <<"ISO-8859-7"
//                 <<"ISO-8859-8"
//                 <<"ISO-8859-9"
//                 <<"ISO-8859-10"
//                 <<"ISO-8859-13"
//                 <<"ISO-8859-14"
//                 <<"ISO-8859-15"
//                 //<<"ISO-8859-16"
//                 <<"UTF-7"
//                 <<"UTF-16"
//                 <<"UTF-16BE"
//                 <<"UTF-16LE"
//                 <<"UTF-32"
//                 <<"UCS-2"
//                 <<"UCS-4"
//                 //<<"ARMSCII-8"
//                 <<"BIG5"
//                 <<"BIG5-HKSCS"
//                 <<"CP866"
//                 <<"EUC-JP"
//                 //<<"EUC-JP-MS"
//                 <<"CP932"
//                 <<"EUC-KR"
//                 //<<"EUC-TW"
//                 <<"GB18030"
//                 <<"GB2312"
//                 <<"GBK"
//                 //<<"GEORGIAN-ACADEMY"
//                 <<"IBM850"
//                 <<"IBM852"
//                 <<"IBM855"
//                 <<"IBM857"
//                 <<"IBM862"
//                 <<"IBM864"
//                 <<"ISO-2022-JP"
//                 <<"ISO-2022-KR"
//                 //<<"ISO-IR-111"
//                 //<<"JOHAB"
//                 <<"KOI8-R"
//                 <<"KOI8-U"
//                 <<"SHIFT_JIS"
//                 //<<"TCVN"
//                 <<"TIS-620"
//                 //<<"UHC"
//                 //<<"VISCII"
//                 <<"WINDOWS-1250"
//                 <<"WINDOWS-1251"
//                 <<"WINDOWS-1252"
//                 <<"WINDOWS-1253"
//                 <<"WINDOWS-1254"
//                 <<"WINDOWS-1255"
//                 <<"WINDOWS-1256"
//                 <<"WINDOWS-1257"
//                 <<"WINDOWS-1258";


    QList<QByteArray> encodeList;
    // 自定义的名称，系统里不一定大小写完全一样，再同步一下。
    for (QByteArray name : showEncodeList) {
        QString strname1 = name;
        bool bFind = false;
        QByteArray encodename;
        for (QByteArray name2 : all) {
            QString strname2 = name2;
            if (strname1.compare(strname2, Qt::CaseInsensitive) == 0) {
                bFind = true;
                encodename = name2;
                break;
            }
        }
        if (!bFind) {
            qDebug() << "encode name :" << name << "not find!";
        } else {
            encodeList << encodename;
        }
    }
    // 返回需要的值
    return encodeList;
}

QCommandLineParser &Utils::setCommandLineParser(QString appDesc, DApplication &app, QCommandLineParser &parser)
{
    parser.setApplicationDescription(appDesc);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
    QCommandLineOption optWorkDirectory({ "w", "work-directory" }, QObject::tr("Set the work directory"), "path");
    QCommandLineOption optWindowState({ "m", "window-mode" },
                                      QString(QObject::tr("Set the window mode on starting") + " (normal, maximize, fullscreen, splitscreen)"),
                                      "state-mode");
    QCommandLineOption optExecute({ "e", "execute" }, QObject::tr("Execute a command in the terminal "), "command");
    QCommandLineOption optScript({ "c", "run-script" }, QObject::tr("Run script string in the terminal"), "script");
    //QCommandLineOption optionExecute2({"x", "Execute" }, "Execute command in the terminal", "command");
    QCommandLineOption optQuakeMode({ "q", "quake-mode" }, QObject::tr("Run in quake mode"), "");
    QCommandLineOption optKeepOpen("keep-open", QObject::tr("Keep terminal open when command finishes"), "");
    //parser.addPositionalArgument("e",  "Execute command in the terminal", "command");

    parser.addOptions({ optWorkDirectory, optExecute, /*optionExecute2,*/ optQuakeMode, optWindowState, optKeepOpen, optScript});
    parser.parse(app.arguments());

    qDebug() << "optionWorkDirectory" << parser.value(optWorkDirectory);
    qDebug() << "optionExecute" << parser.value(optExecute);
//    qDebug() << "optionExecute2"<<m_parser.value(optionExecute2);
    qDebug() << "optionQuakeMode" << parser.isSet(optQuakeMode);
    qDebug() << "optionWindowState" << parser.isSet(optWindowState);
    qDebug() << "positionalArguments" << parser.positionalArguments();

    return parser;
}

