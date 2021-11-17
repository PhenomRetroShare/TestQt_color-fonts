#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHelpEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>
#include <QToolTip>

constexpr int MAX_CHAR_BY_BLOCK = 1024;

/* Useful for set-operations on small enums.
 * For example, for testing "x ∈ {x1, x2, x3}" use:
 * (FLAG_UNSAFE(x) & (FLAG(x1) | FLAG(x2) | FLAG(x3)))
 */
#define FLAG(x) (1 << (x))

const QString getCatNames(uint ucs4)
{
	QChar::Category cat = QChar::category(ucs4);
	QString catNames;
	if( FLAG(cat) & FLAG(QChar::Mark_NonSpacing         )) catNames += ", Mark_NonSpacing (Mn)"          ;
	if( FLAG(cat) & FLAG(QChar::Mark_SpacingCombining   )) catNames += ", Mark_SpacingCombining (Mc)"    ;
	if( FLAG(cat) & FLAG(QChar::Mark_Enclosing          )) catNames += ", Mark_Enclosing (Me)"           ;
	if( FLAG(cat) & FLAG(QChar::Number_DecimalDigit     )) catNames += ", Number_DecimalDigit (Nd)"      ;
	if( FLAG(cat) & FLAG(QChar::Number_Letter           )) catNames += ", Number_Letter (Nl)"            ;
	if( FLAG(cat) & FLAG(QChar::Number_Other            )) catNames += ", Number_Other (No)"             ;
	if( FLAG(cat) & FLAG(QChar::Separator_Space         )) catNames += ", Separator_Space (Zs)"          ;
	if( FLAG(cat) & FLAG(QChar::Separator_Line          )) catNames += ", Separator_Line (Zl)"           ;
	if( FLAG(cat) & FLAG(QChar::Separator_Paragraph     )) catNames += ", Separator_Paragraph (Zp)"      ;
	if( FLAG(cat) & FLAG(QChar::Other_Control           )) catNames += ", Other_Control (Cc)"            ;
	if( FLAG(cat) & FLAG(QChar::Other_Format            )) catNames += ", Other_Format (Cf)"             ;
	if( FLAG(cat) & FLAG(QChar::Other_Surrogate         )) catNames += ", Other_Surrogate (Cs)"          ;
	if( FLAG(cat) & FLAG(QChar::Other_PrivateUse        )) catNames += ", Other_PrivateUse (Co)"         ;
	if( FLAG(cat) & FLAG(QChar::Other_NotAssigned       )) catNames += ", Other_NotAssigned (Cn)"        ;
	if( FLAG(cat) & FLAG(QChar::Letter_Uppercase        )) catNames += ", Letter_Uppercase (Lu)"         ;
	if( FLAG(cat) & FLAG(QChar::Letter_Lowercase        )) catNames += ", Letter_Lowercase (Ll)"         ;
	if( FLAG(cat) & FLAG(QChar::Letter_Titlecase        )) catNames += ", Letter_Titlecase (Lt)"         ;
	if( FLAG(cat) & FLAG(QChar::Letter_Modifier         )) catNames += ", Letter_Modifier (Lm)"          ;
	if( FLAG(cat) & FLAG(QChar::Letter_Other            )) catNames += ", Letter_Other (Lo)"             ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_Connector   )) catNames += ", Punctuation_Connector (Pc)"    ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_Dash        )) catNames += ", Punctuation_Dash (Pd)"         ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_Open        )) catNames += ", Punctuation_Open (Ps)"         ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_Close       )) catNames += ", Punctuation_Close (Pe)"        ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_InitialQuote)) catNames += ", Punctuation_InitialQuote (Pi)" ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_FinalQuote  )) catNames += ", Punctuation_FinalQuote (Pf)"   ;
	if( FLAG(cat) & FLAG(QChar::Punctuation_Other       )) catNames += ", Punctuation_Other (Po)"        ;
	if( FLAG(cat) & FLAG(QChar::Symbol_Math             )) catNames += ", Symbol_Math (Sm)"              ;
	if( FLAG(cat) & FLAG(QChar::Symbol_Currency         )) catNames += ", Symbol_Currency (Sc)"          ;
	if( FLAG(cat) & FLAG(QChar::Symbol_Modifier         )) catNames += ", Symbol_Modifier (Sk)"          ;
	if( FLAG(cat) & FLAG(QChar::Symbol_Other            )) catNames += ", Symbol_Other (So)"             ;
	if (!catNames.isEmpty()) catNames.remove(0,                QString(", ").length());
	return catNames;
};

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , mCurrentFontDir(QString(SOURCE_PATH) + "/color-fonts/fonts")
{
	ui->setupUi(this);

	ui->te_Text->installEventFilter(this);
	ui->te_Text->setWordWrapMode(QTextOption::WrapAnywhere);
	// Fix Palette for Win when selected text without focus is very fade.
	QPalette pal(ui->te_Text->palette());
	pal.setColor(QPalette::Disabled, QPalette::Highlight,
	             pal.color( QPalette::Highlight));

	loadConfig();

	if (!loadUCB()) ui->lbl_FontChooser->setText("Couldn't load UCB File.");

	on_pb_Reload_clicked();
	on_le_CharCode_editingFinished();
}

