#include "MainWindow.hpp"
#include "OutputWidget.hpp"
#include "config.hpp"
#include <algorithm>

MainWindow::MainWindow()
    : menu_recent(new RecentFilesMenu(this))
{
    // Actions
    auto action_open = new QAction(QIcon(":/icons/document-open.png"), "&Open...", this);
    action_open->setShortcuts(QKeySequence::Open);
    action_open->setMenuRole(QAction::NoRole);
    QObject::connect(action_open, &QAction::triggered, this, &MainWindow::open);

    auto action_save_as = new QAction(QIcon(":/icons/document-save-as.png"), "Save &As...", this);
    action_save_as->setShortcuts(QKeySequence::SaveAs);
    action_save_as->setMenuRole(QAction::NoRole);
    QObject::connect(action_save_as, &QAction::triggered, this, &MainWindow::saveAs);

    auto action_quit = new QAction(QIcon(":/icons/application-exit.png"), "&Quit", this);
    QObject::connect(action_quit, &QAction::triggered, this, &QWidget::close);
    action_quit->setShortcuts(QKeySequence::Quit);
    action_quit->setMenuRole(QAction::QuitRole);

    auto action_help = new QAction("&User Manual", this);
    QObject::connect(action_help, &QAction::triggered, this, &MainWindow::help);
    action_help->setShortcut(Qt::Key_F1);

    auto action_about = new QAction(QIcon(":/icons/dialog-information.png"), "&About...", this);
    QObject::connect(action_about, &QAction::triggered, this, &MainWindow::about);
    action_about->setMenuRole(QAction::AboutRole);
    action_about->setIconVisibleInMenu(true);

    // Recent file menu
    QObject::connect(menu_recent, &RecentFilesMenu::openRecent, this, &MainWindow::loadFile);

    // File menu
    auto menu_file = this->menuBar()->addMenu("&File");
    menu_file->addAction(action_open);
    menu_file->addMenu(menu_recent);
    menu_file->addSeparator();
    menu_file->addAction(action_save_as);
    menu_file->addSeparator();
    menu_file->addAction(action_quit);

    // Help menu
    auto menu_help = this->menuBar()->addMenu("&Help");
    menu_help->addAction(action_help);
    menu_help->addAction(action_about);

    // Main window
    this->setWindowIcon(QIcon(":/icons/logo"));

    // Load geometry and state
    QSettings settings;
    restoreState(settings.value("OutputWindow/state").toByteArray());
    restoreGeometry(settings.value("OutputWindow/geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save state and geometry
    QSettings settings;
    settings.setValue("OutputWindow/state", saveState());
    settings.setValue("OutputWindow/geometry", saveGeometry());
}

void MainWindow::loadFile(const QString& path)
{
    try
    {
        OutputData output(path.toStdString());
        this->setCentralWidget(new OutputWidget(output));
        this->setWindowFilePath(path);
        menu_recent->addPath(path);
    }
    catch(const std::exception& e)  // Todo
    {
        QMessageBox::critical(this, "Error", "Failed to open " + path + "\n" + e.what());
    }
}

void MainWindow::saveFile(const QString &path)
{
    try
    {
        auto widget = static_cast<OutputWidget*>(this->centralWidget());
        widget->getData().save(path.toStdString());
        this->setWindowFilePath(path);
    }
    catch(const std::exception& e)  // Todo
    {
        QMessageBox::critical(this, "Error", "Failed to save " + path + "\n" + e.what());
    }
}

void MainWindow::open()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilter("Result Files (*.res)");

    if(dialog.exec() == QDialog::Accepted)
        loadFile(dialog.selectedFiles().first());
}

void MainWindow::saveAs()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter("Result Files (*.red)");
    dialog.setDefaultSuffix("res");
    dialog.selectFile(this->windowFilePath().isEmpty() ? DEFAULT_FILENAME : this->windowFilePath());

    if(dialog.exec() == QDialog::Accepted)
        saveFile(dialog.selectedFiles().first());
}

void MainWindow::help()
{
    QList<QString> paths = {
        QCoreApplication::applicationDirPath() + "/manual.pdf",                // Windows
        QCoreApplication::applicationDirPath() + "/../Resources/manual.pdf",   // MacOS
        "/usr/share/virtualbow/manual.pdf"                                     // Linux
    };

    for(QString path: paths) {
        if(QFileInfo(path).exists()) {
            if(!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
                                QMessageBox::critical(this, "Error", "Failed to open file " + path);
                        }
            return;
        }
    }

    QMessageBox::critical(this, "Error", "Failed to open user manual, file not found.");
}

void MainWindow::about()
{
    QMessageBox::about(this, "About", QString()
        + "<strong><font size=\"6\">" + Config::APPLICATION_DISPLAY_NAME_POST + "</font></strong><br>"
        + "Version " + Config::APPLICATION_VERSION + "<br><br>"
        + Config::APPLICATION_DESCRIPTION + "<br>"
        + "<a href=\"" + Config::ORGANIZATION_DOMAIN + "\">" + Config::ORGANIZATION_DOMAIN + "</a><br><br>"
        + "<small>" + Config::APPLICATION_COPYRIGHT + "<br>"
        + "Distributed under the " + Config::APPLICATION_LICENSE + "</small>"
    );
}
