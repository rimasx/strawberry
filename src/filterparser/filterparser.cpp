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

#include <QString>

#include "filterparser.h"
#include "filterparsersearchcomparators.h"

FilterParser::FilterParser(const QString &filter_string) : filter_string_(filter_string), iter_{}, end_{} {}

void FilterParser::advance() {

  while (iter_ != end_ && iter_->isSpace()) {
    ++iter_;
  }

}

bool FilterParser::checkAnd() {

  if (iter_ != end_) {
    if (*iter_ == QLatin1Char('A')) {
      buf_ += *iter_;
      ++iter_;
      if (iter_ != end_ && *iter_ == QLatin1Char('N')) {
        buf_ += *iter_;
        ++iter_;
        if (iter_ != end_ && *iter_ == QLatin1Char('D')) {
          buf_ += *iter_;
          ++iter_;
          if (iter_ != end_ && (iter_->isSpace() || *iter_ == QLatin1Char('-') || *iter_ == QLatin1Char('('))) {
            advance();
            buf_.clear();
            return true;
          }
        }
      }
    }
  }

  return false;

}

bool FilterParser::checkOr(const bool step_over) {

  if (!buf_.isEmpty()) {
    if (buf_ == QLatin1String("OR")) {
      if (step_over) {
        buf_.clear();
        advance();
      }
      return true;
    }
  }
  else {
    if (iter_ != end_) {
      if (*iter_ == QLatin1Char('O')) {
        buf_ += *iter_;
        ++iter_;
        if (iter_ != end_ && *iter_ == QLatin1Char('R')) {
          buf_ += *iter_;
          ++iter_;
          if (iter_ != end_ && (iter_->isSpace() || *iter_ == QLatin1Char('-') || *iter_ == QLatin1Char('('))) {
            if (step_over) {
              buf_.clear();
              advance();
            }
            return true;
          }
        }
      }
    }
  }

  return false;

}
