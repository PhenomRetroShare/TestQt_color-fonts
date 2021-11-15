#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHelpEvent>
#include <QScrollBar>
#include <QToolTip>

//sizeof(c) or sizeof("😮") don't get the good value.
constexpr int UCHARSIZE = 4;

/**
 * @brief Compare unicode Char
 * @param a
 * @param b
 * @return -1 if a<b; 0 if a==b; 1 if a>b
 */
int unicodechar_Compare(const char *a, const char *b)
{
	for(size_t c = 0; c < UCHARSIZE-1; ++c)
		if (a[c] != b[c])
			return a[c] > b[c] ? 1 : -1;
	return 0;
}

/**
 * @brief Increment unicode Char
 * @param [in,out] unicode Char
 */
void unicodechar_Inc(char *a)
{
	if(a[UCHARSIZE-1] == char(0xFF))
	{
		a[UCHARSIZE-1] = 0;
		for(auto c = UCHARSIZE-2; c >= 0; --c)
			if (a[c] == char(0xFF))
				a[c] = 0;
			else
			{
				++a[c];
				break;
			}
	}
	else
		a[UCHARSIZE-1] += 1;
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , mCurrentFontDir(QString(SOURCE_PATH) + "/color-fonts/fonts")
{
	ui->setupUi(this);

	ui->te_Text->installEventFilter(this);

	mDefaultText = "";
	for(auto c = ' '; c <= '~'; ++c)
		mDefaultText += c;
	mDefaultText += "\n";
	//for(char *c = strdup("¡"); unicodechar_Compare(c, "ÿ") <= 0; unicodechar_Inc(c))
	//	mDefaultText += c;
	//mDefaultText += "\n";
	///for(char *c = strdup("ƀ"); unicodechar_Compare(c, "ɏ") <= 0; unicodechar_Inc(c))
	//	mDefaultText += c;
	//mDefaultText += "\n";
	//for(char *c = strdup("ƀ"); unicodechar_Compare(c, "ɏ") <= 0; unicodechar_Inc(c))
	//	mDefaultText += c;
	//mDefaultText += "\n";
	for(char *c = strdup("😀"); unicodechar_Compare(c, "😮") <= 0; unicodechar_Inc(c))
		mDefaultText += c;

	on_pb_Reload_clicked();
	on_pb_ResetText_clicked();
}

MainWindow::~MainWindow()
{
	delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if ((obj == ui->te_Text) && (event->type() == QEvent::ToolTip))
	{
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
		QPointF pos = helpEvent->pos() + QPointF( ui->te_Text->horizontalScrollBar()->value()
		                                        , ui->te_Text->verticalScrollBar()->value() );
		int charPos = qMin( ui->te_Text->document()->documentLayout()->hitTest(pos, Qt::ExactHit )
		                  , ui->te_Text->toPlainText().size());;
		if (charPos >= 0)
		{
			QString toolTipText = "U+" + QString::number( ui->te_Text->toPlainText().at(charPos).unicode(), 0x10);
			QToolTip::showText(helpEvent->globalPos(), toolTipText);
			return true;
		}
		else
		{
			QToolTip::hideText();
			return true;
		}
	}
	// Pass the event to the parent class
	return QWidget::eventFilter(obj, event);
}

void MainWindow::on_cbx_FontChooser_currentIndexChanged(const QString &fontFileName)
{
	if (fontFileName.isEmpty()) return;

	on_pb_ResetText_clicked();
}


void MainWindow::on_pb_ResetText_clicked()
{
	if (ui->cbx_FontChooser->currentText().isEmpty()) return;
	ui->te_Text->clear();
	QTextCharFormat tcf;
	tcf.setFontFamily(mFontFamilies[ui->cbx_FontChooser->currentData().toInt()][0]);
	tcf.setFontPointSize(20.0);
	ui->te_Text->mergeCurrentCharFormat(tcf);
	ui->te_Text->textCursor().insertText(mDefaultText);
}


void MainWindow::on_pb_Reload_clicked()
{
	for(auto id = mFontFamilies.keyBegin(); id != mFontFamilies.keyEnd(); ++id)
		QFontDatabase::removeApplicationFont(*id);

	ui->cbx_FontChooser->clear();
	mFontFamilies.clear();

	QDir dir = QDir(QFileDialog::getExistingDirectory(this, "Choose your folder with fonts to test.", mCurrentFontDir));
	if (!dir.exists()) return;

	mCurrentFontDir = dir.path();
	for(auto& fontFile : dir.entryInfoList())
	{
		int id = QFontDatabase::addApplicationFont(fontFile.absoluteFilePath());
		if( id == -1)
			qDebug().nospace() << "Can't add font named: " << qPrintable(fontFile.absoluteFilePath());
		else
		{
			mFontFamilies[id] = QFontDatabase::applicationFontFamilies(id);
			qDebug().nospace() << "Added: " << qPrintable(fontFile.absoluteFilePath())
			                   << " with id: " << id
			                   << " with families:"  << qPrintable(mFontFamilies[id].join(";") );

			ui->cbx_FontChooser->addItem(fontFile.fileName(), QVariant(id));
		}
	}
}

