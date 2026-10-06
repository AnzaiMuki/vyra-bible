/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/stage_panel.hpp"

#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

namespace vyra::ui {

AspectFrame::AspectFrame(const QString &caption, QWidget *parent) : QFrame(parent)
{
	captionLabel_ = new QLabel(caption, this);
	captionLabel_->setObjectName(QStringLiteral("stageCaption"));
	captionLabel_->setAlignment(Qt::AlignCenter);
	captionLabel_->setWordWrap(true);

	contentLabel_ = new QLabel(this);
	contentLabel_->setObjectName(QStringLiteral("stageContent"));
	contentLabel_->setTextFormat(Qt::RichText);
	contentLabel_->setAlignment(Qt::AlignCenter);
	contentLabel_->setWordWrap(true);
	contentLabel_->hide();

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	setSizePolicy(policy);
	setMinimumWidth(120);
}

int AspectFrame::heightForWidth(int width) const
{
	return (width * 9) / 16;
}

void AspectFrame::resizeEvent(QResizeEvent *event)
{
	QFrame::resizeEvent(event);
	captionLabel_->setGeometry(rect().adjusted(8, 8, -8, -8));
	contentLabel_->setGeometry(rect().adjusted(8, 8, -8, -8));
}

void AspectFrame::setContent(const QString &html)
{
	contentLabel_->setText(html);
	contentLabel_->show();
	captionLabel_->hide();
}

void AspectFrame::setCaption(const QString &caption)
{
	captionLabel_->setText(caption);
	captionLabel_->show();
	contentLabel_->hide();
}

StagePanel::StagePanel(const QString &title, const QString &caption, const QString &screenName, QWidget *parent)
	: QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	titleLabel_ = new QLabel(title, this);
	titleLabel_->setObjectName(QStringLiteral("stageTitle"));
	layout->addWidget(titleLabel_);

	screen_ = new AspectFrame(caption, this);
	screen_->setObjectName(screenName);
	layout->addWidget(screen_);
}

void StagePanel::setLive(bool live)
{
	screen_->setProperty("live", live);
	screen_->style()->unpolish(screen_);
	screen_->style()->polish(screen_);
}

} // namespace vyra::ui
