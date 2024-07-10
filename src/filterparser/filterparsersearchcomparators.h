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

#ifndef FILTERPARSERSEARCHCOMPARATORS_H
#define FILTERPARSERSEARCHCOMPARATORS_H

#include "config.h"

#include <QString>
#include <QScopedPointer>

class FilterParserSearchTermComparator {
 public:
  FilterParserSearchTermComparator() = default;
  virtual ~FilterParserSearchTermComparator() = default;
  virtual bool Matches(const QString &element) const = 0;
 private:
  Q_DISABLE_COPY(FilterParserSearchTermComparator)
};

// "compares" by checking if the field contains the search term
class FilterParserDefaultComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserDefaultComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.contains(search_term_, Qt::CaseInsensitive);
  }
 private:
  QString search_term_;

  Q_DISABLE_COPY(FilterParserDefaultComparator)
};

class FilterParserEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserEqComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return search_term_ == element;
  }
 private:
  QString search_term_;
};

class FilterParserNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserNeComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return search_term_ != element;
  }
 private:
  QString search_term_;
};

class FilterParserLexicalGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLexicalGtComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element > search_term_;
  }
 private:
  QString search_term_;
};

class FilterParserLexicalGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLexicalGeComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element >= search_term_;
  }
 private:
  QString search_term_;
};

class FilterParserLexicalLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLexicalLtComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element < search_term_;
  }
 private:
  QString search_term_;
};

class FilterParserLexicalLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLexicalLeComparator(const QString &value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element <= search_term_;
  }
 private:
  QString search_term_;
};

// Float Comparators are for the rating
class FilterParserFloatEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatEqComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return search_term_ == element.toFloat();
  }
 private:
  float search_term_;
};

class FilterParserFloatNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatNeComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return search_term_ != element.toFloat();
  }
 private:
  float search_term_;
};

class FilterParserFloatGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatGtComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toFloat() > search_term_;
  }
 private:
  float search_term_;
};

class FilterParserFloatGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatGeComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toFloat() >= search_term_;
  }
 private:
  float search_term_;
};

class FilterParserFloatLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatLtComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toFloat() < search_term_;
  }
 private:
  float search_term_;
};

class FilterParserFloatLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatLeComparator(const float value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toFloat() <= search_term_;
  }
 private:
  float search_term_;
};

class FilterParserGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserGtComparator(const int value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toInt() > search_term_;
  }
 private:
  int search_term_;
};

class FilterParserGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserGeComparator(const int value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toInt() >= search_term_;
  }
 private:
  int search_term_;
};

class FilterParserLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLtComparator(const int value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toInt() < search_term_;
  }
 private:
  int search_term_;
};

class FilterParserLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserLeComparator(const int value) : search_term_(value) {}
  bool Matches(const QString &element) const override {
    return element.toInt() <= search_term_;
  }
 private:
  int search_term_;
};

// The length field of the playlist (entries) contains a song's running time in nanoseconds.
// However, We don't really care about nanoseconds, just seconds.
// Thus, with this decorator we drop the last 9 digits, if that many are present.
class FilterParserDropTailComparatorDecorator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserDropTailComparatorDecorator(FilterParserSearchTermComparator *cmp) : cmp_(cmp) {}

  bool Matches(const QString &element) const override {
    if (element.length() > 9) {
      return cmp_->Matches(element.left(element.length() - 9));
    }
    else {
      return cmp_->Matches(element);
    }
  }
 private:
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
};

class FilterParserRatingComparatorDecorator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserRatingComparatorDecorator(FilterParserSearchTermComparator *cmp) : cmp_(cmp) {}
  bool Matches(const QString &element) const override {
    return cmp_->Matches(QString::number(lround(element.toDouble() * 10.0)));
  }
 private:
  QScopedPointer<FilterParserSearchTermComparator> cmp_;
};

#endif  // FILTERPARSERSEARCHCOMPARATORS_H

