/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/stage_panel.hpp"

#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace vyra::ui {

AspectFrame::AspectFrame(const QString &caption, QWidget *parent) : QFrame(parent)
{
	captionLabel_ = new QLabel(caption, this);
	captionLabel_->setObjectName(QStringLiteral("stageCaption"));
	captionLabel_->setAlignment(Qt::AlignCenter);
	captionLabel_->setWordWrap(true);

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

} // namespace vyra::ui
