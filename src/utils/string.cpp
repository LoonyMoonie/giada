/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * utils
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

#include "utils/string.h"
#include <climits>
#include <cstdarg>
#include <cstddef>
#include <iomanip>
#include <memory>

namespace giada::u::string
{
std::string trim(const std::string& s)
{
	std::size_t first = s.find_first_not_of(" \n\t");
	std::size_t last  = s.find_last_not_of(" \n\t");
	return s.substr(first, last - first + 1);
}

/* -------------------------------------------------------------------------- */

std::string replace(std::string in, const std::string& search, const std::string& replace)
{
	std::size_t pos = 0;
	while ((pos = in.find(search, pos)) != std::string::npos)
	{
		in.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return in;
}

/* -------------------------------------------------------------------------- */

bool contains(const std::string& s, char c)
{
	return s.find(c) != std::string::npos;
}

/* -------------------------------------------------------------------------- */

std::string format(const char* format, ...)
{
	va_list args;

	/* Compute the size of the new expanded std::string (i.e. with replacement taken
	into account). */

	va_start(args, format);
	std::size_t size = vsnprintf(nullptr, 0, format, args) + 1;
	va_end(args);

	/* Create a new temporary char array to hold the new expanded std::string. */

	std::unique_ptr<char[]> tmp(new char[size]);

	/* Fill the temporary std::string with the formatted data. */

	va_start(args, format);
	vsprintf(tmp.get(), format, args);
	va_end(args);

	return std::string(tmp.get(), tmp.get() + size - 1);
}

/* -------------------------------------------------------------------------- */

std::vector<std::string> split(std::string in, std::string sep)
{
	std::vector<std::string> out;
	std::string              full  = in;
	std::string              token = "";
	std::size_t              curr  = 0;
	std::size_t              next  = -1;
	do
	{
		curr  = next + 1;
		next  = full.find_first_of(sep, curr);
		token = full.substr(curr, next - curr);
		if (token != "")
			out.push_back(token);
	} while (next != std::string::npos);
	return out;
}

/* -------------------------------------------------------------------------- */

std::string toString(Thread t)
{
	switch (t)
	{
	case Thread::MAIN:
		return "MAIN";
	case Thread::MIDI:
		return "MIDI";
	case Thread::AUDIO:
		return "AUDIO (rt)";
	case Thread::EVENTS:
		return "EVENTS";
	default:
		return "(unknown)";
	}
}

/* -------------------------------------------------------------------------- */

std::string toString(RtAudio::Api api)
{
	switch (api)
	{
	case RtAudio::Api::LINUX_ALSA:
		return "ALSA";
	case RtAudio::Api::LINUX_PULSE:
		return "PulseAudio";
	case RtAudio::Api::UNIX_JACK:
		return "JACK";
	case RtAudio::Api::MACOSX_CORE:
		return "CoreAudio";
	case RtAudio::Api::WINDOWS_WASAPI:
		return "WASAPI";
	case RtAudio::Api::WINDOWS_ASIO:
		return "ASIO";
	case RtAudio::Api::WINDOWS_DS:
		return "DirectSound";
	case RtAudio::Api::RTAUDIO_DUMMY:
		return "Dummy";
	default:
		return "(unknown)";
	}
}

/* -------------------------------------------------------------------------- */

float toFloat(const std::string& s)
{
	try
	{
		return std::stof(s);
	}
	catch (const std::exception&)
	{
		return 0.0f;
	}
}

/* -------------------------------------------------------------------------- */

int toInt(const std::string& s)
{
	try
	{
		return std::stoi(s);
	}
	catch (const std::exception&)
	{
		return 0;
	}
}
} // namespace giada::u::string
