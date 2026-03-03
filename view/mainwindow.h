#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class LogWidget;
class MainWindowViewModel;
class QPropertyAnimation;
class VideoWidget;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

  private slots:
    void openFileDialog();
    void toggleDebugPanel();
    void updateSelectedFilePath(const QString& filePath);

  private:
    void animateDebugPanel(bool expand);

    Ui::MainWindow* ui;
    LogWidget* m_logWidget;
    QPropertyAnimation* m_debugPanelAnimation;
    VideoWidget* m_videoWidget;
    MainWindowViewModel* m_viewModel;
    int m_debugPanelExpandedHeight;
    int m_windowBaseHeight;
};

#endif // MAINWINDOW_H
