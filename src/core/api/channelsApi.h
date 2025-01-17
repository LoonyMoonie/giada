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

#ifndef G_CHANNELS_API_H
#define G_CHANNELS_API_H

#include "core/channels/channelFactory.h"
#include "core/patch.h"
#include "core/types.h"
#include <string>
#include <vector>

namespace giada::m
{
class MidiEvent;
class Engine;
class KernelAudio;
class Mixer;
class Sequencer;
class ChannelManager;
class Channel;
class Recorder;
class ActionRecorder;
class PluginHost;
class PluginManager;
class Wave;
class ChannelsApi
{
public:
	ChannelsApi(model::Model&, KernelAudio&, Mixer&, Sequencer&, ChannelManager&,
	    Recorder&, ActionRecorder&, PluginHost&, PluginManager&);

	bool hasChannelsWithAudioData() const;
	bool hasChannelsWithActions() const;

	Channel&              get(ID);
	std::vector<Channel>& getAll();

	Channel& add(ID columnId, ChannelType);
	int      loadSampleChannel(ID channelId, const std::string& filePath);
	void     loadSampleChannel(ID channelId, Wave&);
	void     loadPreviewChannel(ID sourceChannelId);
	void     remove(ID);
	void     freeSampleChannel(ID);
	void     freePreviewChannel();
	void     clone(ID);
	void     move(ID channelId, ID columnId, int position);

	void press(ID, int velocity);
	void release(ID);
	void kill(ID);
	void setVolume(ID, float);
	void setPitch(ID, float);
	void setPan(ID, float);
	void toggleMute(ID);
	void toggleSolo(ID);
	void toggleArm(ID);
	void toggleReadActions(ID);
	void killReadActions(ID);
	void setInputMonitor(ID, bool value);
	void setOverdubProtection(ID, bool value);
	void setSamplePlayerMode(ID, SamplePlayerMode);
	void setHeight(ID, int);
	void setName(ID, const std::string&);
	void setPreviewTracker(Frame);
	void clearAllActions(ID);
	void clearAllActions();
	void freeAllSampleChannels();
	void sendMidi(ID, MidiEvent);
	bool saveSample(ID, const std::string& filePath);

private:
	model::Model&   m_model;
	KernelAudio&    m_kernelAudio;
	Mixer&          m_mixer;
	Sequencer&      m_sequencer;
	ChannelManager& m_channelManager;
	Recorder&       m_recorder;
	ActionRecorder& m_actionRecorder;
	PluginHost&     m_pluginHost;
	PluginManager&  m_pluginManager;
};
} // namespace giada::m

#endif