MainWindow::~MainWindow()
{
	saveConfig();
	delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if ((obj == ui->te_Text) && (event->type() == QEvent::ToolTip))
	{
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
		QPointF pos = helpEvent->pos() + QPointF( ui->te_Text->horizontalScrollBar()->value()
		                                        , ui->te_Text->verticalScrollBar()->value() );
		QString s = ui->te_Text->toPlainText();
		int charPos = qMin( ui->te_Text->document()->documentLayout()->hitTest(pos, Qt::ExactHit )
		                  , s.size()-1);;
		if (charPos >= 0)
		{

			uint uc = s.at(charPos).unicode();
			if (QChar(uc).isHighSurrogate() && charPos < s.size()-1) {
				ushort low = s.at(charPos+1).unicode();
				if (QChar(low).isLowSurrogate()) {
					uc = QChar::surrogateToUcs4(uc, low);
				}
			}
			QString toolTipText = "<html><body><p>U+" + QString::number( uc, 0x10) + "</p>"
			                    + "<p>Printed by your system font at size 40pt:"
			                       + "<span style=\" font-size:40pt;\">" + QString::fromUcs4( &uc, 1) + "</span></p>"
			                    + "<p>Printed using \"" + mFontFamilies[ui->cbx_FontChooser->currentData().toInt()] + "\" Font at size 40pt: "
			                       + "<span style=\" font-family:'"
			                         + mFontFamilies[ui->cbx_FontChooser->currentData().toInt()] + "';"
			                                     + " font-size:40pt;\">" + QString::fromUcs4( &uc, 1) + "</span></p>"
			                    + "<p>Categories:" + getCatNames(uc) + "</p></body></html>";

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

bool MainWindow::loadConfig()
{
	QFile confFile(qApp->applicationDirPath() + "/config.json");

	if (!confFile.open(QIODevice::ReadOnly))
	{
		qWarning("Couldn't open Config file.");
		return false;
	}

	QByteArray confData = confFile.readAll();

	QJsonParseError error;
	QJsonDocument loadDoc( QJsonDocument::fromJson(confData, &error));
	if (error.error != QJsonParseError::NoError)
	{
		qCritical() << "Bad formed Config file."
		            << qPrintable(error.errorString());
		return false;
	}

	QJsonObject json(loadDoc.object());

	if (json.contains("mCurrentFontDir") && json["mCurrentFontDir"].isString())
		mCurrentFontDir = json["mCurrentFontDir"].toString();
	if (json.contains("mCurrentFontFile") && json["mCurrentFontFile"].isString())
		mCurrentFontFile = json["mCurrentFontFile"].toString();
	if (json.contains("le_CharCode") && json["le_CharCode"].isString())
		whileBlocking(ui->le_CharCode)->setText(json["le_CharCode"].toString());

	return true;
}

bool MainWindow::saveConfig()
{

	QFile confFile(qApp->applicationDirPath() + "/config.json");

	if (!confFile.open(QIODevice::WriteOnly))
	{
		qWarning("Couldn't open Config file.");
		return false;
	}

	QJsonObject json;
	json["mCurrentFontDir"] = mCurrentFontDir;
	json["mCurrentFontFile"] = mCurrentFontFile;
	json["le_CharCode"] = ui->le_CharCode->text();

	confFile.write(QJsonDocument(json).toJson());

	return true;

}

bool MainWindow::loadUCB()
{
	QFile utcFile(":/UCB");

	if (!utcFile.open(QIODevice::ReadOnly))
	{
		qWarning("Couldn't open Unicode Characters Block file.");
		return false;
	}

	QByteArray utcData = utcFile.readAll();

	QJsonParseError error;
	QJsonDocument loadDoc( QJsonDocument::fromJson(utcData, &error));
	if (error.error != QJsonParseError::NoError)
	{
		qCritical() << "Bad formed UTB file."
		           << qPrintable(error.errorString());
		return false;
	}

	QJsonObject json(loadDoc.object());
	QVector<UC_Block> vUC_Blocks;

	if (json.contains("blocks") && json["blocks"].isArray())
	{
		QJsonArray ja_Blocks = json["blocks"].toArray();

		vUC_Blocks.clear();
		vUC_Blocks.reserve(ja_Blocks.size());
		for (int blockIndex = 0; blockIndex < ja_Blocks.size(); ++blockIndex)
		{
			QJsonObject jo_Curs = ja_Blocks[blockIndex].toObject();
			UC_Block block;
			bool ok = true;
			if (ok && (jo_Curs.contains("beginCharCode") && jo_Curs["beginCharCode"].isString()))
				block.beginCharCode = jo_Curs["beginCharCode"].toString().toInt(&ok, 16);

			if (ok && (jo_Curs.contains("endCharCode") && jo_Curs["endCharCode"].isString()))
				block.endCharCode = jo_Curs["endCharCode"].toString().toInt(&ok, 16);

			if (ok && (jo_Curs.contains("name") && jo_Curs["name"].isString()))
				block.name = jo_Curs["name"].toString();

			if (ok)
				vUC_Blocks.append(block);
			else
			{
				qCritical() << "Error in lines of UTB file for block index:"
				            << blockIndex;
				return false;
			}
		}
	}
	for (auto &block : vUC_Blocks)
	{
		int size = block.endCharCode - block.beginCharCode;
		int split = size / MAX_CHAR_BY_BLOCK;
		for (int curs = 0; curs <= split; ++curs)
		{
			UC_Block splittedBlock;
			splittedBlock.beginCharCode = block.beginCharCode + curs * MAX_CHAR_BY_BLOCK;
			splittedBlock.endCharCode = qMin( block.beginCharCode + (curs+1) * MAX_CHAR_BY_BLOCK -1
			                                , block.endCharCode);
			splittedBlock.name = block.name + ( (split > 0)
			                                  ? QString(" %1/%2").arg(curs+1).arg(split+1)
			                                  : QString() )
			                                + "[U+" + QString::number(splittedBlock.beginCharCode, 0x10)
			                                + " - U+" + QString::number(splittedBlock.endCharCode, 0x10)
			                                + "]";
			ui->cbx_BlockChooser->addItem( splittedBlock.name
			                             , QVariant::fromValue(splittedBlock));
		}
	}
	return true;
}

void MainWindow::on_pb_Reload_clicked()
{
	for(auto id = mFontFamilies.keyBegin(); id != mFontFamilies.keyEnd(); ++id)
		QFontDatabase::removeApplicationFont(*id);

	ui->cbx_FontChooser->clear();
	mFontFamilies.clear();

	QDir dir = QDir(QFileDialog::getExistingDirectory(this, "Choose your directory with fonts to test.", mCurrentFontDir));
	if (!dir.exists()) return;

	mCurrentFontDir = dir.path();
	for(auto& fontFile : dir.entryInfoList())
		if( (fontFile.suffix() == "otf") || (fontFile.suffix() == "ttf") )
		{
			int id = QFontDatabase::addApplicationFont(fontFile.absoluteFilePath());
			if( id == -1)
				qDebug().nospace() << "Can't add font named: " << qPrintable(fontFile.absoluteFilePath());
			else
			{
				QStringList families = QFontDatabase::applicationFontFamilies(id);
				bool added = false;
				for(auto &family : families)
				{
					if(std::find(mFontFamilies.cbegin(),mFontFamilies.cend(),family) == mFontFamilies.cend())
					{
						mFontFamilies[id] = family;
						added = true;
					}
				}
				if (added)
				{
					qDebug().nospace() << "Added: " << qPrintable(fontFile.absoluteFilePath())
					                   << " with id: " << id
					                   << " with families:"  << qPrintable(families.join(";") );

					whileBlocking(ui->cbx_FontChooser)->addItem(fontFile.fileName(), QVariant(id));
				} else {
					QFontDatabase::removeApplicationFont(id);
					qDebug().nospace() << "Can't add font named: " << qPrintable(fontFile.absoluteFilePath())
					                   << " because all its family is already stored: " << qPrintable(families.join(";") );
				}
			}
		}
	ui->statusBar->showMessage(QString("%1 Fonts loaded").arg(ui->cbx_FontChooser->count()),10000);
	ui->cbx_FontChooser->setCurrentText(mCurrentFontFile);
}

void MainWindow::on_cbx_FontChooser_currentIndexChanged(const QString &fontFileName)
{
	if (fontFileName.isEmpty()) return;
	mCurrentFontFile = fontFileName;

	on_cbx_BlockChooser_currentIndexChanged( ui->cbx_BlockChooser->currentIndex() );
}

void MainWindow::on_cbx_BlockChooser_currentIndexChanged(int /*index*/)
{
	if (ui->cbx_FontChooser->currentText().isEmpty()) return;

	ui->te_Text->clear();
	QTextCharFormat tcf;
	tcf.setFontFamily(mFontFamilies[ui->cbx_FontChooser->currentData().toInt()]);
	tcf.setFontPointSize(20.0);
	ui->te_Text->mergeCurrentCharFormat(tcf);

	UC_Block block = ui->cbx_BlockChooser->currentData().value<UC_Block>();
	QString strText;

	for (uint cursChar = block.beginCharCode; cursChar <= block.endCharCode; ++cursChar)
		if (QChar::isPrint(cursChar))
		{
			//	strText += QString::fromUcs4(&cursChar,1);
			if (QChar::requiresSurrogates(cursChar))
				strText += QString(QChar::highSurrogate(cursChar)) + QString(QChar::lowSurrogate(cursChar));
			else
				strText += QChar(cursChar);
		}

	ui->te_Text->textCursor().insertText(strText);

	if(!ui->le_CharCode->text().isEmpty())
	{
		bool ok = true;
		uint uc = ui->le_CharCode->text().toUInt(&ok,0x10);
		if (ok )
		{
			QTextCursor tc(ui->te_Text->textCursor());
			bool reqSur = QChar::requiresSurrogates(uc);
			int pos = ui->te_Text->toPlainText().indexOf( reqSur
			                                            ? /*QChar::highSurrogate(charCode) +*/ QChar::lowSurrogate(uc)
			                                            : QChar(uc)
			                                              );
			if (pos >= 0)
			{
				tc.setPosition(reqSur ? pos - 1 : pos);
				tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
				ui->te_Text->setTextCursor(tc);
			}
			if(!QChar::isPrint(uc))
				ui->statusBar->showMessage(QString("This char U+%1 is not printable. Categories: %2")
				                           .arg(QString::number( uc, 0x10), getCatNames(uc))
				                          , 10000);
		}
	}
}

void MainWindow::on_le_CharCode_editingFinished()
{
	bool ok = true;
	uint charCode = ui->le_CharCode->text().toUInt(&ok, 16);
	if (ok)
	{
		for(int curs = 0; curs < ui->cbx_BlockChooser->count(); ++curs)
		{
			UC_Block block = ui->cbx_BlockChooser->itemData(curs).value<UC_Block>();
			if(  charCode >= block.beginCharCode
			  && charCode <= block.endCharCode )
			{
				whileBlocking(ui->cbx_BlockChooser)->setCurrentIndex(curs);
				break;
			}
		}
	} else {
		ui->le_CharCode->setText("");
	}
	on_cbx_BlockChooser_currentIndexChanged( ui->cbx_BlockChooser->currentIndex() );
}
