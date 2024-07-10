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

#include <algorithm>
#include <cmath>

#include <QList>
#include <QMap>
#include <QSet>
#include <QChar>
#include <QScopedPointer>
#include <QString>
#include <QtAlgorithms>
#include <QAbstractItemModel>

#include "filterparser/filterparsersearchcomparators.h"

#include "playlist.h"
#include "playlistfilterparser.h"
#include "utilities/searchparserutils.h"

// Filter that applies a SearchTermComparator to all fields of a playlist entry
class PlaylistFilterTerm : public PlaylistFilterTree {
 public:
  explicit PlaylistFilterTerm(FilterParserSearchTermComparator *comparator, const QList<int> &columns) : cmp_(comparator), columns_(columns) {}

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    for (const int i : columns_) {
      const QModelIndex idx = model->index(row, i, parent);
      if (cmp_->Matches(idx.data().toString().toLower())) return true;
    }
    return false;
  }
  FilterType type() override { return FilterType::Term; }
 private:
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
  QList<int> columns_;
};

// Filter that applies a SearchTermComparator to one specific field of a playlist entry
class PlaylistFilterColumnTerm : public PlaylistFilterTree {
 public:
  PlaylistFilterColumnTerm(const int column, FilterParserSearchTermComparator *comparator) : col(column), cmp_(comparator) {}

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    const QModelIndex idx = model->index(row, col, parent);
    return cmp_->Matches(idx.data().toString().toLower());
  }
  FilterType type() override { return FilterType::Column; }
 private:
  int col;
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
};

class PlaylistNotFilter : public PlaylistFilterTree {
 public:
  explicit PlaylistNotFilter(const PlaylistFilterTree *inv) : child_(inv) {}

  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return !child_->accept(row, parent, model);
  }
  FilterType type() override { return FilterType::Not; }
 private:
  QScopedPointer<const PlaylistFilterTree> child_;
};

class PlaylistOrFilter : public PlaylistFilterTree {
 public:
  ~PlaylistOrFilter() override { qDeleteAll(children_); }
  virtual void add(PlaylistFilterTree *child) { children_.append(child); }
  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return std::any_of(children_.begin(), children_.end(), [row, parent, model](PlaylistFilterTree *child) { return child->accept(row, parent, model); });
  }
  FilterType type() override { return FilterType::Or; }
 private:
  QList<PlaylistFilterTree*> children_;
};

class PlaylistAndFilter : public PlaylistFilterTree {
 public:
  ~PlaylistAndFilter() override { qDeleteAll(children_); }
  virtual void add(PlaylistFilterTree *child) { children_.append(child); }
  bool accept(const int row, const QModelIndex &parent, const QAbstractItemModel *const model) const override {
    return !std::any_of(children_.begin(), children_.end(), [row, parent, model](PlaylistFilterTree *child) { return !child->accept(row, parent, model); });
  }
  FilterType type() override { return FilterType::And; }
 private:
  QList<PlaylistFilterTree*> children_;
};

PlaylistFilterParser::PlaylistFilterParser(const QString &filter_string, const QMap<QString, int> &columns, const QSet<int> &numerical_cols) : FilterParser(filter_string), columns_(columns), numerical_columns_(numerical_cols) {}

PlaylistFilterTree *PlaylistFilterParser::parse() {
  iter_ = filter_string_.constBegin();
  end_ = filter_string_.constEnd();
  return parseOrGroup();
}

PlaylistFilterTree *PlaylistFilterParser::parseOrGroup() {

  advance();
  if (iter_ == end_) return new PlaylistNopFilter;

  PlaylistOrFilter *group = new PlaylistOrFilter;
  group->add(parseAndGroup());
  advance();
  while (checkOr()) {
    group->add(parseAndGroup());
    advance();
  }

  return group;

}

PlaylistFilterTree *PlaylistFilterParser::parseAndGroup() {

  advance();
  if (iter_ == end_) return new PlaylistNopFilter;

  PlaylistAndFilter *group = new PlaylistAndFilter();
  do {
    group->add(parseSearchExpression());
    advance();
    if (iter_ != end_ && *iter_ == QLatin1Char(')')) break;
    if (checkOr(false)) {
      break;
    }
    checkAnd();  // if there's no 'AND', we'll add the term anyway...
  } while (iter_ != end_);

  return group;

}

PlaylistFilterTree *PlaylistFilterParser::parseSearchExpression() {

  advance();
  if (iter_ == end_) return new PlaylistNopFilter;
  if (*iter_ == QLatin1Char('(')) {
    ++iter_;
    advance();
    PlaylistFilterTree *tree = parseOrGroup();
    advance();
    if (iter_ != end_) {
      if (*iter_ == QLatin1Char(')')) {
        ++iter_;
      }
    }
    return tree;
  }
  else if (*iter_ == QLatin1Char('-')) {
    ++iter_;
    PlaylistFilterTree *tree = parseSearchExpression();
    if (tree->type() != PlaylistFilterTree::FilterType::Nop) return new PlaylistNotFilter(tree);
    return tree;
  }
  else {
    return parseSearchTerm();
  }

}

PlaylistFilterTree *PlaylistFilterParser::parseSearchTerm() {

  QString column;
  QString search;
  QString prefix;
  bool inQuotes = false;
  for (; iter_ != end_; ++iter_) {
    if (inQuotes) {
      if (*iter_ == QLatin1Char('"')) {
        inQuotes = false;
      }
      else {
        buf_ += *iter_;
      }
    }
    else {
      if (*iter_ == QLatin1Char('"')) {
        inQuotes = true;
      }
      else if (column.isEmpty() && *iter_ == QLatin1Char(':')) {
        column = buf_.toLower();
        buf_.clear();
        prefix.clear();  // prefix isn't allowed here - let's ignore it
      }
      else if (iter_->isSpace() || *iter_ == QLatin1Char('(') || *iter_ == QLatin1Char(')') || *iter_ == QLatin1Char('-')) {
        break;
      }
      else if (buf_.isEmpty()) {
        // we don't know whether there is a column part in this search term thus we assume the latter and just try and read a prefix
        if (prefix.isEmpty() && (*iter_ == QLatin1Char('>') || *iter_ == QLatin1Char('<') || *iter_ == QLatin1Char('=') || *iter_ == QLatin1Char('!'))) {
          prefix += *iter_;
        }
        else if (prefix != QLatin1Char('=') && *iter_ == QLatin1Char('=')) {
          prefix += *iter_;
        }
        else {
          buf_ += *iter_;
        }
      }
      else {
        buf_ += *iter_;
      }
    }
  }

  search = buf_.toLower();
  buf_.clear();

  return createSearchTermTreeNode(column, prefix, search);

}

PlaylistFilterTree *PlaylistFilterParser::createSearchTermTreeNode(const QString &column, const QString &prefix, const QString &search) const {

  if (search.isEmpty() && prefix != QLatin1Char('=')) {
    return new PlaylistNopFilter;
  }

  FilterParserSearchTermComparator *cmp = nullptr;

  // Handle the float based Rating Column
  if (columns_[column] == static_cast<int>(Playlist::Column::Rating)) {
    float parsed_search = Utilities::ParseSearchRating(search);

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
      search_value = Utilities::ParseSearchTime(search);
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
    return new PlaylistFilterColumnTerm(columns_[column], cmp);
  }
  else {
    return new PlaylistFilterTerm(cmp, columns_.values());
  }

}
