import 'dart:math';
import 'package:path/path.dart' as path;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'comic.dart';
import 'comic_viewer_route.dart';

class ComicGalleryRoute extends StatefulWidget {
  final List<Comic> _comics;

  ComicGalleryRoute(this._comics);

  @override
  State<StatefulWidget> createState() => _ComicGalleryRouteState();
}

class _ComicGalleryRouteState extends State<ComicGalleryRoute> {
  PageController _landscapeController;
  PageController _potraitController;
  PageController _controller;
  var _currentPageValue = 0.0;
  @override
  void initState() {
    _landscapeController = PageController(viewportFraction: 0.45);
    _potraitController = PageController(viewportFraction: 1);
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
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    const MAX_PAGE_ROTATION = pi / 4;
    return OrientationBuilder(
      builder: (context, orientation) {
        _controller = orientation == Orientation.landscape
            ? _landscapeController
            : _potraitController;
        return Scaffold(
          backgroundColor: Colors.transparent,
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
                                  ComicViewerRoute(widget._comics, position)));
                    },
                    child: Transform(
                      transform: Matrix4.identity()
                        ..setEntry(3, 2, 0.001) // perspective
                        ..rotateY(((_currentPageValue - position).isNegative
                                ? 1
                                : -1) *
                            MAX_PAGE_ROTATION *
                            (1 -
                                exp(-10 *
                                    (_currentPageValue - position).abs()))),
                      alignment: FractionalOffset.center,
                      child: FractionallySizedBox(
                          heightFactor: 0.7,
                          child: widget._comics[position].firstPage.page),
                    ),
                  );
                },
                itemCount: widget._comics.length,
                controller: _controller,
              ),
              Align(
                alignment: Alignment.bottomCenter,
                child: Text(
                  path.basenameWithoutExtension(
                      widget._comics[_currentPageValue.round()].filePath),
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 20, color: Colors.white),
                ),
              )
            ],
          ),
        );
      },
    );
  }
}
