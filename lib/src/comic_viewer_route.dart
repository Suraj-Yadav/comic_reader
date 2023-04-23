import 'dart:async';
import 'dart:io';
import 'dart:math';

import 'package:collection/collection.dart';
import 'package:comic_reader/main.dart';
import 'package:comic_reader/src/comic.dart';
import 'package:comic_reader/src/libarchive/libarchive.dart' as libarchive;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:mime/mime.dart' as mime;
import 'package:path/path.dart' as path;
import 'package:photo_view/photo_view.dart';
import 'package:photo_view/photo_view_gallery.dart';
import 'package:prompt_dialog/prompt_dialog.dart';
import 'package:window_manager/window_manager.dart';

const pageChangeDuration = 500;
const panDuration = 1000;

const debug = false;

const epsilon = 0.000001;

enum Navigation {
  nextComic, // Move to next Comic
  previousComic, // Move to previous Comic
  back, // Go back one route
  nextPage, // Move to next page
  previousPage, // Move to previous page
  switchScroll,
  jumpToPage,
  noOp
}

extension FuzzyCompare on double {
  bool lessThan(double that) {
    return this < (that - max(max(that.abs(), abs()), epsilon) * 0.01);
  }

  bool greaterThan(double that) {
    return this > (that + max(max(that.abs(), abs()), epsilon) * 0.01);
  }

  bool almostEqual(double that) {
    return (this - that).abs() < epsilon;
  }
}

extension VectorOps on Offset {
  double customDot(Offset other) {
    return dx.abs() * other.dx.abs() + dy.abs() * other.dx.abs();
  }
}

class ComicViewerRoute extends StatefulWidget {
  final List<Comic> comics;
  final int index;
  final Comic comic;
  final PageController pageController;

  ComicViewerRoute(this.comics, this.index, {super.key})
      : pageController = PageController(initialPage: 0),
        comic = comics[index];

  @override
  State<StatefulWidget> createState() => _ComicViewerRouteState();
}

extension FuzzyCompareRect on Rect {
  bool almostEqual(dynamic other) {
    if (identical(this, other)) return true;
    if (runtimeType != other.runtimeType) return false;
    return other is Rect &&
        other.left.almostEqual(left) &&
        other.top.almostEqual(top) &&
        other.right.almostEqual(right) &&
        other.bottom.almostEqual(bottom);
  }
}

