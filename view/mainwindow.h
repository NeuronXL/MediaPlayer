#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DebugWidget;
class MainWindowViewModel;
class QPropertyAnimation;

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
    DebugWidget* m_debugWidget;
    QPropertyAnimation* m_debugPanelAnimation;
    MainWindowViewModel* m_viewModel;
    int m_debugPanelExpandedHeight;
    int m_windowBaseHeight;
};

#endif // MAINWINDOW_H
