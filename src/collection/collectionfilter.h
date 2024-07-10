/*
 * Strawberry Music Player
 * Copyright 2021-2024, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef COLLECTIONFILTER_H
#define COLLECTIONFILTER_H

#include "config.h"

#include <QtGlobal>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QString>
#include <QStringList>

#include "core/song.h"
#include "collectionfilterparser.h"

class CollectionItem;

class CollectionFilter : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit CollectionFilter(QObject *parent = nullptr);

 protected:
  bool filterAcceptsRow(const int source_row, const QModelIndex &source_parent) const override;

 private:
  mutable QScopedPointer<CollectionFilterTree> filter_tree_;
  mutable size_t query_hash_;
};

#endif  // COLLECTIONFILTER_H
