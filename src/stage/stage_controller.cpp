/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/stage/stage_controller.hpp"

namespace vyra::stage {

namespace {

constexpr int kLastBook = 66;

bool isInside(const search::Passage &p, const bible::BibleModule &m)
{
	if (p.book < 1 || p.book > kLastBook)
		return false;
	if (p.startChapter < 1 || p.endChapter < p.startChapter || p.endChapter > m.chapterCount(p.book))
		return false;
	if (p.startVerse < 1 || p.startVerse > m.verseCount(p.book, p.startChapter))
		return false;
	if (p.endVerse < 1 || p.endVerse > m.verseCount(p.book, p.endChapter))
		return false;
	if (p.startChapter == p.endChapter && p.endVerse < p.startVerse)
		return false;
	return true;
}

search::Passage single(const bible::Reference &r)
{
	return {r.book, r.chapter, r.verse, r.chapter, r.verse};
}

} // namespace

bool StageController::showInPreview(const search::Passage &passage)
{
	if (!isInside(passage, module_))
		return false;
	preview_ = passage;
	return true;
}

std::optional<bible::Reference> StageController::verseAfterPreview() const
{
	if (!preview_)
		return std::nullopt;
	const search::Passage &p = *preview_;
	if (p.endVerse < module_.verseCount(p.book, p.endChapter))
		return bible::Reference{p.book, p.endChapter, p.endVerse + 1};
	if (p.endChapter < module_.chapterCount(p.book))
		return bible::Reference{p.book, p.endChapter + 1, 1};
	for (int book = p.book + 1; book <= kLastBook; ++book) {
		if (module_.chapterCount(book) > 0)
			return bible::Reference{book, 1, 1};
	}
	return std::nullopt;
}

std::optional<bible::Reference> StageController::verseBeforePreview() const
{
	if (!preview_)
		return std::nullopt;
	const search::Passage &p = *preview_;
	if (p.startVerse > 1)
		return bible::Reference{p.book, p.startChapter, p.startVerse - 1};
	if (p.startChapter > 1)
		return bible::Reference{p.book, p.startChapter - 1, module_.verseCount(p.book, p.startChapter - 1)};
	for (int book = p.book - 1; book >= 1; --book) {
		const int chapters = module_.chapterCount(book);
		if (chapters > 0)
			return bible::Reference{book, chapters, module_.verseCount(book, chapters)};
	}
	return std::nullopt;
}

bool StageController::hasNext() const
{
	return verseAfterPreview().has_value();
}

bool StageController::hasPrevious() const
{
	return verseBeforePreview().has_value();
}

bool StageController::previewNext()
{
	const auto next = verseAfterPreview();
	if (!next)
		return false;
	preview_ = single(*next);
	return true;
}

bool StageController::previewPrevious()
{
	const auto previous = verseBeforePreview();
	if (!previous)
		return false;
	preview_ = single(*previous);
	return true;
}

const char *themeName(Theme theme)
{
	switch (theme) {
	case Theme::LowerThird:
		return "lower";
	case Theme::FullScreen:
		return "full";
	case Theme::Minimal:
		return "minimal";
	}
	return "lower";
}

std::size_t pageBudget(Theme theme)
{
	switch (theme) {
	case Theme::FullScreen:
		return 650;
	case Theme::Minimal:
	case Theme::LowerThird:
		break;
	}
	return 260;
}

void StageController::repaginate()
{
	programPages_.clear();
	programPage_ = 0;
	if (program_)
		programPages_ = paginate(slide(*program_), pageBudget(theme_));
}

bool StageController::takeOnAir()
{
	if (!preview_)
		return false;
	program_ = preview_;
	programVisible_ = true;
	repaginate();
	return true;
}

void StageController::setTheme(Theme theme)
{
	if (theme == theme_)
		return;
	theme_ = theme;
	repaginate();
}

bool StageController::nextPage()
{
	if (!programLive() || programPage_ + 1 >= programPages_.size())
		return false;
	++programPage_;
	return true;
}

bool StageController::previousPage()
{
	if (!programLive() || programPage_ == 0)
		return false;
	--programPage_;
	return true;
}

bool StageController::hideProgram()
{
	if (!programLive())
		return false;
	programVisible_ = false;
	return true;
}

Slide StageController::slide(const search::Passage &passage) const
{
	Slide s;
	s.reference = search::formatPassage(passage, module_);
	for (const bible::Reference &r : search::passageVerses(passage, module_)) {
		if (const auto text = module_.verse(r))
			s.verses.push_back({r.chapter, r.verse, std::string(*text)});
	}
	return s;
}

} // namespace vyra::stage
