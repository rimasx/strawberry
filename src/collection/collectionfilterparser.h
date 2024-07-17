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

class FilterTree;

class CollectionFilterParser : public FilterParser {
 public:
  explicit CollectionFilterParser(const QString &filter_string);
  FilterTree *createSearchTermTreeNode(const QString &column, const QString &prefix, const QString &search) const override;
};

#endif  // COLLECTIONFILTERPARSER_H
