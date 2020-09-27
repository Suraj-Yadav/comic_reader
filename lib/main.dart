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

Iterable<List<T>> zip<T>(Iterable<Iterable<T>> iterables) sync* {
  if (iterables.isEmpty) return;
  final iterators = iterables.map((e) => e.iterator).toList(growable: false);
  while (iterators.every((e) => e.moveNext())) {
    yield iterators.map((e) => e.current).toList(growable: false);
  }
}

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
    // home: FirstRoute(),
    shortcuts: shortcut,
  ));
}

class FirstRoute extends StatelessWidget {
  @override
  void dispose() {
    print(
        '\n' + StackTrace.current.toString().split('\n')[0] + '\n 1: ${1}\n\n');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('First Route'),
      ),
      body: Center(
        child: RaisedButton(
          child: Text('Open route'),
          onPressed: () {
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => MiddleRoute()),
            );
          },
        ),
      ),
    );
  }
}

class KeyIntent extends Intent {
  const KeyIntent(this.direction);

  final Navigation direction;
}

class MiddleRoute extends StatelessWidget {
  @override
  void dispose() {
    print(
        '\n' + StackTrace.current.toString().split('\n')[0] + '\n 1: ${1}\n\n');
  }

  @override
  Widget build(BuildContext context) {
    return Shortcuts(
        shortcuts: <LogicalKeySet, Intent>{
          LogicalKeySet(LogicalKeyboardKey.arrowLeft):
              KeyIntent(Navigation.PREVIOUS_PAGE),
          LogicalKeySet(LogicalKeyboardKey.arrowRight):
              KeyIntent(Navigation.NEXT_PAGE),
          LogicalKeySet(LogicalKeyboardKey.arrowUp): KeyIntent(Navigation.NO_OP)
        },
        child: Actions(
            actions: <Type, Action<Intent>>{
              KeyIntent:
                  CallbackAction<KeyIntent>(onInvoke: (KeyIntent intent) {
                print('\n' +
                    StackTrace.current.toString().split('\n')[0] +
                    '\n intent: ${intent.direction}\n\n');
                if (intent.direction == Navigation.NO_OP) {
                  Navigator.pop(context);
                }
              })
            },
            child: Focus(
                autofocus: true,
                child: Scaffold(
                  appBar: AppBar(
                    title: Text('Middle Route'),
                  ),
                  body: Center(
                    child: RaisedButton(
                      child: Text('Open route'),
                      onPressed: () {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => SecondRoute()),
                        );
                      },
                    ),
                  ),
                ))));
  }
}

class SecondRoute extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    final pageController = PageController();

    final pageView = PageView.builder(
      controller: pageController,
      itemBuilder: (context, position) {
        return Container(
          color: position % 2 == 0 ? Colors.pink : Colors.cyan,
        );
      },
    );

    return Shortcuts(
        shortcuts: <LogicalKeySet, Intent>{
          LogicalKeySet(LogicalKeyboardKey.arrowLeft):
              KeyIntent(Navigation.PREVIOUS_PAGE),
          LogicalKeySet(LogicalKeyboardKey.arrowRight):
              KeyIntent(Navigation.NEXT_PAGE),
          LogicalKeySet(LogicalKeyboardKey.arrowUp): KeyIntent(Navigation.NO_OP)
        },
        child: Actions(
            actions: <Type, Action<Intent>>{
              KeyIntent:
                  CallbackAction<KeyIntent>(onInvoke: (KeyIntent intent) {
                print('\n' +
                    StackTrace.current.toString().split('\n')[0] +
                    '\n intent: ${intent.direction}\n\n');
                if (intent.direction == Navigation.PREVIOUS_PAGE) {
                  pageController.previousPage(
                      duration:
                          const Duration(milliseconds: PAGE_CHANGE_DURATION),
                      curve: Curves.ease);
                } else if (intent.direction == Navigation.NEXT_PAGE) {
                  pageController.nextPage(
                      duration:
                          const Duration(milliseconds: PAGE_CHANGE_DURATION),
                      curve: Curves.ease);
                } else if (intent.direction == Navigation.NO_OP) {
                  Navigator.pop(context);
                }
              })
            },
            child: Focus(
                autofocus: true,
                child: Scaffold(
                    appBar: AppBar(
                      title: Text("Second Route"),
                    ),
                    floatingActionButton: FloatingActionButton(
                      onPressed: () {
                        Navigator.pop(context);
                      },
                      child: Icon(Icons.navigation),
                    ),
                    body: Stack(
                      children: [pageView],
                    )))));
  }
}
