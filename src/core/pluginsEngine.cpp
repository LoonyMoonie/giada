/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2022 Giovanni A. Zuliani | Monocasual Laboratories
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

#include "core/pluginsEngine.h"
#include "core/channels/channelManager.h"
#include "core/engine.h"
#include "core/kernelAudio.h"
#include "core/mixer.h"
#include "utils/fs.h"

namespace giada::m
{
PluginsEngine::PluginsEngine(Engine& e, KernelAudio& ka, ChannelManager& cm, PluginManager& pm, PluginHost& ph, model::Model& m)
: m_engine(e)
, m_kernelAudio(ka)
, m_channelManager(cm)
, m_pluginManager(pm)
, m_pluginHost(ph)
, m_model(m)
{
}

/* -------------------------------------------------------------------------- */

const Plugin* PluginsEngine::get(ID pluginId) const
{
	return m_model.findShared<Plugin>(pluginId);
}

/* -------------------------------------------------------------------------- */

std::vector<PluginManager::PluginInfo> PluginsEngine::getInfo() const
{
	return m_pluginManager.getPluginsInfo();
}

/* -------------------------------------------------------------------------- */

int PluginsEngine::countAvailablePlugins() const
{
	return m_pluginManager.countAvailablePlugins();
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::add(int pluginListIndex, ID channelId)
{
	/* Plug-in creation must be done in the main thread, due to JUCE and VST3
	internal workings. */

	if (pluginListIndex >= m_pluginManager.countAvailablePlugins())
		return;

	const int sampleRate = m_kernelAudio.getSampleRate();
	const int bufferSize = m_kernelAudio.getBufferSize();

	std::unique_ptr<m::Plugin> plugin    = m_pluginManager.makePlugin(pluginListIndex, sampleRate, bufferSize, m_model.get().sequencer);
	const Plugin*              pluginPtr = plugin.get();
	if (plugin != nullptr)
		m_pluginHost.addPlugin(std::move(plugin));

	/* TODO - unfortunately JUCE wants mutable plugin objects due to the 
		presence of the non-const processBlock() method. Why not const_casting
		only in the Plugin class? */
	m_model.get().getChannel(channelId).plugins.push_back(const_cast<Plugin*>(pluginPtr));
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::swap(const Plugin& p1, const Plugin& p2, ID channelId)
{
	m_pluginHost.swapPlugin(p1, p2, m_model.get().getChannel(channelId).plugins);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::sort(PluginManager::SortMethod method)
{
	m_pluginManager.sortPlugins(method);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::free(const Plugin& plugin, ID channelId)
{
	u::vector::remove(m_model.get().getChannel(channelId).plugins, &plugin);
	m_model.swap(model::SwapType::HARD);
	m_pluginHost.freePlugin(plugin);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::setProgram(ID pluginId, int programIndex)
{
	m_pluginHost.setPluginProgram(pluginId, programIndex);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::toggleBypass(ID pluginId)
{
	m_pluginHost.toggleBypass(pluginId);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::setParameter(ID pluginId, int paramIndex, float value)
{
	m_pluginHost.setPluginParameter(pluginId, paramIndex, value);
}

/* -------------------------------------------------------------------------- */

void PluginsEngine::scan(const std::string& dir, const std::function<void(float)>& progress)
{
	m_pluginManager.scanDirs(dir, progress);
	m_pluginManager.saveList(u::fs::join(u::fs::getHomePath(), "plugins.xml"));
}

/* -------------------------------------------------------------------------- */

const Patch::Plugin PluginsEngine::serialize(const Plugin& p) const
{
	return m_pluginManager.serializePlugin(p);
}

std::unique_ptr<Plugin> PluginsEngine::deserialize(const Patch::Plugin& pplugin)
{
	const int sampleRate = m_engine.getSampleRate();
	const int bufferSize = m_engine.getBufferSize();

	return m_pluginManager.deserializePlugin(pplugin, sampleRate, bufferSize, m_model.get().sequencer);
}
} // namespace giada::m