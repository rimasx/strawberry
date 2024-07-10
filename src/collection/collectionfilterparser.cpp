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

#include "core/logging.h"
#include "core/song.h"
#include "utilities/searchparserutils.h"
#include "filterparser/filterparser.h"
#include "filterparser/filtertree.h"
#include "filterparser/filterparsersearchcomparators.h"
#include "collectionitem.h"
#include "collectionmodel.h"
#include "collectionfilterparser.h"

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

// Filter that applies a SearchTermComparator to all fields of a playlist entry
class CollectionFilterTerm : public CollectionFilterTree {
 public:
  explicit CollectionFilterTerm(FilterParserSearchTermComparator *comparator, const QStringList &columns) : cmp_(comparator), columns_(columns) {}
  bool accept(const Song &song) const override {
    for (const QString &column : columns_) {
      if (cmp_->Matches(DataFromField(column, song).toString())) return true;
    }
    return false;
  }
  FilterType type() override { return FilterType::Term; }
 private:
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
  const QStringList columns_;
};

// Filter that applies a SearchTermComparator to one specific field of a playlist entry
class CollectionFilterColumnTerm : public CollectionFilterTree {
 public:
  CollectionFilterColumnTerm(const QString &column, FilterParserSearchTermComparator *comparator) : column_(column), cmp_(comparator) {}
  bool accept(const Song &song) const override {
    return cmp_->Matches(DataFromField(column_, song).toString());
  }
  FilterType type() override { return FilterType::Column; }
 private:
  const QString column_;
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
};

class CollectionNotFilter : public CollectionFilterTree {
 public:
  explicit CollectionNotFilter(const CollectionFilterTree *inv) : child_(inv) {}
  bool accept(const Song &song) const override {
    return !child_->accept(song);
  }
  FilterType type() override { return FilterType::Not; }
 private:
  QScopedPointer<const CollectionFilterTree> child_;
};

class CollectionOrFilter : public CollectionFilterTree {
 public:
  ~CollectionOrFilter() override { qDeleteAll(children_); }
  virtual void add(CollectionFilterTree *child) { children_.append(child); }
  bool accept(const Song &song) const override {
    return std::any_of(children_.begin(), children_.end(), [song](CollectionFilterTree *child) { return child->accept(song); });
  }
  FilterType type() override { return FilterType::Or; }
 private:
  QList<CollectionFilterTree*> children_;
};

class CollectionAndFilter : public CollectionFilterTree {
 public:
  ~CollectionAndFilter() override { qDeleteAll(children_); }
  virtual void add(CollectionFilterTree *child) { children_.append(child); }
  bool accept(const Song &song) const override {
    return !std::any_of(children_.begin(), children_.end(), [song](CollectionFilterTree *child) { return !child->accept(song); });
  }
  FilterType type() override { return FilterType::And; }
 private:
  QList<CollectionFilterTree*> children_;
};

CollectionFilterParser::CollectionFilterParser(const QString &filter_string) : FilterParser(filter_string) {}

CollectionFilterTree *CollectionFilterParser::parse() {
  iter_ = filter_string_.constBegin();
  end_ = filter_string_.constEnd();
  return parseOrGroup();
}

CollectionFilterTree *CollectionFilterParser::parseOrGroup() {

  advance();
  if (iter_ == end_) return new CollectionNopFilter;

  CollectionOrFilter *group = new CollectionOrFilter;
  group->add(parseAndGroup());
  advance();
  while (checkOr()) {
    group->add(parseAndGroup());
    advance();
  }

  return group;

}

CollectionFilterTree *CollectionFilterParser::parseAndGroup() {

  advance();
  if (iter_ == end_) return new CollectionNopFilter;

  CollectionAndFilter *group = new CollectionAndFilter();
  do {
    group->add(parseSearchExpression());
    advance();
    if (iter_ != end_ && *iter_ == QLatin1Char(')')) break;
    if (checkOr(false)) {
      break;
    }
    checkAnd();  // If there's no 'AND', we'll add the term anyway...
  } while (iter_ != end_);

  return group;

}

CollectionFilterTree *CollectionFilterParser::parseSearchExpression() {

  advance();
  if (iter_ == end_) return new CollectionNopFilter;
  if (*iter_ == QLatin1Char('(')) {
    ++iter_;
    advance();
    CollectionFilterTree *tree = parseOrGroup();
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
    CollectionFilterTree *tree = parseSearchExpression();
    if (tree->type() != FilterTree::FilterType::Nop) return new CollectionNotFilter(tree);
    return tree;
  }
  else {
    return parseSearchTerm();
  }

}

CollectionFilterTree *CollectionFilterParser::parseSearchTerm() {

  QString column;
  QString search;
  QString prefix;
  bool in_quotes = false;
  for (; iter_ != end_; ++iter_) {
    if (in_quotes) {
      if (*iter_ == QLatin1Char('"')) {
        in_quotes = false;
      }
      else {
        buf_ += *iter_;
      }
    }
    else {
      if (*iter_ == QLatin1Char('"')) {
        in_quotes = true;
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

CollectionFilterTree *CollectionFilterParser::createSearchTermTreeNode(const QString &column, const QString &prefix, const QString &search) const {

  if (search.isEmpty() && prefix != QLatin1Char('=')) {
    return new CollectionNopFilter;
  }

  FilterParserSearchTermComparator *cmp = nullptr;

  // Handle the float based Rating Column
  if (column == QLatin1String("rating")) {
    const float parsed_search = Utilities::ParseSearchRating(search);
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
  else if (!column.isEmpty() && Song::kSearchColumns.contains(column) && Song::kNumericalSearchColumns.contains(column)) {
    int search_value = 0;
    if (column == QLatin1String("length")) {
      search_value = Utilities::ParseSearchTime(search);
    }
    else {
      search_value = search.toInt();
    }
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

  if (Song::kSearchColumns.contains(column)) {
    if (column == QLatin1String("length")) {
      cmp = new FilterParserDropTailComparatorDecorator(cmp);
    }
    return new CollectionFilterColumnTerm(column, cmp);
  }
  else {
    return new CollectionFilterTerm(cmp, Song::kSearchColumns);
  }

}
