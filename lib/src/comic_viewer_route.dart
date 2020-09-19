import 'dart:async';
import 'dart:io';
import 'dart:math';
import 'dart:ui';

import 'package:comic_reader/src/comic.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
import 'package:photo_view/photo_view.dart';
import 'package:photo_view/photo_view_gallery.dart';
import 'package:window_size/window_size.dart' as window_size;

const PAGE_CHANGE_DURATION = 500;
const PAN_DURATION = 1000;

const DEBUG = false;

const EPSILON = 0.000001;

enum Direction { NEXT, PREVIOUS, STAY }

extension FuzzyCompare on double {
  bool lessThan(double that) {
    return this < (that - max(max(that.abs(), this.abs()), EPSILON) * 0.01);
  }

  bool greaterThan(double that) {
    return this > (that + max(max(that.abs(), this.abs()), EPSILON) * 0.01);
  }

  bool almostEqual(double that) {
    return (this - that).abs() < EPSILON;
  }
}

extension VectorOps on Offset {
  double customDot(Offset other) {
    return this.dx.abs() * other.dx.abs() + this.dy.abs() * other.dx.abs();
  }
}

class ComicViewerRoute extends StatefulWidget {
  final List<Comic> comics;
  final int index;
  final Comic comic;
  final PageController pageController;

  ComicViewerRoute(this.comics, this.index)
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
        other.left.almostEqual(this.left) &&
        other.top.almostEqual(this.top) &&
        other.right.almostEqual(this.right) &&
        other.bottom.almostEqual(this.bottom);
  }
}

