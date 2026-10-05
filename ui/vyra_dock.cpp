/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/vyra_dock.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

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

} // namespace

void VyraDock::setStatus(const QString &text, bool isError)
{
	statusLabel_->setText(text);
	statusLabel_->setProperty("error", isError);
	// A changed dynamic property is not picked up by the style sheet until the widget is re-polished.
	statusLabel_->style()->unpolish(statusLabel_);
	statusLabel_->style()->polish(statusLabel_);
}

VyraDock::VyraDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("vyraDock"));
	setStyleSheet(QString::fromUtf8(kStyleSheet));

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	// Search bar (arrives with Milestone 03)
	searchEdit_ = new QLineEdit(this);
	searchEdit_->setPlaceholderText(text("Dock.Search.Placeholder"));
	searchEdit_->setClearButtonEnabled(true);
	searchEdit_->setEnabled(false);
	searchEdit_->setToolTip(text("Tooltip.NotYet"));
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

	// Transport buttons (arrive with Milestone 04)
	auto *buttons = new QHBoxLayout();
	buttons->setSpacing(6);
	previousButton_ = new QPushButton(text("Button.Previous"), this);
	nextButton_ = new QPushButton(text("Button.Next"), this);
	hideButton_ = new QPushButton(text("Button.Hide"), this);
	onAirButton_ = new QPushButton(text("Button.OnAir"), this);
	onAirButton_->setObjectName(QStringLiteral("onAirButton"));

	for (QPushButton *button : {previousButton_, nextButton_, hideButton_, onAirButton_}) {
		button->setEnabled(false);
		button->setToolTip(text("Tooltip.NotYet"));
	}

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

	obs_log(LOG_DEBUG, "dock widgets created");
}

} // namespace vyra::ui
