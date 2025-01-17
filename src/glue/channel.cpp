/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2023 Giovanni A. Zuliani | Monocasual Laboratories
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */

#include "gui/elems/mainWindow/keyboard/channel.h"
#include "core/channels/channelFactory.h"
#include "core/conf.h"
#include "core/engine.h"
#include "core/kernelAudio.h"
#include "core/mixer.h"
#include "core/model/model.h"
#include "core/patch.h"
#include "core/plugins/plugin.h"
#include "core/plugins/pluginHost.h"
#include "core/plugins/pluginManager.h"
#include "core/recorder.h"
#include "core/wave.h"
#include "core/waveFactory.h"
#include "glue/channel.h"
#include "glue/main.h"
#include "gui/dialogs/mainWindow.h"
#include "gui/dialogs/sampleEditor.h"
#include "gui/dialogs/warnings.h"
#include "gui/dispatcher.h"
#include "gui/elems/basics/dial.h"
#include "gui/elems/basics/input.h"
#include "gui/elems/mainWindow/keyboard/channelButton.h"
#include "gui/elems/mainWindow/keyboard/keyboard.h"
#include "gui/elems/mainWindow/keyboard/sampleChannel.h"
#include "gui/elems/sampleEditor/pitchTool.h"
#include "gui/elems/sampleEditor/rangeTool.h"
#include "gui/elems/sampleEditor/waveTools.h"
#include "gui/elems/sampleEditor/waveform.h"
#include "gui/ui.h"
#include "src/gui/elems/panTool.h"
#include "src/gui/elems/volumeTool.h"
#include "utils/fs.h"
#include "utils/gui.h"
#include "utils/log.h"
#include <FL/Fl.H>
#include <cassert>
#include <cmath>
#include <functional>

extern giada::v::Ui     g_ui;
extern giada::m::Engine g_engine;

namespace giada::c::channel
{
namespace
{
void printLoadError_(int res)
{
	if (res == G_RES_ERR_WRONG_DATA)
		v::gdAlert(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_MULTICHANNOTSUPPORTED));
	else if (res == G_RES_ERR_IO)
		v::gdAlert(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_CANTREADSAMPLE));
	else if (res == G_RES_ERR_PATH_TOO_LONG)
		v::gdAlert(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_PATHTOOLONG));
	else if (res == G_RES_ERR_NO_DATA)
		v::gdAlert(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_NOFILESPECIFIED));
}
} // namespace

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

SampleData::SampleData(const m::Channel& ch)
: waveId(ch.samplePlayer->getWaveId())
, mode(ch.samplePlayer->mode)
, isLoop(ch.samplePlayer->isAnyLoopMode())
, pitch(ch.samplePlayer->pitch)
, begin(ch.samplePlayer->begin)
, end(ch.samplePlayer->end)
, inputMonitor(ch.audioReceiver->inputMonitor)
, overdubProtection(ch.audioReceiver->overdubProtection)
, m_tracker(&ch.shared->tracker)
{
}

Frame SampleData::getTracker() const { return m_tracker->load(); }

/* -------------------------------------------------------------------------- */

MidiData::MidiData(const m::Channel& m)
: isOutputEnabled(m.midiSender->enabled)
, filter(m.midiSender->filter)
{
}

/* -------------------------------------------------------------------------- */

Data::Data(const m::Channel& c)
: id(c.id)
, columnId(c.columnId)
, position(c.position)
, plugins(c.plugins)
, type(c.type)
, height(c.height)
, name(c.name)
, volume(c.volume)
, pan(c.pan)
, key(c.key)
, hasActions(c.hasActions)
, m_playStatus(&c.shared->playStatus)
, m_recStatus(&c.shared->recStatus)
, m_readActions(&c.shared->readActions)
{
	if (c.type == ChannelType::SAMPLE)
		sample = std::make_optional<SampleData>(c);
	else if (c.type == ChannelType::MIDI)
		midi = std::make_optional<MidiData>(c);
}

