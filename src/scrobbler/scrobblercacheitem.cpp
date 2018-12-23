/*
 * Strawberry Music Player
 * Copyright 2018, Jonas Kvinge <jonas@jkvinge.net>
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

#include <QObject>
#include <QtGlobal>
#include <QString>
#include <QHash>

#include "scrobblercacheitem.h"

ScrobblerCacheItem::ScrobblerCacheItem(const QString &artist, const QString &album, const QString &song, const QString &albumartist, const int track, const qint64 duration, const quint64 &timestamp) :
  artist_(artist),
  album_(album),
  song_(song),
  albumartist_(albumartist),
  track_(track),
  duration_(duration),
  timestamp_(timestamp),
  sent_(false) {}

ScrobblerCacheItem::~ScrobblerCacheItem() {}
