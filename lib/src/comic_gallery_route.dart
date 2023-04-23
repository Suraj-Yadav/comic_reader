import 'dart:math';

import 'package:collection/collection.dart';
import 'package:comic_reader/main.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;

import 'comic.dart';
import 'comic_viewer_route.dart';

class ComicGalleryRoute extends StatefulWidget {
  final List<String> originalFilePaths;

  ComicGalleryRoute(this.originalFilePaths, {super.key}) {
    originalFilePaths.sort(compareAsciiLowerCaseNatural);
  }

  @override
  State<StatefulWidget> createState() => _ComicGalleryRouteState();
}

class _ComicGalleryRouteState extends State<ComicGalleryRoute> {
  late PageController _landscapeController;
  late PageController _portraitController;
  late PageController _controller;
  late double _currentPageValue = 0.0;

  final List<Comic> _comics = [];

  bool loaded = false;

  Future<bool> loadGallery() {
    if (cacheDirectory.existsSync()) {
      logger.d("Deleting existing cache folder");
      cacheDirectory.deleteSync(recursive: true);
    }

    logger.d("Create cache folder");
    cacheDirectory.createSync(recursive: true);

    final comics =
        widget.originalFilePaths.map((e) => Comic(archiveFilePath: e)).toList();

    return Future(() async {
      for (var comic in comics) {
        try {
          comic.loadComicPreview();
          _comics.add(comic);
        } catch (e) {
          await showDialog(
              context: context,
              builder: (context) {
                return AlertDialog(
                  title: const Text('Unable to open comic'),
                  content: SingleChildScrollView(
                    child: ListBody(
                      children: <Widget>[
                        Text(comic.archiveFilePath),
                        Text(e.toString()),
                      ],
                    ),
                  ),
                  actions: <Widget>[
                    TextButton(
                      child: const Text('Ok'),
                      onPressed: () {
                        Navigator.of(context).pop();
                      },
                    ),
                  ],
                );
              });
        }
      }
      if (_comics.isEmpty) {
        if (context.mounted) {
          await showDialog(
              context: context,
              builder: (context) {
                return AlertDialog(
                  title: const Text('No valid files were found'),
                  actions: <Widget>[
                    TextButton(
                      child: const Text('Ok'),
                      onPressed: () {
                        Navigator.of(context).pop();
                      },
                    ),
                  ],
                );
              });
        }
      }
      return _comics.isNotEmpty;
    });
  }

  unloadGallery() async {
    _comics.clear();

    if (cacheDirectory.existsSync()) {
      logger.d("Deleting existing cache folder");
      cacheDirectory.deleteSync(recursive: true);
    }

    logger.d("Files deleted");
  }

  @override
  void initState() {
    super.initState();
    _landscapeController = PageController(viewportFraction: 0.45);
    _portraitController = PageController(viewportFraction: 1);
    _landscapeController.addListener(() {
      setState(() {
        _currentPageValue = _landscapeController.page!;
      });
    });
    _portraitController.addListener(() {
      setState(() {
        _currentPageValue = _portraitController.page!;
      });
    });
    _controller = _landscapeController;
    loadGallery().then((success) {
      setState(() {
        loaded = success;
      });
      if (!success) {
        Navigator.pop(context);
      }
    });
  }

  @override
  void dispose() {
    logger.d("Calling dispose");
    _landscapeController.dispose();
    _portraitController.dispose();
    unloadGallery();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    const maxPageRotation = pi / 2;
    return OrientationBuilder(
      builder: (context, orientation) {
        _controller = orientation == Orientation.landscape
            ? _landscapeController
            : _portraitController;
        if (loaded) {
          return Shortcuts(
              shortcuts: <LogicalKeySet, Intent>{
                LogicalKeySet(LogicalKeyboardKey.arrowLeft):
                    const KeyIntent(Navigation.previousComic),
                LogicalKeySet(LogicalKeyboardKey.arrowRight):
                    const KeyIntent(Navigation.nextComic),
                LogicalKeySet(LogicalKeyboardKey.escape):
                    const KeyIntent(Navigation.back),
              },
              child: Actions(
                actions: <Type, Action<Intent>>{
                  KeyIntent: CallbackAction<KeyIntent>(
                    onInvoke: (KeyIntent intent) {
                      if (intent.direction == Navigation.previousComic) {
                        _controller.previousPage(
                            duration: const Duration(
                                milliseconds: pageChangeDuration),
                            curve: Curves.ease);
                      } else if (intent.direction == Navigation.nextComic) {
                        _controller.nextPage(
                            duration: const Duration(
                                milliseconds: pageChangeDuration),
                            curve: Curves.ease);
                      } else if (intent.direction == Navigation.back) {
                        Navigator.pop(context);
                      }
                      return intent;
                    },
                  )
                },
                child: Scaffold(
                  backgroundColor: Colors.black,
                  body: Stack(
                    children: [
                      PageView.builder(
                        // physics: ClampingScrollPhysics(),
                        itemBuilder: (context, position) {
                          return ElevatedButton(
                            autofocus: false,
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.transparent,
                            ),
                            onPressed: () {
                              Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                      builder: (context) =>
                                          ComicViewerRoute(_comics, position)));
                            },
                            child: Transform(
                              transform: Matrix4.identity()
                                ..setEntry(3, 2, 0.001) // perspective
                                ..rotateY(
                                    ((_currentPageValue - position).isNegative
                                            ? 1
                                            : -1) *
                                        maxPageRotation *
                                        (1 -
                                            exp(-1 *
                                                (_currentPageValue - position)
                                                    .abs()))),
                              alignment: FractionalOffset.center,
                              child: FractionallySizedBox(
                                  heightFactor: 0.7,
                                  child: Image.file(
                                      _comics[position].firstPage.imgFile)),
                            ),
                          );
                        },
                        itemCount: _comics.length,
                        controller: _controller,
                      ),
                      Align(
                        alignment: Alignment.bottomCenter,
                        child: Text(
                          [
                            path.basenameWithoutExtension(
                                _comics[_currentPageValue.round()]
                                    .archiveFilePath),
                            _comics[_currentPageValue.round()]
                                .numberOfPages
                                .toString()
                          ].join("\n"),
                          textAlign: TextAlign.center,
                          style: const TextStyle(
                              fontSize: 20, color: Colors.white),
                        ),
                      )
                    ],
                  ),
                ),
              ));
        } else {
          return const Center(
            child: SizedBox(
              height: 60,
              width: 60,
              child: CircularProgressIndicator(),
            ),
          );
        }
      },
    );
  }
}
