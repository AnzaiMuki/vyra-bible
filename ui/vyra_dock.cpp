/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/vyra_dock.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "src/bible/bible_module.hpp"
#include "src/search/book_catalog.hpp"
#include "src/stage/stage_controller.hpp"
#include "ui/stage_panel.hpp"

namespace vyra::ui {

namespace {

// Only our own widgets are styled (by object name). Colors of the rest follow the OBS theme.
constexpr const char *kStyleSheet = R"(
#stageTitle {
	font-weight: 600;
	letter-spacing: 1px;
}
#stagePreviewScreen, #stageProgramScreen {
	background-color: #0b0d10;
	border: 1px solid #2a2f36;
}
#stagePreviewScreen[live="true"], #stageProgramScreen[live="true"] {
	border: 2px solid #d92d20;
}
#stageContent {
	color: #f2f4f7;
	font-size: 12px;
}
#stageCaption {
	color: #8b95a1;
	font-size: 11px;
}
#onAirButton {
	font-weight: 700;
	letter-spacing: 1px;
	min-height: 28px;
}
#onAirButton:enabled {
	background-color: #d92d20;
	color: #ffffff;
}
#dockStatus {
	color: #8b95a1;
	font-size: 11px;
}
#dockStatus[error="true"] {
	color: #f04438;
}
)";

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QString fromStd(const std::string &s)
{
	return QString::fromStdString(s);
}

/** The line shown under the search bar for what is typed. @p isError is true for the red ones. */
QString describeQuery(const search::QueryResult &r, const bible::BibleModule &bible, bool &isError)
{
	using search::QueryStatus;
	isError = !r.ok() && !r.inProgress();
	const QString book = fromStd(std::string(search::frenchBookName(r.book)));

	switch (r.status) {
	case QueryStatus::Ok: {
		const size_t count = search::passageVerses(r.passage, bible).size();
		return text("Status.Ready").arg(fromStd(search::formatPassage(r.passage, bible))).arg(count);
	}
	case QueryStatus::Empty:
		return text("Status.Type");
	case QueryStatus::NeedChapter:
		return text("Status.NeedChapter").arg(book);
	case QueryStatus::Incomplete:
		return text("Status.Incomplete").arg(book);
	case QueryStatus::Syntax:
		return text("Error.Syntax");
	case QueryStatus::Unsupported:
		return text("Error.Unsupported");
	case QueryStatus::UnknownBook:
		return text("Error.UnknownBook");
	case QueryStatus::AmbiguousBook: {
		QStringList names;
		for (int candidate : r.candidates)
			names << fromStd(std::string(search::frenchBookName(candidate)));
		return text("Error.AmbiguousBook").arg(names.join(QStringLiteral(", ")));
	}
	case QueryStatus::BookNotInModule:
		return text("Error.BookNotInModule").arg(book);
	case QueryStatus::ChapterOutOfRange:
		return text("Error.ChapterOutOfRange").arg(book).arg(r.limit);
	case QueryStatus::VerseOutOfRange:
		return text("Error.VerseOutOfRange").arg(book).arg(r.limit);
	case QueryStatus::ReversedRange:
		return text("Error.ReversedRange");
	}
	return {};
}

/** A slide as Qt rich text. Everything that comes from the Bible is escaped. */
QString slideHtml(const stage::Slide &slide)
{
	QString html = QStringLiteral("<p><b>%1</b></p><p>").arg(fromStd(slide.reference).toHtmlEscaped());
	const bool numbered = slide.verses.size() > 1;
	for (const stage::SlideVerse &v : slide.verses) {
		if (numbered)
			html += QStringLiteral("<sup>%1</sup>&nbsp;").arg(v.verse);
		html += fromStd(v.text).toHtmlEscaped() + QLatin1Char(' ');
	}
	return html + QStringLiteral("</p>");
}

} // namespace

