import 'dart:io';
import 'dart:math';

import 'package:collection/collection.dart';
import 'package:comic_reader/main.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
import 'package:path_provider/path_provider.dart';

import 'comic.dart';
import 'comic_viewer_route.dart';

class ComicGalleryRoute extends StatefulWidget {
  final List<String> originalFilePaths;

  ComicGalleryRoute(this.originalFilePaths) {
    this.originalFilePaths.sort(compareAsciiLowerCaseNatural);
  }

  @override
  State<StatefulWidget> createState() => _ComicGalleryRouteState();
}

class _ComicGalleryRouteState extends State<ComicGalleryRoute> {
  PageController _landscapeController;
  PageController _potraitController;
  PageController _controller;
  var _currentPageValue = 0.0;

  List<Comic> _comics = [];

  Future<bool> _loader;

  Future<bool> loadGallery() async {
    if (CACHE_DIRECTORY.existsSync()) {
      logger.d("Deleting existing cache folder");
      await CACHE_DIRECTORY.delete(recursive: true);
    }

    logger.d("Create cache folder");
    await CACHE_DIRECTORY.create(recursive: true);

    final comics =
        widget.originalFilePaths.map((e) => Comic(archiveFilePath: e)).toList();

    for (var comic in comics) {
      try {
        await comic.loadComicPreview();
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
                      return false;
                    },
                  ),
                ],
              );
            });
      }
    }

    _comics = comics;

    return true;
  }

  unloadGallery() async {
    _comics.clear();
    final directory = new Directory(
        (await getTemporaryDirectory()).path + "/comicReaderCache");

    if (directory.existsSync()) {
      logger.d("Deleting existing cache folder");
      directory.deleteSync(recursive: true);
    }

    logger.d("Files deleted");
  }

  @override
  void initState() {
    super.initState();
    _landscapeController = PageController(viewportFraction: 0.45);
    _potraitController = PageController(viewportFraction: 1);
    _loader = loadGallery();
    _landscapeController.addListener(() {
      setState(() {
        _currentPageValue = _landscapeController.page;
      });
    });
    _potraitController.addListener(() {
      setState(() {
        _currentPageValue = _potraitController.page;
      });
    });
    _controller = _landscapeController;
  }

  @override
  void dispose() {
    logger.d("Calling dispose");
    _landscapeController.dispose();
    _potraitController.dispose();
    _loader.then((value) {
      unloadGallery();
    });
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    const MAX_PAGE_ROTATION = pi / 2;
    return OrientationBuilder(
      builder: (context, orientation) {
        _controller = orientation == Orientation.landscape
            ? _landscapeController
            : _potraitController;
        return FutureBuilder(
          future: _loader,
          builder: (context, AsyncSnapshot<bool> snapshot) {
            if (snapshot.hasData) {
              return Shortcuts(
                  shortcuts: <LogicalKeySet, Intent>{
                    LogicalKeySet(LogicalKeyboardKey.arrowLeft):
                        KeyIntent(Navigation.PREVIOUS_COMIC),
                    LogicalKeySet(LogicalKeyboardKey.arrowRight):
                        KeyIntent(Navigation.NEXT_COMIC),
                    LogicalKeySet(LogicalKeyboardKey.escape):
                        KeyIntent(Navigation.BACK),
                  },
                  child: Actions(
                    actions: <Type, Action<Intent>>{
                      KeyIntent: CallbackAction<KeyIntent>(
                        onInvoke: (KeyIntent intent) {
                          if (intent.direction == Navigation.PREVIOUS_COMIC) {
                            _controller.previousPage(
                                duration: const Duration(
                                    milliseconds: PAGE_CHANGE_DURATION),
                                curve: Curves.ease);
                          } else if (intent.direction ==
                              Navigation.NEXT_COMIC) {
                            _controller.nextPage(
                                duration: const Duration(
                                    milliseconds: PAGE_CHANGE_DURATION),
                                curve: Curves.ease);
                          } else if (intent.direction == Navigation.BACK) {
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
                              return RaisedButton(
                                autofocus: false,
                                color: Colors.transparent,
                                onPressed: () {
                                  Navigator.push(
                                      context,
                                      MaterialPageRoute(
                                          builder: (context) =>
                                              ComicViewerRoute(
                                                  _comics, position)));
                                },
                                child: Transform(
                                  transform: Matrix4.identity()
                                    ..setEntry(3, 2, 0.001) // perspective
                                    ..rotateY(((_currentPageValue - position)
                                                .isNegative
                                            ? 1
                                            : -1) *
                                        MAX_PAGE_ROTATION *
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
                              path.basenameWithoutExtension(
                                      _comics[_currentPageValue.round()]
                                          .archiveFilePath) +
                                  "\n" +
                                  _comics[_currentPageValue.round()]
                                      .numberOfPages
                                      .toString(),
                              textAlign: TextAlign.center,
                              style:
                                  TextStyle(fontSize: 20, color: Colors.white),
                            ),
                          )
                        ],
                      ),
                    ),
                  ));
            } else {
              return Center(
                child: SizedBox(
                  child: CircularProgressIndicator(),
                  height: 60,
                  width: 60,
                ),
              );
            }
          },
        );
      },
    );
  }
}
