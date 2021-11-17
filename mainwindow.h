#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QMetaType>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


struct UC_Block
{
	uint beginCharCode;
	uint endCharCode;
	QString name;
};
Q_DECLARE_METATYPE(UC_Block);

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_pb_Reload_clicked();

	void on_cbx_FontChooser_currentIndexChanged(const QString &arg1);

	void on_cbx_BlockChooser_currentIndexChanged(int index);

	void on_le_CharCode_editingFinished();

private:
	bool eventFilter(QObject *obj, QEvent *event);
	bool loadConfig();
	bool saveConfig();
	bool loadUCB();

	Ui::MainWindow *ui;

	QString mCurrentFontDir;
	QString mCurrentFontFile;
	QMap<int, QString> mFontFamilies;
};

template<class T> class SignalsBlocker
{
public:
	SignalsBlocker(T *blocked) : blocked(blocked), previous(blocked->blockSignals(true)) {}
	~SignalsBlocker() { blocked->blockSignals(previous); }

	T *operator->() { return blocked; }

private:
	T *blocked;
	bool previous;
};

template<class T> inline SignalsBlocker<T> whileBlocking(T *blocked) { return SignalsBlocker<T>(blocked); }

#endif // MAINWINDOW_H
