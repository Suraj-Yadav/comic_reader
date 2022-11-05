import 'dart:io';

import 'package:comic_reader/src/comic_viewer_route.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
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
    final List<Widget> children = [];
    for (var index = 0; index < _comics.length; index++) {
      final comic = _comics[index];
      children.add(ClipRRect(
        borderRadius: BorderRadius.circular(10),
        child: ElevatedButton(
          style: ElevatedButton.styleFrom(
            primary: Colors.white,
          ),
          child: ClipRRect(
            borderRadius: BorderRadius.circular(10),
            child: Stack(
              children: [
                Align(
                  alignment: Alignment.center,
                  child: Image.file(comic.firstPage.imgFile),
                ),
                Align(
                  alignment: Alignment.bottomCenter,
                  child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 5),
                      color: Colors.grey,
                      width: MediaQuery.of(context).size.width,
                      child: Text(
                        path.basenameWithoutExtension(comic.archiveFilePath),
                        textAlign: TextAlign.center,
                        style: TextStyle(fontSize: 10),
                      )),
                )
              ],
            ),
          ),
          onPressed: () {
            Navigator.push(
                context,
                MaterialPageRoute(
                    builder: (context) => ComicViewerRoute(_comics, index)));
          },
        ),
      ));
    }
    return RawKeyboardListener(
      autofocus: true,
      focusNode: new FocusNode(),
      onKey: (RawKeyEvent value) {
        if (value is RawKeyDownEvent &&
            value.data.logicalKey == LogicalKeyboardKey.escape) {
          // Navigator.pop(context);
        }
      },
      child: Container(
        color: Colors.white,
        child: GridView.count(
          crossAxisCount: count,
          childAspectRatio: 0.650196335078534,
          children: children,
        ),
      ),
    );
  }
}
