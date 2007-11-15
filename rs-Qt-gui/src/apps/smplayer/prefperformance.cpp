/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "prefperformance.h"
#include "images.h"
#include "preferences.h"


PrefPerformance::PrefPerformance(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	// Priority is only for windows, so we disable for other systems
#ifndef Q_OS_WIN
	priority_group->hide();
#endif

	createHelp();
}

PrefPerformance::~PrefPerformance()
{
}

QString PrefPerformance::sectionName() {
	return tr("Performance");
}

QPixmap PrefPerformance::sectionIcon() {
    return Images::icon("pref_performance");
}


void PrefPerformance::retranslateStrings() {
	int priority = priority_combo->currentIndex();

	retranslateUi(this);

	priority_combo->setCurrentIndex(priority);

	createHelp();
}

void PrefPerformance::setData(Preferences * pref) {
	setCacheEnabled( pref->use_cache );
	setCache( pref->cache );
	setPriority( pref->priority );
	setFrameDrop( pref->frame_drop );
	setHardFrameDrop( pref->hard_frame_drop );
	setAutoSyncActivated( pref->autosync );
	setAutoSyncFactor( pref->autosync_factor );
	setFastChapterSeeking( pref->fast_chapter_change );
	setFastAudioSwitching( !pref->audio_change_requires_restart );
	setUseIdx( pref->use_idx );
}

void PrefPerformance::getData(Preferences * pref) {
	requires_restart = false;

	TEST_AND_SET(pref->use_cache, cacheEnabled());
	TEST_AND_SET(pref->cache, cache());
	TEST_AND_SET(pref->priority, priority());
	TEST_AND_SET(pref->frame_drop, frameDrop());
	TEST_AND_SET(pref->hard_frame_drop, hardFrameDrop());
	TEST_AND_SET(pref->autosync, autoSyncActivated());
	TEST_AND_SET(pref->autosync_factor, autoSyncFactor());
	TEST_AND_SET(pref->fast_chapter_change, fastChapterSeeking());
	pref->audio_change_requires_restart = !fastAudioSwitching();
	TEST_AND_SET(pref->use_idx, useIdx());
}

void PrefPerformance::setCacheEnabled(bool b) {
	use_cache_check->setChecked(b);
}

bool PrefPerformance::cacheEnabled() {
	return use_cache_check->isChecked();
}

void PrefPerformance::setCache(int n) {
	cache_spin->setValue(n);
}

int PrefPerformance::cache() {
	return cache_spin->value();
}

void PrefPerformance::setPriority(int n) {
	priority_combo->setCurrentIndex(n);
}

int PrefPerformance::priority() {
	return priority_combo->currentIndex();
}

void PrefPerformance::setFrameDrop(bool b) {
	framedrop_check->setChecked(b);
}

bool PrefPerformance::frameDrop() {
	return framedrop_check->isChecked();
}

void PrefPerformance::setHardFrameDrop(bool b) {
	hardframedrop_check->setChecked(b);
}

bool PrefPerformance::hardFrameDrop() {
	return hardframedrop_check->isChecked();
}

void PrefPerformance::setAutoSyncFactor(int factor) {
	autosync_spin->setValue(factor);
}

int PrefPerformance::autoSyncFactor() {
	return autosync_spin->value();
}

void PrefPerformance::setAutoSyncActivated(bool b) {
	autosync_check->setChecked(b);
}

bool PrefPerformance::autoSyncActivated() {
	return autosync_check->isChecked();
}

void PrefPerformance::setFastChapterSeeking(bool b) {
	fast_chapter_check->setChecked(b);
}

bool PrefPerformance::fastChapterSeeking() {
	return fast_chapter_check->isChecked();
}

void PrefPerformance::setFastAudioSwitching(bool b) {
	fastaudioswitching_check->setChecked(b);
}

bool PrefPerformance::fastAudioSwitching() {
	return fastaudioswitching_check->isChecked();
}

void PrefPerformance::setUseIdx(bool b) {
	idx_check->setChecked(b);
}

bool PrefPerformance::useIdx() {
	return idx_check->isChecked();
}

void PrefPerformance::createHelp() {
	clearHelp();

	// Performance tab
	setWhatsThis(priority_combo, tr("Priority"), 
		tr("Set process priority for mplayer according to the predefined "
           "priorities available under Windows.<br>"
           "<b>WARNING:</b> Using realtime priority can cause system lockup.")
#ifndef Q_OS_WIN
        + tr("<br><b>Note:</b> This option is for Windows only.") 
#endif
		);

	setWhatsThis(cache_spin, tr("Cache"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a file or URL. Especially useful on slow media.") );

	setWhatsThis(framedrop_check, tr("Allow frame drop"),
		tr("Skip displaying some frames to maintain A/V sync on slow systems." ) );

	setWhatsThis(hardframedrop_check, tr("Allow hard frame drop"),
		tr("More intense frame dropping (breaks decoding). "
           "Leads to image distortion!") );

	setWhatsThis(autosync_check, tr("Audio/video auto synchronization"),
		tr("Gradually adjusts the A/V sync based on audio delay "
           "measurements.") );

	setWhatsThis(fastaudioswitching_check, tr("Fast audio track switching"),
		tr("If checked, it will try the fastest method to switch audio "
           "tracks but might not work with some formats.") );

	setWhatsThis(fast_chapter_check, tr("Fast seek to chapters in dvds"),
		tr("If checked, it will try the fastest method to seek to chapters "
           "but it might not work with some discs.") );

	setWhatsThis(idx_check, tr("Create index if needed"),
		tr("Rebuilds index of files if no index was found, allowing seeking. "
		   "Useful with broken/incomplete downloads, or badly created files. "
           "This option only works if the underlying media supports "
           "seeking (i.e. not with stdin, pipe, etc).<br> "
           "Note: the creation of the index may take some time.") );

}

#include "moc_prefperformance.cpp"