void VyraDock::setStatus(const QString &text, bool isError)
{
	statusLabel_->setText(text);
	statusLabel_->setProperty("error", isError);
	// A changed dynamic property is not picked up by the style sheet until the widget is re-polished.
	statusLabel_->style()->unpolish(statusLabel_);
	statusLabel_->style()->polish(statusLabel_);
}

VyraDock::VyraDock(const bible::BibleModule *bible, QWidget *parent) : QWidget(parent), bible_(bible)
{
	setObjectName(QStringLiteral("vyraDock"));
	setStyleSheet(QString::fromUtf8(kStyleSheet));
	if (bible_)
		stage_ = std::make_unique<stage::StageController>(*bible_);

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	searchEdit_ = new QLineEdit(this);
	searchEdit_->setPlaceholderText(text("Dock.Search.Placeholder"));
	searchEdit_->setClearButtonEnabled(true);
	searchEdit_->setToolTip(text("Tooltip.Search"));
	searchEdit_->installEventFilter(this);
	root->addWidget(searchEdit_);

	// Control-room monitors
	auto *stages = new QHBoxLayout();
	stages->setSpacing(8);
	preview_ = new StagePanel(text("Stage.Preview"), text("Stage.Preview.Empty"),
				  QStringLiteral("stagePreviewScreen"), this);
	program_ = new StagePanel(text("Stage.Program"), text("Stage.Program.Empty"),
				  QStringLiteral("stageProgramScreen"), this);
	stages->addWidget(preview_, 1);
	stages->addWidget(program_, 1);
	root->addLayout(stages);

	auto *buttons = new QHBoxLayout();
	buttons->setSpacing(6);
	previousButton_ = new QPushButton(text("Button.Previous"), this);
	nextButton_ = new QPushButton(text("Button.Next"), this);
	hideButton_ = new QPushButton(text("Button.Hide"), this);
	onAirButton_ = new QPushButton(text("Button.OnAir"), this);
	onAirButton_->setObjectName(QStringLiteral("onAirButton"));
	previousButton_->setToolTip(text("Tooltip.Previous"));
	nextButton_->setToolTip(text("Tooltip.Next"));
	hideButton_->setToolTip(text("Tooltip.Hide"));
	onAirButton_->setToolTip(text("Tooltip.OnAir"));

	// The keyboard flow (type, Enter, Ctrl+Enter) must never be interrupted by a click moving the focus.
	for (QPushButton *button : {previousButton_, nextButton_, hideButton_, onAirButton_})
		button->setFocusPolicy(Qt::NoFocus);

	buttons->addWidget(previousButton_);
	buttons->addWidget(nextButton_);
	buttons->addWidget(hideButton_);
	buttons->addStretch(1);
	buttons->addWidget(onAirButton_);
	root->addLayout(buttons);

	root->addStretch(1);

	statusLabel_ = new QLabel(this);
	statusLabel_->setObjectName(QStringLiteral("dockStatus"));
	statusLabel_->setWordWrap(true);
	root->addWidget(statusLabel_);

	connect(searchEdit_, &QLineEdit::textChanged, this, [this] { onQueryChanged(); });
	connect(previousButton_, &QPushButton::clicked, this, [this] { navigate(false); });
	connect(nextButton_, &QPushButton::clicked, this, [this] { navigate(true); });
	connect(hideButton_, &QPushButton::clicked, this, [this] { hideProgram(); });
	connect(onAirButton_, &QPushButton::clicked, this, [this] { airFromField(); });

	searchEdit_->setEnabled(stage_ != nullptr);
	refresh();
	obs_log(LOG_DEBUG, "dock widgets created");
}

VyraDock::~VyraDock() = default;

void VyraDock::setProgramListener(ProgramListener listener)
{
	programListener_ = std::move(listener);
	notifyProgram();
}

void VyraDock::notifyProgram()
{
	if (programListener_ && stage_)
		programListener_(*stage_);
}

