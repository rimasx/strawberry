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

#include "config.h"

#include <QString>

#include "core/logging.h"

#include "collectionfilter.h"
#include "collectionfilterparser.h"
#include "collectionmodel.h"
#include "collectionitem.h"

CollectionFilter::CollectionFilter(QObject *parent) : QSortFilterProxyModel(parent), query_hash_(0) {

  setSortLocaleAware(true);
  setDynamicSortFilter(true);
  setRecursiveFilteringEnabled(true);

}

bool CollectionFilter::filterAcceptsRow(const int source_row, const QModelIndex &source_parent) const {

  CollectionModel *model = qobject_cast<CollectionModel*>(sourceModel());
  if (!model) return false;
  const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
  if (!idx.isValid()) return false;
  CollectionItem *item = model->IndexToItem(idx);
  if (!item) return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const QString filter_string = filterRegularExpression().pattern().remove(QLatin1Char('\\'));
#else
  const QString filter_string = filterRegExp().pattern();
#endif

  if (filter_string.isEmpty()) return true;

  if (item->type != CollectionItem::Type::Song) {
    return item->type == CollectionItem::Type::LoadingIndicator;
  }

  size_t hash = qHash(filter_string);
  if (hash != query_hash_) {
    CollectionFilterParser p(filter_string);
    filter_tree_.reset(p.parse());
    query_hash_ = hash;
  }

  return item->type == CollectionItem::Type::Song && item->metadata.is_valid() && filter_tree_->accept(item->metadata);

}