ChannelStatus Data::getPlayStatus() const { return m_playStatus->load(); }
ChannelStatus Data::getRecStatus() const { return m_recStatus->load(); }
bool          Data::getReadActions() const { return m_readActions->load(); }
bool          Data::isRecordingInput() const { return g_engine.getMainApi().isRecordingInput(); }
bool          Data::isRecordingActions() const { return g_engine.getMainApi().isRecordingActions(); }
bool          Data::isMuted() const { return g_engine.getChannelsApi().get(id).isMuted(); }
bool          Data::isSoloed() const { return g_engine.getChannelsApi().get(id).isSoloed(); }
bool          Data::isArmed() const { return g_engine.getChannelsApi().get(id).armed; }

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

Data getData(ID channelId)
{
	return Data(g_engine.getChannelsApi().get(channelId));
}

std::vector<Data> getChannels()
{
	std::vector<Data> out;
	for (const m::Channel& ch : g_engine.getChannelsApi().getAll())
		if (!ch.isInternal())
			out.push_back(Data(ch));

	std::sort(out.begin(), out.end(), [](const Data& a, const Data& b) { return a.position < b.position; });

	return out;
}

/* -------------------------------------------------------------------------- */

void loadChannel(ID channelId, const std::string& fname)
{
	auto progress = g_ui.mainWindow->getScopedProgress(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_LOADINGSAMPLES));

	int res = g_engine.getChannelsApi().loadSampleChannel(channelId, fname);
	if (res != G_RES_OK)
		printLoadError_(res);

	if (auto* w = sampleEditor::getWindow(); w != nullptr)
		w->rebuild();
}

/* -------------------------------------------------------------------------- */

void addChannel(ID columnId, ChannelType type)
{
	g_engine.getChannelsApi().add(columnId, type);
}

/* -------------------------------------------------------------------------- */

void addAndLoadChannels(ID columnId, const std::vector<std::string>& fnames)
{
	auto progress      = g_ui.mainWindow->getScopedProgress(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_LOADINGSAMPLES));
	auto channelEngine = g_engine.getChannelsApi();

	int  i      = 0;
	bool errors = false;
	for (const std::string& f : fnames)
	{
		progress.setProgress(++i / static_cast<float>(fnames.size()));

		const m::Channel& ch  = channelEngine.add(columnId, ChannelType::SAMPLE);
		const int         res = channelEngine.loadSampleChannel(ch.id, f);
		if (res != G_RES_OK)
			errors = true;
	}

	if (errors)
		v::gdAlert(g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_LOADINGSAMPLESERROR));
}

/* -------------------------------------------------------------------------- */

void deleteChannel(ID channelId)
{
	if (!v::gdConfirmWin(g_ui.getI18Text(v::LangMap::COMMON_WARNING), g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_DELETE)))
		return;
	g_ui.closeAllSubwindows();
	g_engine.getChannelsApi().remove(channelId);
}

/* -------------------------------------------------------------------------- */

void freeChannel(ID channelId)
{
	if (!v::gdConfirmWin(g_ui.getI18Text(v::LangMap::COMMON_WARNING), g_ui.getI18Text(v::LangMap::MESSAGE_CHANNEL_FREE)))
		return;
	g_ui.closeAllSubwindows();
	g_engine.getChannelsApi().freeSampleChannel(channelId);
}

/* -------------------------------------------------------------------------- */

void setInputMonitor(ID channelId, bool value)
{
	g_engine.getChannelsApi().setInputMonitor(channelId, value);
}

/* -------------------------------------------------------------------------- */

void setOverdubProtection(ID channelId, bool value)
{
	g_engine.getChannelsApi().setOverdubProtection(channelId, value);
}

/* -------------------------------------------------------------------------- */

void cloneChannel(ID channelId)
{
	g_engine.getChannelsApi().clone(channelId);
}

/* -------------------------------------------------------------------------- */

void moveChannel(ID channelId, ID columnId, int position)
{
	g_engine.getChannelsApi().move(channelId, columnId, position);
}

/* -------------------------------------------------------------------------- */

void setSamplePlayerMode(ID channelId, SamplePlayerMode mode)
{
	g_engine.getChannelsApi().setSamplePlayerMode(channelId, mode);
	g_ui.refreshSubWindow(WID_ACTION_EDITOR);
}

