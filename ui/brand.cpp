/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ui/brand.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>

namespace vyra::ui {

BrandMark::BrandMark(int size, QWidget *parent) : QWidget(parent)
{
	setFixedSize(size, size);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void BrandMark::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	const qreal s = width();

	p.setPen(Qt::NoPen);
	p.setBrush(QColor(0x12, 0x16, 0x1b));
	p.drawRoundedRect(QRectF(0, 0, s, s), s * 0.22, s * 0.22); // the plate

	QPen ring(QColor(0xf3, 0xf6, 0xf9), s * 0.085);
	p.setPen(ring);
	p.setBrush(Qt::NoBrush);
	const qreal inset = s * 0.255;
	p.drawRoundedRect(QRectF(inset, inset, s - 2 * inset, s - 2 * inset), s * 0.12, s * 0.12);

	p.setPen(Qt::NoPen);
	p.setBrush(QColor(0xa8, 0x82, 0x5a)); // the tan bar
	p.drawRoundedRect(QRectF(s * 0.385, s * 0.375, s * 0.1, s * 0.25), s * 0.05, s * 0.05);
	p.setBrush(QColor(0x45, 0x4d, 0x57)); // the grey dot
	p.drawEllipse(QPointF(s * 0.6, s * 0.585), s * 0.04, s * 0.04);
}

SegmentedControl::SegmentedControl(const QStringList &labels, QWidget *parent) : QFrame(parent)
{
	setProperty("segmentedControl", true); // style selector (the object name is free for the owner)
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(3, 3, 3, 3);
	layout->setSpacing(2);
	group_ = new QButtonGroup(this);
	group_->setExclusive(true);
	for (int i = 0; i < labels.size(); ++i) {
		auto *button = new QPushButton(labels[i], this);
		button->setCheckable(true);
		button->setFocusPolicy(Qt::NoFocus); // the keyboard stays in the search bar
		button->setCursor(Qt::PointingHandCursor);
		group_->addButton(button, i);
		layout->addWidget(button);
	}
	if (group_->button(0))
		group_->button(0)->setChecked(true);
	connect(group_, &QButtonGroup::idClicked, this, &SegmentedControl::activated);
}

int SegmentedControl::count() const
{
	return group_->buttons().size();
}

int SegmentedControl::currentIndex() const
{
	return group_->checkedId();
}

void SegmentedControl::setCurrentIndex(int index)
{
	if (QAbstractButton *b = group_->button(index))
		b->setChecked(true);
}

QAbstractButton *SegmentedControl::segment(int index) const
{
	return group_->button(index);
}

void SegmentedControl::setSegmentToolTip(int index, const QString &tip)
{
	if (QAbstractButton *b = group_->button(index))
		b->setToolTip(tip);
}

} // namespace vyra::ui
