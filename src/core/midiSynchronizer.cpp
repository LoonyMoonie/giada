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

#include "core/midiSynchronizer.h"
#include "core/conf.h"
#include "core/kernelMidi.h"
#include "core/midiEvent.h"
#include "core/model/sequencer.h"
#include "glue/events.h"
#include "utils/log.h"
#include "utils/time.h"
#include <numeric>

namespace giada::m
{
MidiSynchronizer::MidiSynchronizer(const Conf::Data& c, KernelMidi& k)
: m_kernelMidi(k)
, m_conf(c)
, m_timeElapsed(0.0)
, m_lastTimestamp(0.0)
, m_lastDelta(0.0)
, m_lastBpm(G_DEFAULT_BPM)
{
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::receive(const MidiEvent& e, int numBeatsInLoop)
{
	if (m_conf.midiSync != G_MIDI_SYNC_CLOCK_SLAVE || e.getType() != MidiEvent::Type::SYSTEM)
		return;

	switch (e.getByte1())
	{
	case MidiEvent::SYSTEM_CLOCK:
		computeClock(e.getTimestamp());
		break;

	case MidiEvent::SYSTEM_START:
		c::events::startSequencer(Thread::MIDI);
		break;

	case MidiEvent::SYSTEM_STOP:
		c::events::stopSequencer(Thread::MIDI);
		break;

	case MidiEvent::SYSTEM_SPP:
		computePosition(e.getSppPosition(), numBeatsInLoop);
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::advance(geompp::Range<Frame> block, int framesInBeat)
{
	if (m_conf.midiSync != G_MIDI_SYNC_CLOCK_MASTER)
		return;

	/* A MIDI clock event (MIDI_CLOCK) is sent 24 times per quarter note, that 
	is 24 times per beat. This is tempo-relative, since the tempo defines the 
	length of a quarter note (aka frames in beat) and so the duration of each 
	pulse. Faster tempo -> faster MIDI_CLOCK events stream. */

	const int rate = framesInBeat / 24.0;
	for (Frame frame = block.a; frame < block.b; frame++)
	{
		if (frame % rate == 0)
			m_kernelMidi.send(MidiEvent::makeFrom1Byte(MidiEvent::SYSTEM_CLOCK));
	}
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::sendRewind()
{
	if (m_conf.midiSync != G_MIDI_SYNC_CLOCK_MASTER)
		return;
	m_kernelMidi.send(MidiEvent::makeFrom3Bytes(MidiEvent::SYSTEM_SPP, 0, 0));
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::sendStart()
{
	if (m_conf.midiSync != G_MIDI_SYNC_CLOCK_MASTER)
		return;
	m_kernelMidi.send(MidiEvent::makeFrom1Byte(MidiEvent::SYSTEM_START));
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::sendStop()
{
	if (m_conf.midiSync != G_MIDI_SYNC_CLOCK_MASTER)
		return;
	m_kernelMidi.send(MidiEvent::makeFrom1Byte(MidiEvent::SYSTEM_STOP));
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::computeClock(double timestamp)
{
	/* SMOOTHNESS
	Smooth factor for delta and bpm. The smaller the value, the stronger the
	effect. SMOOTHNESS == 1.0 means no smooth effect. */

	constexpr double SMOOTHNESS = 0.1;

	/* BPM_CHANGE_FREQ
	How fast the bpm is changed, in seconds. */

	constexpr double BPM_CHANGE_FREQ = 1.0;

	/* Skip the very first iteration where the last timestamp does not exist
	yet. It would screw up BPM computation otherwise. */

	if (m_lastTimestamp == 0.0)
	{
		m_lastTimestamp = timestamp;
		return;
	}

	/* Compute a raw timestamp delta (rawDelta) and then smooth it out with the
	previous one (m_lastDelta) by a SMOOTHNESS factor. */

	const double rawDelta = timestamp - m_lastTimestamp;
	m_lastDelta           = (rawDelta * SMOOTHNESS) + (m_lastDelta * (1.0 - SMOOTHNESS));

	/* Do the same as delta for the BPM value. The raw bpm formula is a simplified
	version of 
		rawBpm = ((1.0 / m_lastDelta) / MIDI_CLOCK_PPQ) * 60.0;
	where MIDI_CLOCK_PPQ == 24.0 */

	const double rawBpm = 2.5 / m_lastDelta;
	m_lastBpm           = (rawBpm * SMOOTHNESS) + (m_lastBpm * (1.0 - SMOOTHNESS));

	m_lastTimestamp = timestamp;
	m_timeElapsed   = m_timeElapsed + m_lastDelta;

	if (m_timeElapsed > BPM_CHANGE_FREQ)
	{
		c::events::setBpm(m_lastBpm, Thread::MIDI);
		m_timeElapsed = 0;
	}
}

/* -------------------------------------------------------------------------- */

void MidiSynchronizer::computePosition(int sppPosition, int numBeatsInLoop)
{
	/* Each MIDI Beat spans 6 MIDI Clocks. In other words, each MIDI Beat is a 
	16th note (since there are 24 MIDI Clocks in a quarter note).

	So 1 MIDI beat = a 16th note = 6 clock pulses. A quarter (aka a beat) is
	4 MIDI beats. */

	const int beat = (sppPosition / 4) % numBeatsInLoop;

	c::events::goToBeat(beat, Thread::MIDI);
}
} // namespace giada::m