/* -------------------------------------------------------------------------- */

void setHeight(ID channelId, Pixel p)
{
	g_engine.getChannelsApi().setHeight(channelId, p);
}

/* -------------------------------------------------------------------------- */

void setName(ID channelId, const std::string& name)
{
	g_engine.getChannelsApi().setName(channelId, name);
}

/* -------------------------------------------------------------------------- */

void clearAllActions(ID channelId)
{
	if (!v::gdConfirmWin(g_ui.getI18Text(v::LangMap::COMMON_WARNING),
	        g_ui.getI18Text(v::LangMap::MESSAGE_MAIN_CLEARALLACTIONS)))
		return;

	g_engine.getChannelsApi().clearAllActions(channelId);
	g_ui.refreshSubWindow(WID_ACTION_EDITOR);
}

/* -------------------------------------------------------------------------- */

void setCallbacks(m::Channel& ch)
{
	auto onSendMidiCb = [channelId = ch.id]() {
		g_ui.pumpEvent([channelId]() { g_ui.mainWindow->keyboard->notifyMidiOut(channelId); });
	};

	ch.midiLighter.onSend = onSendMidiCb;
	if (ch.midiSender)
		ch.midiSender->onSend = onSendMidiCb;
}

/* -------------------------------------------------------------------------- */

void pressChannel(ID channelId, int velocity, Thread t)
{
	g_engine.getChannelsApi().press(channelId, velocity);
	notifyChannelForMidiIn(t, channelId);
}

void releaseChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().release(channelId);
	notifyChannelForMidiIn(t, channelId);
}

void killChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().kill(channelId);
	notifyChannelForMidiIn(t, channelId);
}

/* -------------------------------------------------------------------------- */

float setChannelVolume(ID channelId, float v, Thread t, bool repaintMainUi)
{
	g_engine.getChannelsApi().setVolume(channelId, v);
	notifyChannelForMidiIn(t, channelId);

	if (t != Thread::MAIN || repaintMainUi)
		g_ui.pumpEvent([channelId, v]() { g_ui.mainWindow->keyboard->setChannelVolume(channelId, v); });

	return v;
}

/* -------------------------------------------------------------------------- */

float setChannelPitch(ID channelId, float v, Thread t)
{
	g_engine.getChannelsApi().setPitch(channelId, v);
	g_ui.pumpEvent([v]() {
		if (auto* w = sampleEditor::getWindow(); w != nullptr)
			w->pitchTool->update(v); });
	notifyChannelForMidiIn(t, channelId);
	return v;
}

/* -------------------------------------------------------------------------- */

float sendChannelPan(ID channelId, float v)
{
	g_engine.getChannelsApi().setPan(channelId, v);
	notifyChannelForMidiIn(Thread::MAIN, channelId); // Currently triggered only by the main thread
	return v;
}

/* -------------------------------------------------------------------------- */

void toggleMuteChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().toggleMute(channelId);
	notifyChannelForMidiIn(t, channelId);
}

void toggleSoloChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().toggleSolo(channelId);
	notifyChannelForMidiIn(t, channelId);
}

/* -------------------------------------------------------------------------- */

void toggleArmChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().toggleArm(channelId);
	notifyChannelForMidiIn(t, channelId);
}

void toggleReadActionsChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().toggleReadActions(channelId);
	notifyChannelForMidiIn(t, channelId);
}

void killReadActionsChannel(ID channelId, Thread t)
{
	g_engine.getChannelsApi().killReadActions(channelId);
	notifyChannelForMidiIn(t, channelId);
}

/* -------------------------------------------------------------------------- */

void sendMidiToChannel(ID channelId, m::MidiEvent e, Thread t)
{
	g_engine.getChannelsApi().sendMidi(channelId, e);
	notifyChannelForMidiIn(t, channelId);
}

/* -------------------------------------------------------------------------- */

void notifyChannelForMidiIn(Thread t, ID channelId)
{
	if (t == Thread::MIDI)
		g_ui.pumpEvent([channelId]() { g_ui.mainWindow->keyboard->notifyMidiIn(channelId); });
}

} // namespace giada::c::channel
