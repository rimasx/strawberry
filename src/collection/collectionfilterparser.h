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

#ifndef COLLECTIONFILTERPARSER_H
#define COLLECTIONFILTERPARSER_H

#include "config.h"

#include <QSet>
#include <QMap>
#include <QString>
#include <QStringList>

#include "filterparser/filterparser.h"
#include "filterparser/filtertree.h"
#include "collectionitem.h"

class CollectionFilterTree : public FilterTree {
 public:
  CollectionFilterTree() = default;
  virtual ~CollectionFilterTree() {}
  virtual bool accept(const Song &song) const = 0;
 private:
  Q_DISABLE_COPY(CollectionFilterTree)
};

// Trivial filter that accepts *anything*
class CollectionNopFilter : public CollectionFilterTree {
 public:
  bool accept(const Song &song) const override { Q_UNUSED(song); return true; }
  FilterType type() override { return FilterType::Nop; }
};

class CollectionFilterParser : public FilterParser {
 public:
  explicit CollectionFilterParser(const QString &filter_string);
  CollectionFilterTree *parse();

 private:
  CollectionFilterTree *parseOrGroup();
  CollectionFilterTree *parseAndGroup();
  CollectionFilterTree *parseSearchExpression();
  CollectionFilterTree *parseSearchTerm();
  CollectionFilterTree *createSearchTermTreeNode(const QString &column, const QString &prefix, const QString &search) const;
};

#endif  // COLLECTIONFILTERPARSER_H
