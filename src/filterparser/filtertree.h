/*
 * Strawberry Music Player
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

#ifndef FILTERTREE_H
#define FILTERTREE_H

#include "config.h"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QStringList>

#include "filterparsersearchcomparators.h"
#include "core/song.h"

namespace {
QVariant DataFromField(const QString &field, const Song &metadata) {

  if (field == QLatin1String("albumartist")) return metadata.effective_albumartist();
  if (field == QLatin1String("artist"))      return metadata.artist();
  if (field == QLatin1String("album"))       return metadata.album();
  if (field == QLatin1String("title"))       return metadata.title();
  if (field == QLatin1String("composer"))    return metadata.composer();
  if (field == QLatin1String("performer"))   return metadata.performer();
  if (field == QLatin1String("grouping"))    return metadata.grouping();
  if (field == QLatin1String("genre"))       return metadata.genre();
  if (field == QLatin1String("comment"))     return metadata.comment();
  if (field == QLatin1String("track"))       return metadata.track();
  if (field == QLatin1String("year"))        return metadata.year();
  if (field == QLatin1String("length"))      return metadata.length_nanosec();
  if (field == QLatin1String("samplerate"))  return metadata.samplerate();
  if (field == QLatin1String("bitdepth"))    return metadata.bitdepth();
  if (field == QLatin1String("bitrate"))     return metadata.bitrate();
  if (field == QLatin1String("rating"))      return metadata.rating();
  if (field == QLatin1String("playcount"))   return metadata.playcount();
  if (field == QLatin1String("skipcount"))   return metadata.skipcount();

  return QVariant();

}

} // namespace

class FilterTree {
 public:
  explicit FilterTree();
  virtual ~FilterTree();

  enum class FilterType {
    Nop = 0,
    Or,
    And,
    Not,
    Column,
    Term
  };

  virtual FilterType type() const = 0;

  virtual bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const { Q_UNUSED(row); Q_UNUSED(parent); Q_UNUSED(model); return false; }
  virtual bool accept(const Song &song) const { Q_UNUSED(song); return false; }

 private:
  Q_DISABLE_COPY(FilterTree)
};

// Trivial filter that accepts *anything*
class NopFilter : public FilterTree {
 public:
  FilterType type() const override { return FilterType::Nop; }
  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override { Q_UNUSED(row); Q_UNUSED(parent); Q_UNUSED(model); return true; }
  bool accept(const Song &song) const override { Q_UNUSED(song); return true; }
};

// Filter that applies a SearchTermComparator to all fields of a playlist entry
class FilterTerm : public FilterTree {
 public:
  explicit FilterTerm(FilterParserSearchTermComparator *comparator, const QList<int> &i_columns) : cmp_(comparator), i_columns_(i_columns) {}
  explicit FilterTerm(FilterParserSearchTermComparator *comparator, const QStringList &s_columns) : cmp_(comparator), s_columns_(s_columns) {}

  FilterType type() const override { return FilterType::Term; }

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    for (const int i : i_columns_) {
      const QModelIndex idx = model->index(row, i, parent);
      if (cmp_->Matches(idx.data().toString().toLower())) return true;
    }
    return false;
  }

  bool accept(const Song &song) const override {
    for (const QString &column : s_columns_) {
      if (cmp_->Matches(DataFromField(column, song).toString())) return true;
    }
    return false;
  }

 private:
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
  const QList<int> i_columns_;
  const QStringList s_columns_;
};

// Filter that applies a SearchTermComparator to one specific field of a playlist entry
class FilterColumnTerm : public FilterTree {
 public:
  explicit FilterColumnTerm(const int column, FilterParserSearchTermComparator *comparator) : i_column_(column), cmp_(comparator) {}
  explicit FilterColumnTerm(const QString &column, FilterParserSearchTermComparator *comparator) : i_column_(-1), s_column_(column), cmp_(comparator) {}

  FilterType type() const override { return FilterType::Column; }

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    const QModelIndex idx = model->index(row, i_column_, parent);
    return cmp_->Matches(idx.data().toString().toLower());
  }

  bool accept(const Song &song) const override {
    return cmp_->Matches(DataFromField(s_column_, song).toString());
  }

 private:
  const int i_column_;
  const QString s_column_;
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
};

class NotFilter : public FilterTree {
 public:
  explicit NotFilter(const FilterTree *inv) : child_(inv) {}

  FilterType type() const override { return FilterType::Not; }

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return !child_->accept(row, parent, model);
  }

  bool accept(const Song &song) const override {
    return !child_->accept(song);
  }

 private:
  QScopedPointer<const FilterTree> child_;
};

class OrFilter : public FilterTree {
 public:
  ~OrFilter() override { qDeleteAll(children_); }

  FilterType type() const override { return FilterType::Or; }

  virtual void add(FilterTree *child) { children_.append(child); }

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return std::any_of(children_.begin(), children_.end(), [row, parent, model](FilterTree *child) { return child->accept(row, parent, model); });
  }

  bool accept(const Song &song) const override {
    return std::any_of(children_.begin(), children_.end(), [song](FilterTree *child) { return child->accept(song); });
  }

 private:
  QList<FilterTree*> children_;
};

class AndFilter : public FilterTree {
 public:
  ~AndFilter() override { qDeleteAll(children_); }

  FilterType type() const override { return FilterType::And; }

  virtual void add(FilterTree *child) { children_.append(child); }

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return !std::any_of(children_.begin(), children_.end(), [row, parent, model](FilterTree *child) { return !child->accept(row, parent, model); });
  }

  bool accept(const Song &song) const override {
    return !std::any_of(children_.begin(), children_.end(), [song](FilterTree *child) { return !child->accept(song); });
  }

 private:
  QList<FilterTree*> children_;
};

#endif  // FILTERTREE_H
