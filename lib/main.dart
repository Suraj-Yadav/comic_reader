import 'dart:io';

import 'package:comic_reader/src/comic_viewer_route.dart';
import 'package:comic_reader/src/file_picker_route.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:logger/logger.dart';
import 'package:path_provider/path_provider.dart';

var logger = Logger(
  printer: PrettyPrinter(colors: true, lineLength: 100, printTime: true),
);

Directory CACHE_DIRECTORY;

void main() async {
  final shortcut = Map.of(WidgetsApp.defaultShortcuts)
    ..remove(LogicalKeySet(LogicalKeyboardKey.escape))
    ..remove(LogicalKeySet(LogicalKeyboardKey.enter));

  CACHE_DIRECTORY =
      new Directory((await getTemporaryDirectory()).path + "/comicReaderCache");

  runApp(MaterialApp(
    title: 'Comic Reader',
    theme: ThemeData.dark(),
    darkTheme: ThemeData.dark(),
    home: FilePickerRoute(),
    shortcuts: shortcut,
  ));
}

class KeyIntent extends Intent {
  const KeyIntent(this.direction);

  final Navigation direction;
}
