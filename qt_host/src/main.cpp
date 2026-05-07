#include "main_window.h"

#include <QApplication>

// Qt 上位机入口。
// QApplication 管理事件循环、控件样式和平台消息；MainWindow 承载实际业务界面。
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
