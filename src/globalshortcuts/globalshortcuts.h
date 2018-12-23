/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
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

#ifndef GLOBALSHORTCUTS_H
#define GLOBALSHORTCUTS_H

#include "config.h"

#include <stdbool.h>

#include <QObject>
#include <QWidget>
#include <QMap>
#include <QString>
#include <QAction>
#include <QShortcut>
#include <QSignalMapper>
#include <QKeySequence>
#include <QSettings>

class GlobalShortcutBackend;

class GlobalShortcuts : public QWidget {
  Q_OBJECT

 public:
  explicit GlobalShortcuts(QWidget *parent = nullptr);

  struct Shortcut {
    QString id;
    QKeySequence default_key;
    QAction *action;
    QShortcut *shortcut;
  };

  QMap<QString, Shortcut> shortcuts() const { return shortcuts_; }
  bool IsGsdAvailable() const;
  bool IsMacAccessibilityEnabled() const;

 public slots:
  void ReloadSettings();
  void ShowMacAccessibilityDialog();

  void Unregister();
  void Register();

signals:
  void Play();
  void Pause();
  void PlayPause();
  void Stop();
  void StopAfter();
  void Next();
  void Previous();
  void IncVolume();
  void DecVolume();
  void Mute();
  void SeekForward();
  void SeekBackward();
  void ShowHide();
  void ShowOSD();
  void TogglePrettyOSD();
  void CycleShuffleMode();
  void CycleRepeatMode();
  void RemoveCurrentSong();
  void ToggleScrobbling();

 private:
  void AddShortcut(const QString &id, const QString &name, const char *signal, const QKeySequence &default_key = QKeySequence(0));
  Shortcut AddShortcut(const QString &id, const QString &name, const QKeySequence &default_key);

 private:
  GlobalShortcutBackend *gnome_backend_;
  GlobalShortcutBackend *system_backend_;

  QMap<QString, Shortcut> shortcuts_;
  QSettings settings_;

  bool use_gnome_;
  QSignalMapper *rating_signals_mapper_;
};

#endif

