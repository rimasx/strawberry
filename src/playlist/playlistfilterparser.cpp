/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2012, David Sansome <me@davidsansome.com>
 * Copyright 2018-2024, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <QString>

#include "core/logging.h"
#include "filterparser/filterparser.h"
#include "filterparser/filtertree.h"
#include "filterparser/filterparsersearchcomparators.h"
#include "playlistfilterparser.h"
#include "playlist.h"

PlaylistFilterParser::PlaylistFilterParser(const QString &filter_string, const QMap<QString, int> &columns, const QSet<int> &numerical_cols) : FilterParser(filter_string), columns_(columns), numerical_columns_(numerical_cols) {}

FilterTree *PlaylistFilterParser::createSearchTermTreeNode(const QString &column, const QString &prefix, const QString &search) const {

  if (search.isEmpty() && prefix != QLatin1Char('=')) {
    return new NopFilter;
  }

  FilterParserSearchTermComparator *cmp = nullptr;

  // Handle the float based Rating Column
  if (columns_[column] == static_cast<int>(Playlist::Column::Rating)) {
    float parsed_search = ParseSearchRating(search);

    if (prefix == QLatin1Char('=')) {
      cmp = new FilterParserFloatEqComparator(parsed_search);
    }
    else if (prefix == QLatin1String("!=") || prefix == QLatin1String("<>")) {
      cmp = new FilterParserFloatNeComparator(parsed_search);
    }
    else if (prefix == QLatin1Char('>')) {
      cmp = new FilterParserFloatGtComparator(parsed_search);
    }
    else if (prefix == QLatin1String(">=")) {
      cmp = new FilterParserFloatGeComparator(parsed_search);
    }
    else if (prefix == QLatin1Char('<')) {
      cmp = new FilterParserFloatLtComparator(parsed_search);
    }
    else if (prefix == QLatin1String("<=")) {
      cmp = new FilterParserFloatLeComparator(parsed_search);
    }
    else {
      cmp = new FilterParserFloatEqComparator(parsed_search);
    }
  }
  else if (prefix == QLatin1String("!=") || prefix == QLatin1String("<>")) {
    cmp = new FilterParserNeComparator(search);
  }
  else if (!column.isEmpty() && columns_.contains(column) && numerical_columns_.contains(columns_[column])) {
    // The length column contains the time in seconds (nanoseconds, actually - the "nano" part is handled by the DropTailComparatorDecorator,  though).
    int search_value = 0;
    if (columns_[column] == static_cast<int>(Playlist::Column::Length)) {
      search_value = ParseSearchTime(search);
    }
    else {
      search_value = search.toInt();
    }
    // Alright, back to deciding which comparator we'll use
    if (prefix == QLatin1Char('>')) {
      cmp = new FilterParserGtComparator(search_value);
    }
    else if (prefix == QLatin1String(">=")) {
      cmp = new FilterParserGeComparator(search_value);
    }
    else if (prefix == QLatin1Char('<')) {
      cmp = new FilterParserLtComparator(search_value);
    }
    else if (prefix == QLatin1String("<=")) {
      cmp = new FilterParserLeComparator(search_value);
    }
    else {
      // Convert back because for time/rating
      cmp = new FilterParserEqComparator(QString::number(search_value));
    }
  }
  else {
    if (prefix == QLatin1Char('=')) {
      cmp = new FilterParserEqComparator(search);
    }
    else if (prefix == QLatin1Char('>')) {
      cmp = new FilterParserLexicalGtComparator(search);
    }
    else if (prefix == QLatin1String(">=")) {
      cmp = new FilterParserLexicalGeComparator(search);
    }
    else if (prefix == QLatin1Char('<')) {
      cmp = new FilterParserLexicalLtComparator(search);
    }
    else if (prefix == QLatin1String("<=")) {
      cmp = new FilterParserLexicalLeComparator(search);
    }
    else {
      cmp = new FilterParserDefaultComparator(search);
    }
  }

  if (columns_.contains(column)) {
    if (columns_[column] == static_cast<int>(Playlist::Column::Length)) {
      cmp = new FilterParserDropTailComparatorDecorator(cmp);
    }
    return new FilterColumnTerm(columns_[column], cmp);
  }
  else {
    return new FilterTerm(cmp, columns_.values());
  }

}
