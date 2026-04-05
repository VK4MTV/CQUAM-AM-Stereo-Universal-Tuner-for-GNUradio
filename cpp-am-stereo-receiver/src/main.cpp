// ─────────────────────────────────────────────────────────────────────────────
// main.cpp  –  AM Stereo Receiver entry point
// ─────────────────────────────────────────────────────────────────────────────
#include <QApplication>
#include <QStyleFactory>
#include <iostream>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AM Stereo Receiver");
    app.setOrganizationName("TropicalRadioNetwork");
    app.setApplicationVersion("1.0.0");

    // Apply a dark-ish style on platforms that support it
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(45,  45,  48));
    dark.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    dark.setColor(QPalette::Base,            QColor(30,  30,  30));
    dark.setColor(QPalette::AlternateBase,   QColor(60,  60,  60));
    dark.setColor(QPalette::ToolTipBase,     QColor(50,  50,  50));
    dark.setColor(QPalette::ToolTipText,     QColor(200, 200, 200));
    dark.setColor(QPalette::Text,            QColor(210, 210, 210));
    dark.setColor(QPalette::Button,          QColor(60,  63,  65));
    dark.setColor(QPalette::ButtonText,      QColor(210, 210, 210));
    dark.setColor(QPalette::Highlight,       QColor(0,   122, 204));
    dark.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(dark);

    std::cout << "AM Stereo Receiver v1.0.0 – starting...\n";

    MainWindow win;
    win.show();

    return app.exec();
}
