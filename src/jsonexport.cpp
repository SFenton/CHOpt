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

#include <map>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "jsonexport.hpp"

namespace {
QJsonArray range_array(
    const std::vector<std::tuple<double, double>>& ranges)
{
    QJsonArray arr;
    for (const auto& [start, end] : ranges) {
        QJsonObject obj;
        obj["startBeat"] = start;
        obj["endBeat"] = end;
        arr.append(obj);
    }
    return arr;
}

QJsonArray double_array(const std::vector<double>& values)
{
    QJsonArray arr;
    for (auto v : values) {
        arr.append(v);
    }
    return arr;
}

QJsonArray int_array(const std::vector<int>& values)
{
    QJsonArray arr;
    for (auto v : values) {
        arr.append(v);
    }
    return arr;
}

const char* fret_name(int index)
{
    // FiveFret / FortniteFestival fret order
    switch (index) {
    case 0: return "green";
    case 1: return "red";
    case 2: return "yellow";
    case 3: return "blue";
    case 4: return "orange";
    case 5: return "open";
    default: return "unknown";
    }
}

const char* difficulty_string(SightRead::Difficulty diff)
{
    switch (diff) {
    case SightRead::Difficulty::Easy: return "easy";
    case SightRead::Difficulty::Medium: return "medium";
    case SightRead::Difficulty::Hard: return "hard";
    case SightRead::Difficulty::Expert: return "expert";
    default: return "unknown";
    }
}
}

std::string export_builder_as_json(const ImageBuilder& builder,
                                   const std::string& path_summary)
{
    QJsonObject root;

    // ── Song metadata ──
    root["songName"] = QString::fromStdString(builder.song_name());
    root["artist"] = QString::fromStdString(builder.artist());
    root["charter"] = QString::fromStdString(builder.charter());
    root["difficulty"] = difficulty_string(builder.difficulty());
    root["totalScore"] = builder.total_score();
    root["pathSummary"] = QString::fromStdString(path_summary);

    // ── Per-measure data ──
    // measure_lines has N+1 entries (boundaries); base/score/sp arrays have N
    const auto& measures = builder.measure_lines();
    const auto& base_values = builder.base_values();
    const auto& score_values = builder.score_values();
    const auto& sp_pct = builder.sp_percent_values();
    const auto& sp_vals = builder.sp_values();

    QJsonArray measures_arr;
    for (std::size_t i = 0; i < base_values.size(); ++i) {
        QJsonObject m;
        m["beatStart"] = measures.at(i);
        if (i + 1 < measures.size()) {
            m["beatEnd"] = measures.at(i + 1);
        }
        m["baseValue"] = base_values.at(i);
        m["cumulativeScore"] = score_values.at(i);
        if (!sp_pct.empty() && i < sp_pct.size()) {
            m["odPercent"] = sp_pct.at(i);
        }
        if (!sp_vals.empty() && i < sp_vals.size()) {
            m["spWhammy"] = sp_vals.at(i);
        }
        measures_arr.append(m);
    }
    root["measures"] = measures_arr;

    // ── Activation ranges (blue = OD active) ──
    // Built below with startNotes embedded

    // ── Squeeze windows (red) ──
    root["squeezeWindows"] = range_array(builder.red_ranges());

    // ── SP phrases (green) ──
    root["spPhrases"] = range_array(builder.green_ranges());

    // ── Compressed whammy zones (yellow) ──
    root["compressedWhammy"] = range_array(builder.yellow_ranges());

    // ── Solo sections ──
    root["soloSections"] = range_array(builder.solo_ranges());

    // ── Unison phrases ──
    root["unisonPhrases"] = range_array(builder.unison_ranges());

    // ── BRE ranges ──
    root["breRanges"] = range_array(builder.bre_ranges());

    // ── Drum fills ──
    root["drumFills"] = range_array(builder.fill_ranges());

    // ── BPMs ──
    {
        QJsonArray bpm_arr;
        for (const auto& [beat, tempo] : builder.bpms()) {
            QJsonObject obj;
            obj["beat"] = beat;
            obj["bpm"] = tempo;
            bpm_arr.append(obj);
        }
        root["bpms"] = bpm_arr;
    }

    // ── Time signatures ──
    {
        QJsonArray ts_arr;
        for (const auto& [beat, num, denom] : builder.time_sigs()) {
            QJsonObject obj;
            obj["beat"] = beat;
            obj["numerator"] = num;
            obj["denominator"] = denom;
            ts_arr.append(obj);
        }
        root["timeSignatures"] = ts_arr;
    }

    // ── Notes ──
    {
        QJsonArray notes_arr;
        for (const auto& note : builder.notes()) {
            QJsonObject obj;
            obj["beat"] = note.beat;
            obj["isSpNote"] = note.is_sp_note;
            QJsonObject frets;
            for (int i = 0; i < 5; ++i) {
                if (note.lengths.at(i) != -1) {
                    frets[fret_name(i)] = note.lengths.at(i);
                }
            }
            // Open notes (index 5)
            if (note.lengths.at(5) != -1) {
                frets["open"] = note.lengths.at(5);
            }
            obj["frets"] = frets;
            notes_arr.append(obj);
        }
        root["notes"] = notes_arr;
    }

    // ── Practice sections ──
    {
        QJsonArray sections_arr;
        for (const auto& [beat, name] : builder.practice_sections()) {
            QJsonObject obj;
            obj["beat"] = beat;
            obj["name"] = QString::fromStdString(name);
            sections_arr.append(obj);
        }
        root["practiceSections"] = sections_arr;
    }

    // ── Per-note score and OD data (activation start notes only) ──
    // Group by activation_index and merge into the activations array
    {
        const auto& note_data = builder.note_score_data();
        // Build a map from activation_index to its start notes
        std::map<int, QJsonArray> act_start_notes;
        for (const auto& nd : note_data) {
            QJsonObject obj;
            obj["beat"] = nd.beat;
            obj["cumulativeScore"] = nd.cumulative_score;
            obj["noteValue"] = nd.note_value;
            obj["odPercent"] = nd.od_percent;
            obj["isSpGranting"] = nd.is_sp_granting;
            act_start_notes[nd.activation_index].append(obj);
        }

        // Rebuild activations array with startNotes embedded
        const auto& blue = builder.blue_ranges();
        QJsonArray acts_arr;
        for (std::size_t i = 0; i < blue.size(); ++i) {
            const auto& [start, end] = blue.at(i);
            QJsonObject obj;
            obj["startBeat"] = start;
            obj["endBeat"] = end;
            auto it = act_start_notes.find(static_cast<int>(i));
            if (it != act_start_notes.end()) {
                obj["startNotes"] = it->second;
            }
            acts_arr.append(obj);
        }
        root["activations"] = acts_arr;
    }

    // ── Measure line positions (raw beat boundaries) ──
    root["measureLines"] = double_array(measures);

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}
