/*
 * CHOpt - Star Power optimiser for Clone Hero
 * Copyright (C) 2026 Raymond Wright, Stephen Fenton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHOPT_JSONEXPORT_HPP
#define CHOPT_JSONEXPORT_HPP

#include <string>

#include "imagebuilder.hpp"
#include <sightread/tempomap.hpp>

// Serialise the ImageBuilder state to a JSON string suitable for machine
// consumption. The output contains per-measure score/OD data, activation
// ranges, note positions, SP phrases, BPMs, and the path summary fields.
// When a TempoMap is provided, beat-based positions also include seconds.
std::string export_builder_as_json(const ImageBuilder& builder,
                                   const std::string& path_summary,
                                   const SightRead::TempoMap* tempo_map = nullptr);

#endif