class _ComicViewerRouteState extends State<ComicViewerRoute>
    with TickerProviderStateMixin {
  int _pageIndex = 0;
  bool _isAnimating = false;
  Animation<Rect?>? _animation;
  AnimationController? _animationController;
  Rect? _viewport;
  double? _scale;
  Timer? _pageChangeTimer;

  bool verticalScroll = false;

  late Future<bool> _loader;

  final List<ComicPage> _pages = [];
  final List<PhotoViewControllerBase> _controllers = [];

  Future<bool> openComic() async {
    final comicCache = Directory(path.join(cacheDirectory.path,
        path.basenameWithoutExtension(widget.comic.archiveFilePath)));

    closeComic();
    double lastProgress = 0;

    logger.d("Create comic page cache folder");
    comicCache.createSync(recursive: true);

    try {
      for (var file
          in libarchive.getArchiveFiles(widget.comic.archiveFilePath)) {
        if (!file.isFile) {
          continue;
        }
        final mimeType =
            mime.lookupMimeType(file.path, headerBytes: file.headerBytes);
        if (mimeType != null && mimeType.startsWith("image/")) {
          _pages.add(savePage(
              path.join(comicCache.path, path.basename(file.path)), file));
        }
        if ((_pages.length / widget.comic.numberOfPages) - lastProgress >
            0.01) {
          setState(() {});
          lastProgress = _pages.length / widget.comic.numberOfPages;
        }
      }
    } catch (e) {
      logger.e("Error while opening comic", e);
    }

    _pages.sort(
        (a, b) => compareAsciiLowerCaseNatural(a.imgFile.path, b.imgFile.path));

    for (var i = 0; i < _pages.length; i++) {
      _controllers.add(PhotoViewController()
        ..outputStateStream.listen((event) {
          viewPortListener(i, event);
        }));
    }

    return true;
  }

  Future<bool> closeComic() async {
    _pages.clear();
    _controllers.clear();
    final comicCache = Directory(path.join(cacheDirectory.path,
        path.basenameWithoutExtension(widget.comic.archiveFilePath)));

    if (comicCache.existsSync()) {
      logger.d("Deleting existing comic page cache folder");
      comicCache.deleteSync(recursive: true);
    }

    return false;
  }

  @override
  void initState() {
    // log('${DateTime.now().millisecondsSinceEpoch}: initState');
    super.initState();
    _loader = openComic();
  }

  @override
  void dispose() {
    for (var controller in _controllers) {
      controller.dispose();
    }
    if (_animationController != null) {
      _animationController!.dispose();
    }
    closeComic();
    super.dispose();
  }

  void viewPortListener(int pageIndex, PhotoViewControllerValue value) {
    // logger.d("Calling viewPortListener");
    // logger.d({"pageIndex": pageIndex});
    if (pageIndex == _pageIndex && !_isAnimating && value.scale != null) {
      final currentImage = _pages[pageIndex];
      final newViewport = computeViewPort(MediaQuery.of(context).size,
          currentImage.size, value.position, value.scale!);
      // logger.d({
      //   "_scale": _scale,
      //   "value.scale": value.scale,
      //   "newViewport": newViewport,
      //   "_viewport": _viewport,
      //   "_scale != value.scale": _scale != value.scale,
      //   "newViewport != _viewport": newViewport != _viewport,
      // });
      if (_scale != value.scale || !newViewport.almostEqual(_viewport)) {
        setState(() {
          _viewport = newViewport;
          _scale = value.scale;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    // log('${DateTime.now().millisecondsSinceEpoch}: build');

    return FutureBuilder(
      future: _loader,
      builder: (context, AsyncSnapshot<bool> snapshot) {
        if (snapshot.hasData) {
          if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
            windowManager.setTitle(
                path.basenameWithoutExtension(widget.comic.archiveFilePath));
            // window_size.setWindowTitle(
            //     path.basenameWithoutExtension(widget.comic.archiveFilePath!));
          }
          if (_viewport != null) {
            final screenSize = MediaQuery.of(context).size;
            final imageSize = _pages[_pageIndex].size;
            final positionAndScale =
                computePositionAndScale(screenSize, imageSize, _viewport!);
            _controllers[_pageIndex].updateMultiple(
                position: positionAndScale.key, scale: positionAndScale.value);
          }

          final List<Widget> stackChildren = [];
          stackChildren.add(PhotoViewGallery(
            scrollDirection: verticalScroll ? Axis.vertical : Axis.horizontal,
            backgroundDecoration:
                const BoxDecoration(color: Colors.transparent),
            scrollPhysics: const BouncingScrollPhysics(),
            onPageChanged: onPageChanged,
            pageController: widget.pageController,
            pageOptions: List.generate(
                _pages.length,
                (index) => PhotoViewGalleryPageOptions(
                    imageProvider: Image.file(_pages[index].imgFile).image,
                    filterQuality: FilterQuality.high,
                    onTapDown: onTapDown,
                    basePosition: Alignment.center,
                    initialScale: _scale,
                    minScale: PhotoViewComputedScale.contained,
                    controller: _controllers[index] as PhotoViewController?)),
          ));
          stackChildren.add(Align(
              alignment: Alignment.bottomCenter,
              child: ClipRRect(
                  borderRadius:
                      const BorderRadius.vertical(top: Radius.circular(10)),
                  child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 5),
                      color: Colors.grey,
                      child: Text(
                        '${_pageIndex + 1}/${_pages.length}',
                        style: const TextStyle(fontSize: 20),
                      )))));
          if (debug) {
            final currentImage = _pages[_pageIndex];
            if (_viewport != null) {
              stackChildren.add(Align(
                  alignment: Alignment.bottomRight,
                  child: Opacity(
                      opacity: 0.5,
                      child: SizedBox(
                          width: 150,
                          height: currentImage.size.height *
                              (150 / currentImage.size.width),
                          child: Stack(children: [
                            Image.file(currentImage.imgFile),
                            Positioned(
                                top: (150 / currentImage.size.width) *
                                    _viewport!.top,
                                left: 150 *
                                    _viewport!.left /
                                    currentImage.size.width,
                                child: Container(
                                  width: 150 *
                                      _viewport!.width /
                                      currentImage.size.width,
                                  height: (150 / currentImage.size.width) *
                                      _viewport!.height,
                                  decoration: BoxDecoration(
                                      border: Border.all(
                                          color: Colors.red, width: 2)),
                                ))
                          ])))));
            }
          }

          SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual,
              overlays: [SystemUiOverlay.bottom]);
          return Shortcuts(
            shortcuts: const <SingleActivator, Intent>{
              SingleActivator(LogicalKeyboardKey.arrowLeft):
                  KeyIntent(Navigation.previousPage),
              SingleActivator(LogicalKeyboardKey.arrowRight):
                  KeyIntent(Navigation.nextPage),
              SingleActivator(LogicalKeyboardKey.escape):
                  KeyIntent(Navigation.back),
              SingleActivator(LogicalKeyboardKey.keyR, control: true):
                  KeyIntent(Navigation.switchScroll),
              SingleActivator(LogicalKeyboardKey.keyG, control: true):
                  KeyIntent(Navigation.jumpToPage),
            },
            child: Actions(
              actions: <Type, Action<Intent>>{
                KeyIntent: CallbackAction<KeyIntent>(
                    onInvoke: (KeyIntent intent) async {
                  if (!_isAnimating &&
                      (intent.direction == Navigation.previousPage ||
                          intent.direction == Navigation.nextPage)) {
                    moveViewport(intent.direction, 0);
                  } else if (intent.direction == Navigation.back) {
                    Navigator.pop(context);
                  } else if (intent.direction == Navigation.switchScroll) {
                    verticalScroll = !verticalScroll;
                  } else if (intent.direction == Navigation.jumpToPage) {
                    String pageNumberStr = (await prompt(
                      context,
                      title: const Text('Enter Page Number'),
                    ))!;
                    moveViewport(intent.direction, int.tryParse(pageNumberStr));
                  }
                  return intent;
                })
              },
              child: Focus(
                  autofocus: true,
                  child: Scaffold(
                      backgroundColor: const Color.fromRGBO(25, 25, 25, 1),
                      body: Stack(children: stackChildren))),
            ),
            // Stack(children: stackChildren))),
          );
        } else {
          return Center(
            child: SizedBox(
              height: 60,
              width: 60,
              child: CircularProgressIndicator(
                value: _pages.length / widget.comic.numberOfPages,
              ),
            ),
          );
        }
      },
    );
  }

  void startPageChangeAnimation(pageIndex) {
    if (widget.pageController.hasClients) {
      widget.pageController.animateToPage(pageIndex,
          duration: const Duration(milliseconds: pageChangeDuration),
          curve: Curves.ease);
      _pageChangeTimer = Timer(
          const Duration(
              milliseconds: pageChangeDuration + pageChangeDuration ~/ 5), () {
        _isAnimating = false;
      });
    }
  }

  void onPageChanged(int index) {
    // log(
    //     '${DateTime.now().millisecondsSinceEpoch}: onPageChanged, index: $index');
    final newImageSize = _pages[index].size;
    if (index > _pageIndex) {
      _viewport = centralizeViewport(
          _viewport!.translate(-_viewport!.left, -_viewport!.top),
          newImageSize);
    } else if (index < _pageIndex) {
      _viewport = centralizeViewport(
          _viewport!.translate(newImageSize.width - _viewport!.right,
              newImageSize.height - _viewport!.bottom),
          newImageSize);
    }

    // This is needed because i haven't figured a way to specify initial location for a new page
    startPanningAnimation(_viewport!, _viewport!.translate(0, 10));
    startPanningAnimation(_viewport!, _viewport!.translate(0, -10));

    if (_pageChangeTimer != null) {
      _pageChangeTimer!.cancel();
      _pageChangeTimer = null;
    }
    setState(() {
      _pageIndex = index;
      _isAnimating = false;
    });
  }

  Rect computeViewPort(
      Size screenSize, Size imageSize, Offset position, double scale) {
    final viewPortCenter = imageSize.center(Offset.zero) + -position / scale;
    return (viewPortCenter - screenSize.center(Offset.zero) / scale) &
        screenSize / scale;
  }

  MapEntry<Offset, double?> computePositionAndScale(
      Size screenSize, Size imageSize, Rect viewPort) {
    final scale = max(
        screenSize.width / viewPort.width, screenSize.height / viewPort.height);
    final position = (imageSize.center(Offset.zero) - viewPort.center) * scale;
    return MapEntry(position, _scale);
  }

  Rect computeNextViewPort(Rect viewport, Navigation direction) {
    final imageSize = _pages[_pageIndex].size;
    switch (direction) {
      case Navigation.nextPage:
        {
          if (viewport.right.lessThan(imageSize.width)) {
            return viewport.translate(
                min(viewport.width / 2, imageSize.width - viewport.right), 0);
          }
          return viewport.translate(-viewport.left,
              min(viewport.height / 2, imageSize.height - viewport.bottom));
        }
      case Navigation.previousPage:
        {
          if (viewport.left.greaterThan(0.0)) {
            return viewport.translate(
                -min(viewport.width / 2, viewport.left), 0);
          }
          return viewport.translate(imageSize.width - viewport.width,
              -min(viewport.height / 2, viewport.top));
        }
      default:
        return viewport;
    }
  }

  Navigation getDirection(Offset tapPoint, Size screenSize) {
    double horizontalTapArea = tapPoint.dx / screenSize.width;
    switch ((3 * horizontalTapArea).floor()) {
      case 0:
        return Navigation.previousPage;
      case 2:
        return Navigation.nextPage;
    }
    return Navigation.noOp;
  }

  void startPanningAnimation(Rect oldViewport, Rect nextViewport) {
    final relativeDistance = (nextViewport.center - oldViewport.center)
            .customDot(oldViewport.size.bottomRight(Offset.zero)) /
        oldViewport.size.bottomRight(Offset.zero).distanceSquared;
    final animationDuration =
        max(min(panDuration * relativeDistance, panDuration), 1).toInt();
    _animationController = AnimationController(
        duration: Duration(milliseconds: animationDuration), vsync: this);
    _animation = RectTween(begin: oldViewport, end: nextViewport).animate(
        CurvedAnimation(parent: _animationController!, curve: Curves.ease))
      ..addListener(animationListener);
    _animationController!.forward();
  }

  void animationListener() {
    if (_animationController == null) {
      return;
    }
    final status = _animationController!.status;
    if (status == AnimationStatus.completed) {
      setState(() {
        _isAnimating = false;
        _viewport = _animation!.value;
        _animation!.removeListener(animationListener);
        _animation = null;
        _animationController!.dispose();
        _animationController = null;
      });
    } else {
      setState(() {
        _viewport = _animation!.value;
        _isAnimating = true;
      });
    }
  }

  Rect centralizeViewport(Rect viewport, Size imageSize) {
    var newViewport = viewport;
    if (newViewport.width > imageSize.width) {
      newViewport = Rect.fromCenter(
          center: Offset(imageSize.width / 2, newViewport.center.dy),
          height: newViewport.height,
          width: newViewport.width);
    }
    if (newViewport.height > imageSize.height) {
      newViewport = Rect.fromCenter(
          center: Offset(newViewport.center.dx, imageSize.height / 2),
          height: newViewport.height,
          width: newViewport.width);
    }
    return newViewport;
  }

  void moveViewport(Navigation direction, int? nextPageNumber) {
    final imageSize = _pages[_pageIndex].size;
    final viewport = _viewport!;
    final gotoNextPage = !viewport.right.lessThan(imageSize.width) &&
        !viewport.bottom.lessThan(imageSize.height);
    final gotoPreviousPage =
        !viewport.left.greaterThan(0.0) && !viewport.top.greaterThan(0.0);

    var nextViewPort = viewport;

    if (direction == Navigation.nextPage && gotoNextPage) {
      if (_pageIndex < (_pages.length - 1)) {
        _isAnimating = true;
        startPageChangeAnimation(_pageIndex + 1);
      }
    } else if (direction == Navigation.previousPage && gotoPreviousPage) {
      if (_pageIndex > 0) {
        _isAnimating = true;
        startPageChangeAnimation(_pageIndex - 1);
      }
    } else if (direction == Navigation.jumpToPage &&
        nextPageNumber != null &&
        (nextPageNumber >= 0 && nextPageNumber < _pages.length)) {
      _isAnimating = true;
      startPageChangeAnimation(nextPageNumber - 1);
    } else if (direction != Navigation.noOp) {
      nextViewPort = centralizeViewport(
          computeNextViewPort(viewport, direction), imageSize);
      startPanningAnimation(viewport, nextViewPort);
    }
  }

  void onTapDown(BuildContext context, TapDownDetails details,
      PhotoViewControllerValue controllerValue) {
    // log('${DateTime.now().millisecondsSinceEpoch}: onTapDown');
    if (_isAnimating) {
      return;
    }

    final screenSize = MediaQuery.of(context).size;
    double horizontalTapArea = details.globalPosition.dx / screenSize.width;

    Navigation direction = Navigation.noOp;
    if (horizontalTapArea <= 0.2) {
      direction = Navigation.previousPage;
    } else if (horizontalTapArea >= 0.8) {
      direction = Navigation.nextPage;
    }
    if (direction == Navigation.noOp) {
      return;
    }
    moveViewport(direction, 0);
  }
}