void VyraDock::onQueryChanged()
{
	if (syncingField_ || !bible_)
		return;
	query_ = search::resolveQuery(searchEdit_->text().toUtf8().toStdString(), *bible_);
	bool isError = false;
	const QString message = describeQuery(query_, *bible_, isError);
	setStatus(message, isError);
}

void VyraDock::previewFromField()
{
	if (!stage_ || !query_.ok())
		return;
	if (stage_->showInPreview(query_.passage)) {
		setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(query_.passage, *bible_))), false);
		refresh();
	}
}

void VyraDock::airFromField()
{
	if (!stage_)
		return;
	if (!searchEdit_->text().trimmed().isEmpty()) {
		// Something is typed: it must be valid, otherwise nothing goes on the air (the error is on screen).
		if (!query_.ok())
			return;
		stage_->showInPreview(query_.passage);
	}
	if (stage_->takeOnAir()) {
		setStatus(text("Status.OnAir").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))), false);
		setFieldToPreview();
		refresh();
		notifyProgram();
	}
}

void VyraDock::navigate(bool forward)
{
	if (!stage_)
		return;
	if (forward ? stage_->previewNext() : stage_->previewPrevious()) {
		setFieldToPreview();
		setStatus(text("Status.Previewed").arg(fromStd(search::formatPassage(*stage_->preview(), *bible_))), false);
		refresh();
	}
}

void VyraDock::hideProgram()
{
	if (stage_ && stage_->hideProgram()) {
		setStatus(text("Status.Hidden").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))), false);
		refresh();
		notifyProgram();
	}
}

/** Keeps the search bar equal to the preview, so that Ctrl+Enter always airs what the operator sees. */
void VyraDock::setFieldToPreview()
{
	if (!stage_ || !stage_->preview())
		return;
	syncingField_ = true;
	searchEdit_->setText(fromStd(search::formatPassage(*stage_->preview(), *bible_)));
	syncingField_ = false;
	query_ = search::resolveQuery(searchEdit_->text().toUtf8().toStdString(), *bible_);
}

void VyraDock::refresh()
{
	const bool have = stage_ != nullptr;

	if (have && stage_->preview())
		preview_->setContent(slideHtml(stage_->slide(*stage_->preview())));
	else
		preview_->setCaption(text("Stage.Preview.Empty"));

	if (have && stage_->programLive()) {
		program_->setContent(slideHtml(stage_->slide(*stage_->program())));
		program_->setLive(true);
	} else if (have && stage_->program()) {
		program_->setCaption(
			text("Stage.Program.Hidden").arg(fromStd(search::formatPassage(*stage_->program(), *bible_))));
		program_->setLive(false);
	} else {
		program_->setCaption(text("Stage.Program.Empty"));
		program_->setLive(false);
	}

	previousButton_->setEnabled(have && stage_->hasPrevious());
	nextButton_->setEnabled(have && stage_->hasNext());
	hideButton_->setEnabled(have && stage_->programLive());
	onAirButton_->setEnabled(have && stage_->preview().has_value());
}

bool VyraDock::eventFilter(QObject *watched, QEvent *event)
{
	if (watched != searchEdit_ || event->type() != QEvent::KeyPress || !stage_)
		return QWidget::eventFilter(watched, event);

	const auto *key = static_cast<QKeyEvent *>(event);
	const bool ctrl = key->modifiers() & Qt::ControlModifier;
	const bool plain = (key->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier;

	switch (key->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
		if (ctrl)
			airFromField();
		else
			previewFromField();
		return true;
	case Qt::Key_Up:
	case Qt::Key_Down:
		if (plain && stage_->preview()) {
			navigate(key->key() == Qt::Key_Down);
			return true;
		}
		break;
	case Qt::Key_Escape:
		if (stage_->programLive()) {
			hideProgram();
			return true;
		}
		break;
	default:
		break;
	}
	return QWidget::eventFilter(watched, event);
}

} // namespace vyra::ui
