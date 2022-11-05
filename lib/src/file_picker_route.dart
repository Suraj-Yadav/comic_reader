import 'dart:io';

import 'package:comic_reader/src/comic_gallery_route.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:window_size/window_size.dart' as window_size;

class FilePickerRoute extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
      window_size.setWindowTitle("Choose Files");
    }
    return Scaffold(
      appBar: AppBar(
        title: Text("Choose Files"),
      ),
      body: Center(
        child: ElevatedButton(
          style: ElevatedButton.styleFrom(padding: EdgeInsets.all(20)),
          onPressed: () async {
            final List<String> filePaths = [];
            var res = await FilePicker.platform.pickFiles(allowMultiple: true);
            filePaths
                .addAll(res.paths.where((element) => element.endsWith('cbz')));

            if (filePaths.isEmpty) {
              return;
            }

            Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => ComicGalleryRoute(filePaths),
                ));
          },
          child: Text('Open file picker'),
        ),
      ),
    );
  }
}