class _ComicViewerRouteState extends State<ComicViewerRoute>
    with TickerProviderStateMixin {
  int _pageIndex = 0;
  bool _isAnimating = false;
  Animation<Rect> _animation;
  AnimationController _animationController;
  Rect _viewport;
  double _scale;
  Timer _pageChangeTimer;

  List<PhotoViewControllerBase> _controllers;

  @override
  void initState() {
    // log('${DateTime.now().millisecondsSinceEpoch}: initState');
    widget.comic.loadPages();
    _controllers = [];
    for (var i = 0; i < widget.comic.pages.length; i++) {
      _controllers.add(PhotoViewController()
        ..outputStateStream.listen((PhotoViewControllerValue value) {
          viewPortListener(i, value);
        }));
    }
    super.initState();
  }

  @override
  void dispose() {
    widget.comic.cleanPages();
    // log('${DateTime.now().millisecondsSinceEpoch}: dispose');
    _controllers.forEach((controller) => controller.dispose());
    if (_animationController != null) {
      _animationController.dispose();
    }
    super.dispose();
  }

  void viewPortListener(int pageIndex, PhotoViewControllerValue value) {
    // print('${DateTime.now().millisecondsSinceEpoch}: viewPortListener');
    if (pageIndex == _pageIndex &&
        !_isAnimating &&
        value.position != null &&
        value.scale != null) {
      final currentImage = widget.comic.pages[pageIndex];
      if (currentImage.size != null) {
        final newViewport = computeViewPort(MediaQuery.of(context).size,
            currentImage.size, value.position, value.scale);
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n _scale: ${_scale}\n\n');
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n value.scale: ${value.scale}\n\n');
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n newViewport: ${newViewport}\n\n');
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n _viewport: ${_viewport}\n\n');
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n _scale != value.scale: ${_scale != value.scale}\n\n');
        // print('\n' +
        //     StackTrace.current.toString().split('\n')[0] +
        //     '\n newViewport != _viewport: ${newViewport != _viewport}\n\n');
        if (_scale != value.scale || !newViewport.almostEqual(_viewport)) {
          setState(() {
            _viewport = newViewport;
            _scale = value.scale;
          });
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    // log('${DateTime.now().millisecondsSinceEpoch}: build');
    if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
      window_size
          .setWindowTitle(path.basenameWithoutExtension(widget.comic.filePath));
    }
    if (_viewport != null) {
      final screenSize = MediaQuery.of(context).size;
      final imageSize = widget.comic.pages[_pageIndex].size;
      final positionAndScale =
          computePositionAndScale(screenSize, imageSize, _viewport);
      _controllers[_pageIndex].updateMultiple(
          position: positionAndScale.key, scale: positionAndScale.value);
    }

    final List<Widget> stackChilds = [];
    stackChilds.add(PhotoViewGallery(
      backgroundDecoration: BoxDecoration(color: Colors.transparent),
      scrollPhysics: BouncingScrollPhysics(),
      onPageChanged: onPageChanged,
      pageController: widget.pageController,
      pageOptions: List.generate(
          widget.comic.pages.length,
          (index) => PhotoViewGalleryPageOptions(
              imageProvider: widget.comic.pages[index].page.image,
              filterQuality: FilterQuality.high,
              onTapDown: onTapDown,
              basePosition: Alignment.center,
              initialScale: _scale,
              minScale: PhotoViewComputedScale.contained,
              controller: _controllers[index])),
    ));
    stackChilds.add(Align(
        alignment: Alignment.bottomCenter,
        child: ClipRRect(
            borderRadius: BorderRadius.vertical(top: const Radius.circular(10)),
            child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 5),
                color: Colors.grey,
                child: Text(
                  (_pageIndex + 1).toString() +
                      '/' +
                      widget.comic.pages.length.toString(),
                  style: TextStyle(fontSize: 20),
                )))));
    if (DEBUG) {
      final currentImage = widget.comic.pages[_pageIndex];
      if (currentImage.size != null && _viewport != null) {
        stackChilds.add(Align(
            alignment: Alignment.bottomRight,
            child: Opacity(
                opacity: 0.5,
                child: Container(
                    width: 150,
                    height: currentImage.size.height *
                        (150 / currentImage.size.width),
                    child: Stack(children: [
                      currentImage.page,
                      Positioned(
                          top: (150 / currentImage.size.width) * _viewport.top,
                          left: 150 * _viewport.left / currentImage.size.width,
                          child: Container(
                            width:
                                150 * _viewport.width / currentImage.size.width,
                            height: (150 / currentImage.size.width) *
                                _viewport.height,
                            decoration: BoxDecoration(
                                border:
                                    Border.all(color: Colors.red, width: 2)),
                          ))
                    ])))));
      }
    }

    SystemChrome.setEnabledSystemUIOverlays([SystemUiOverlay.bottom]);
    return WillPopScope(
        onWillPop: () async {
          SystemChrome.setEnabledSystemUIOverlays(SystemUiOverlay.values);
          return true;
        },
        child: Scaffold(
            backgroundColor: Color.fromRGBO(25, 25, 25, 1),
            body: RawKeyboardListener(
                autofocus: true,
                focusNode: FocusNode(),
                onKey: onKeyChange,
                child: Stack(children: stackChilds))));
  }

  void startPageChangeAnimation(pageIndex) {
    widget.pageController.animateToPage(pageIndex,
        duration: const Duration(milliseconds: PAGE_CHANGE_DURATION),
        curve: Curves.ease);
    _pageChangeTimer = Timer(
        const Duration(
            milliseconds: PAGE_CHANGE_DURATION + PAGE_CHANGE_DURATION ~/ 5),
        () {
      _isAnimating = false;
    });
  }

  void onPageChanged(int index) {
    // log(
    //     '${DateTime.now().millisecondsSinceEpoch}: onPageChanged, index: $index');
    final newImageSize = widget.comic.pages[index].size;
    if (index > _pageIndex) {
      _viewport = centralizeViewport(
          _viewport.translate(-_viewport.left, -_viewport.top), newImageSize);
    } else if (index < _pageIndex) {
      _viewport = centralizeViewport(
          _viewport.translate(newImageSize.width - _viewport.right,
              newImageSize.height - _viewport.bottom),
          newImageSize);
    }

    // This is needed beacuse i haven't figured a way to specify initial location for a new page
    // startPanningAnimation(_viewport, _viewport.translate(0, 10));
    // startPanningAnimation(_viewport, _viewport.translate(0, -10));

    if (_pageChangeTimer != null) {
      _pageChangeTimer.cancel();
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

  MapEntry<Offset, double> computePositionAndScale(
      Size screenSize, Size imageSize, Rect viewPort) {
    final scale = max(
        screenSize.width / viewPort.width, screenSize.height / viewPort.height);
    final position = (imageSize.center(Offset.zero) - viewPort.center) * scale;
    return MapEntry(position, _scale);
  }

  Rect computeNextViewPort(Rect viewport, Direction direction) {
    final imageSize = widget.comic.pages[_pageIndex].size;
    switch (direction) {
      case Direction.NEXT:
        {
          if (viewport.right.lessThan(imageSize.width)) {
            return viewport.translate(
                min(viewport.width / 2, imageSize.width - viewport.right), 0);
          }
          return viewport.translate(-viewport.left,
              min(viewport.height / 2, imageSize.height - viewport.bottom));
        }
      case Direction.PREVIOUS:
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

  Direction getDirection(Offset tapPoint, Size screenSize) {
    double horizontalTapArea = tapPoint.dx / screenSize.width;
    switch ((3 * horizontalTapArea).floor()) {
      case 0:
        return Direction.PREVIOUS;
      case 2:
        return Direction.NEXT;
    }
    return Direction.STAY;
  }

  void startPanningAnimation(Rect oldViewport, Rect nextViewport) {
    final relativeDistance = (nextViewport.center - oldViewport.center)
            .customDot(oldViewport.size.bottomRight(Offset.zero)) /
        oldViewport.size.bottomRight(Offset.zero).distanceSquared;
    final animationDuration =
        max(min(PAN_DURATION * relativeDistance, PAN_DURATION), 1).toInt();
    _animationController = AnimationController(
        duration: Duration(milliseconds: animationDuration), vsync: this);
    _animation = RectTween(begin: oldViewport, end: nextViewport).animate(
        CurvedAnimation(parent: _animationController, curve: Curves.ease))
      ..addListener(animationListener);
    _animationController.forward();
  }

  void animationListener() {
    if (_animationController == null) {
      return;
    }
    final status = _animationController.status;
    if (status == AnimationStatus.completed) {
      setState(() {
        _isAnimating = false;
        _viewport = _animation.value;
        _animation.removeListener(animationListener);
        _animation = null;
        _animationController.dispose();
        _animationController = null;
      });
    } else {
      setState(() {
        _viewport = _animation.value;
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

  void moveViewport(Direction direction) {
    final imageSize = widget.comic.pages[_pageIndex].size;
    final viewport = _viewport;
    final gotoNextPage = !viewport.right.lessThan(imageSize.width) &&
        !viewport.bottom.lessThan(imageSize.height);
    final gotoPreviousPage =
        !viewport.left.greaterThan(0.0) && !viewport.top.greaterThan(0.0);

    var nextViewPort = viewport;

    if (direction == Direction.NEXT && gotoNextPage) {
      if (_pageIndex < (widget.comic.pages.length - 1)) {
        _isAnimating = true;
        startPageChangeAnimation(_pageIndex + 1);
      }
    } else if (direction == Direction.PREVIOUS && gotoPreviousPage) {
      if (_pageIndex > 0) {
        _isAnimating = true;
        startPageChangeAnimation(_pageIndex - 1);
      }
    } else if (direction != Direction.STAY) {
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

    Direction direction = Direction.STAY;
    if (horizontalTapArea <= 0.2) {
      direction = Direction.PREVIOUS;
    } else if (horizontalTapArea >= 0.8) {
      direction = Direction.NEXT;
    }
    if (direction == Direction.STAY) {
      return;
    }
    moveViewport(direction);
  }

  void onKeyChange(RawKeyEvent value) {
    if (!_isAnimating && value is RawKeyDownEvent) {
      if (value.data.logicalKey == LogicalKeyboardKey.escape) {
        Navigator.pop(context);
      } else if (value.data.logicalKey == LogicalKeyboardKey.arrowRight) {
        moveViewport(Direction.NEXT);
      } else if (value.data.logicalKey == LogicalKeyboardKey.arrowLeft) {
        moveViewport(Direction.PREVIOUS);
      }
    }
  }
}
