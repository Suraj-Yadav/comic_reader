import 'dart:io';

import 'package:comic_reader/src/comic_viewer_route.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_size/window_size.dart' as window_size;

import 'comic.dart';

class ComicGridRoute extends StatelessWidget {
  final List<Comic> _comics;

  ComicGridRoute(this._comics);

  @override
  Widget build(BuildContext context) {
    if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
      window_size.setWindowTitle('${_comics.length} comics');
    }
    final count =
        MediaQuery.of(context).orientation == Orientation.landscape ? 5 : 2;
    return RawKeyboardListener(
      autofocus: true,
      focusNode: new FocusNode(),
      onKey: (RawKeyEvent value) {
        if (value is RawKeyDownEvent &&
            value.data.logicalKey == LogicalKeyboardKey.escape) {
          Navigator.pop(context);
        }
      },
      child: Container(
        color: Colors.white,
        child: GridView.count(
          crossAxisCount: count,
          childAspectRatio: 0.650196335078534,
          children: _comics
              .map((Comic comic) => ClipRRect(
                  borderRadius: BorderRadius.circular(10),
                  child: RaisedButton(
                    color: Colors.white,
                    hoverColor: Colors.grey,
                    padding: EdgeInsets.all(2),
                    child: ClipRRect(
                      borderRadius: BorderRadius.circular(10),
                      child: comic.firstPage.page,
                    ),
                    onPressed: () {
                      Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => ComicViewerRoute(comic),
                          ));
                    },
                  )))
              .toList(),
        ),
      ),
    );
  }
}
