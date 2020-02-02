import 'package:comic_reader/src/comic_viewer_route.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;

import 'comic.dart';

class ComicGridRoute extends StatelessWidget {
  final List<Comic> _comics;

  ComicGridRoute(this._comics);

  @override
  Widget build(BuildContext context) {
    // if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
    //   window_size.setWindowTitle('Choose File');
    // }
    return RawKeyboardListener(
      focusNode: new FocusNode(),
      onKey: (RawKeyEvent value) {
        if (value is RawKeyDownEvent &&
            value.data.logicalKey == LogicalKeyboardKey.escape) {
          Navigator.pop(context);
        }
      },
      child: GridView.count(
        crossAxisCount: 4,
        childAspectRatio: 0.5,
        children: _comics
            .map((Comic comic) => RaisedButton(
                  child: Column(
                    children: [
                      comic.firstPage.page,
                      Text(path.basenameWithoutExtension(comic.filePath))
                    ],
                  ),
                  onPressed: () {
                    // if (Platform.isMacOS ||
                    //     Platform.isWindows ||
                    //     Platform.isLinux) {
                    //   window_size.setWindowTitle(
                    //       path.basenameWithoutExtension(comic.filePath));
                    // }
                    Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => ComicViewerRoute(comic),
                        ));
                  },
                ))
            .toList(),
      ),
    );
  }
}
