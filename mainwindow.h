#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_cbx_FontChooser_currentIndexChanged(const QString &arg1);

	void on_pb_ResetText_clicked();

	void on_pb_Reload_clicked();

private:
	bool eventFilter(QObject *obj, QEvent *event);

	Ui::MainWindow *ui;

	QString mCurrentFontDir;
	QString mDefaultText;
	QMap<int, QStringList> mFontFamilies;
};
#endif // MAINWINDOW_H
