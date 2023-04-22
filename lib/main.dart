import 'dart:io';

import 'package:comic_reader/src/comic_viewer_route.dart';
import 'package:comic_reader/src/file_picker_route.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:logger/logger.dart';
import 'package:path/path.dart' as path;
import 'package:path_provider/path_provider.dart';
import 'package:window_manager/window_manager.dart';

var logger = Logger(
  printer: PrettyPrinter(colors: true, lineLength: 100, printTime: true),
);

late Directory cacheDirectory;

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();

  final shortcut = Map.of(WidgetsApp.defaultShortcuts)
    ..remove(LogicalKeySet(LogicalKeyboardKey.escape))
    ..remove(LogicalKeySet(LogicalKeyboardKey.enter));

  cacheDirectory = Directory(
      path.join((await getTemporaryDirectory()).path, "comicReaderCache"));

  runApp(MaterialApp(
    title: 'Comic Reader',
    theme: ThemeData.dark(),
    darkTheme: ThemeData.dark(),
    home: const FilePickerRoute(),
    shortcuts: shortcut,
  ));
}

class KeyIntent extends Intent {
  const KeyIntent(this.direction);

  final Navigation direction;
}
